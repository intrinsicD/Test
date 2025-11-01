#include "engine/runtime/api.hpp"
#include "engine/runtime/diagnostics_bridge.hpp"
#include "engine/runtime/errors.hpp"
#include "engine/runtime/loop.hpp"
#include "engine/io/telemetry.hpp"
#include "engine/math/telemetry/conversion_telemetry.hpp"
#include "engine/animation/benchmarking/telemetry.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <span>
#include <stdexcept>
#include <system_error>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <new>
#include <spdlog/spdlog.h>

#include "engine/animation/deformation/linear_blend_skinning.hpp"
#include "engine/geometry/deform/linear_blend_skinning.hpp"
#include "engine/geometry/telemetry.hpp"

#if ENGINE_ENABLE_ASSETS
#    include "engine/assets/api.hpp"
#    include "engine/assets/async.hpp"
#    include "engine/assets/validation.hpp"
#endif
#if ENGINE_ENABLE_COMPUTE_CUDA
#    include "engine/compute/cuda/api.hpp"
#endif
#if ENGINE_ENABLE_CORE
#    include "engine/core/api.hpp"
#endif
#if ENGINE_ENABLE_IO
#    include "engine/io/api.hpp"
#endif
#if ENGINE_ENABLE_PLATFORM
#    include "engine/platform/api.hpp"
#endif
#if ENGINE_ENABLE_RENDERING
#    include "engine/rendering/api.hpp"
#    include "engine/rendering/command_encoder.hpp"
#    include "engine/rendering/components.hpp"
#    include "engine/rendering/frame_graph.hpp"
#    include "engine/rendering/forward_pipeline.hpp"
#    include "engine/rendering/pipeline/research_baseline_telemetry.hpp"
#    include "engine/rendering/material_system.hpp"
#endif
#if ENGINE_ENABLE_SCENE
#    include "engine/scene/api.hpp"
#endif
#include "engine/scene/components.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/systems.hpp"

namespace
{
    constexpr std::uint64_t kHierarchyFailureWarningThreshold = 3;
    constexpr std::uint64_t kHierarchyFailureCriticalThreshold = 10;

    [[nodiscard]] engine::runtime::SceneValidationAlertLevel evaluate_scene_validation_alert(
        std::uint64_t consecutive_failures) noexcept
    {
        using engine::runtime::SceneValidationAlertLevel;
        if (consecutive_failures >= kHierarchyFailureCriticalThreshold)
        {
            return SceneValidationAlertLevel::Critical;
        }
        if (consecutive_failures >= kHierarchyFailureWarningThreshold)
        {
            return SceneValidationAlertLevel::Warning;
        }
        return SceneValidationAlertLevel::None;
    }

    using engine::runtime::RuntimeError;
    using engine::runtime::RuntimeErrorCode;

    enum class PluginVisitState
    {
        none,
        visiting,
        visited,
    };

    [[nodiscard]] std::string format_cycle_message(const std::vector<std::string>& cycle)
    {
        std::ostringstream builder;
        builder << "Subsystem dependency cycle detected: ";
        for (std::size_t index = 0; index < cycle.size(); ++index)
        {
            builder << cycle[index];
            if (index + 1 < cycle.size())
            {
                builder << " -> ";
            }
        }
        return builder.str();
    }

    [[nodiscard]] std::optional<std::vector<std::string>> detect_plugin_cycle(
        std::span<const std::shared_ptr<engine::core::plugin::ISubsystemInterface>> plugins)
    {
        std::unordered_map<std::string_view, std::size_t, std::hash<std::string_view>, std::equal_to<>> index_map{};
        index_map.reserve(plugins.size());

        for (std::size_t index = 0; index < plugins.size(); ++index)
        {
            const auto& plugin = plugins[index];
            if (plugin == nullptr)
            {
                continue;
            }

            index_map.emplace(plugin->name(), index);
        }

        std::vector<PluginVisitState> states(plugins.size(), PluginVisitState::none);
        std::vector<std::string_view> stack{};
        std::vector<std::string> cycle{};
        stack.reserve(plugins.size());

        const auto record_cycle = [&](std::string_view dependency_name)
        {
            cycle.clear();
            const auto begin = std::find(stack.begin(), stack.end(), dependency_name);
            for (auto it = begin; it != stack.end(); ++it)
            {
                cycle.emplace_back(*it);
            }
            cycle.emplace_back(dependency_name);
        };

        const auto dfs = [&](auto&& self, std::size_t index) -> bool
        {
            if (states[index] == PluginVisitState::visited)
            {
                return false;
            }

            const auto& plugin = plugins[index];
            if (plugin == nullptr)
            {
                states[index] = PluginVisitState::visited;
                return false;
            }

            states[index] = PluginVisitState::visiting;
            stack.push_back(plugin->name());

            for (const auto dependency_name : plugin->dependencies())
            {
                const auto dependency = index_map.find(dependency_name);
                if (dependency == index_map.end())
                {
                    continue;
                }

                const auto dependency_index = dependency->second;
                if (states[dependency_index] == PluginVisitState::visiting)
                {
                    record_cycle(dependency_name);
                    return true;
                }

                if (states[dependency_index] == PluginVisitState::none)
                {
                    if (self(self, dependency_index))
                    {
                        return true;
                    }
                }
            }

            stack.pop_back();
            states[index] = PluginVisitState::visited;
            return false;
        };

        for (std::size_t index = 0; index < plugins.size(); ++index)
        {
            if (states[index] != PluginVisitState::none)
            {
                continue;
            }

            if (dfs(dfs, index))
            {
                return cycle;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::string format_dependency_errors(std::span<const RuntimeErrorCode> errors)
    {
        std::ostringstream message;
        message << "RuntimeHostDependencies validation failed";
        for (const auto& error : errors)
        {
            message << " [" << error.identifier();
            if (error.has_message())
            {
                message << ": " << error.message();
            }
            message << ']';
        }
        return message.str();
    }

    [[nodiscard]] std::vector<RuntimeErrorCode> validate_dependencies(
        const engine::runtime::RuntimeHostDependencies& dependencies)
    {
        using engine::runtime::make_runtime_error;

        std::vector<RuntimeErrorCode> errors{};

        const auto mesh_vertex_count = dependencies.mesh.rest_positions.size();
        if (dependencies.mesh.positions.size() != mesh_vertex_count)
        {
            std::ostringstream builder;
            builder << "mesh.positions size (" << dependencies.mesh.positions.size()
                << ") must match mesh.rest_positions size (" << mesh_vertex_count << ")";
            errors.push_back(make_runtime_error(RuntimeError::dependency_invalid_mesh, builder.str()));
        }

        const bool binding_has_vertices = !dependencies.binding.vertices.empty();
        const bool binding_has_joints = !dependencies.binding.joints.empty();
        if (binding_has_vertices && !binding_has_joints)
        {
            errors.push_back(make_runtime_error(
                RuntimeError::dependency_invalid_binding,
                "Rig binding provides vertex influences but no joints"));
        }

        if (binding_has_joints)
        {
            if (binding_has_vertices && dependencies.binding.vertices.size() != mesh_vertex_count)
            {
                std::ostringstream builder;
                builder << "Rig binding vertex count (" << dependencies.binding.vertices.size()
                    << ") must match mesh vertex count (" << mesh_vertex_count << ")";
                errors.push_back(make_runtime_error(RuntimeError::dependency_invalid_binding, builder.str()));
            }

            if (binding_has_vertices && !engine::animation::skinning::validate_binding(dependencies.binding))
            {
                errors.push_back(make_runtime_error(
                    RuntimeError::dependency_invalid_binding,
                    "Rig binding contains invalid joint indices or non-normalized weights"));
            }
        }

        const auto clip_errors = engine::animation::validate_clip(dependencies.controller.clip);
        if (!clip_errors.empty())
        {
            std::ostringstream builder;
            builder << "Animation clip '";
            if (dependencies.controller.clip.name.empty())
            {
                builder << "<unnamed>";
            }
            else
            {
                builder << dependencies.controller.clip.name;
            }
            builder << "' failed validation";

            const std::size_t limit = std::min<std::size_t>(clip_errors.size(), 3U);
            for (std::size_t index = 0; index < limit; ++index)
            {
                const auto& error = clip_errors[index];
                builder << ": " << error.message;
                if (!error.joint_name.empty())
                {
                    builder << " (joint=" << error.joint_name << ')';
                }
                if (error.track_index != std::numeric_limits<std::size_t>::max())
                {
                    builder << " (track=" << error.track_index << ')';
                }
                if (error.keyframe_index != std::numeric_limits<std::size_t>::max())
                {
                    builder << " (keyframe=" << error.keyframe_index << ')';
                }
            }
            if (clip_errors.size() > limit)
            {
                builder << " (" << (clip_errors.size() - limit) << " additional issues)";
            }

            errors.push_back(
                make_runtime_error(RuntimeError::dependency_invalid_clip, builder.str()));
        }

        if (!dependencies.subsystem_plugins.empty())
        {
            if (const auto cycle = detect_plugin_cycle(dependencies.subsystem_plugins))
            {
                errors.push_back(
                    make_runtime_error(RuntimeError::dependency_cycle, format_cycle_message(*cycle)));
            }
        }

        return errors;
    }

    [[nodiscard]] const char* hierarchy_issue_type_name(
        engine::scene::validation::HierarchyIssueType type) noexcept
    {
        return engine::scene::validation::to_string(type).data();
    }

    [[nodiscard]] engine::geometry::SurfaceMesh make_runtime_skinning_mesh()
    {
        constexpr std::uint32_t subdivisions = 128;
        const std::uint32_t vertices_per_axis = subdivisions + 1;
        const float step = 1.0F / static_cast<float>(subdivisions);

        engine::geometry::SurfaceMesh mesh{};
        mesh.rest_positions.reserve(static_cast<std::size_t>(vertices_per_axis) * vertices_per_axis);
        for (std::uint32_t y = 0; y < vertices_per_axis; ++y)
        {
            const float z = -0.5F + step * static_cast<float>(y);
            for (std::uint32_t x = 0; x < vertices_per_axis; ++x)
            {
                const float px = -0.5F + step * static_cast<float>(x);
                mesh.rest_positions.emplace_back(px, 0.0F, z);
            }
        }
        mesh.positions = mesh.rest_positions;
        mesh.normals.assign(mesh.rest_positions.size(), engine::math::vec3{0.0F, 1.0F, 0.0F});

        mesh.indices.reserve(static_cast<std::size_t>(subdivisions) * subdivisions * 6U);
        for (std::uint32_t y = 0; y < subdivisions; ++y)
        {
            for (std::uint32_t x = 0; x < subdivisions; ++x)
            {
                const std::uint32_t top_left = y * vertices_per_axis + x;
                const std::uint32_t top_right = top_left + 1U;
                const std::uint32_t bottom_left = top_left + vertices_per_axis;
                const std::uint32_t bottom_right = bottom_left + 1U;

                mesh.indices.push_back(top_left);
                mesh.indices.push_back(top_right);
                mesh.indices.push_back(bottom_right);

                mesh.indices.push_back(top_left);
                mesh.indices.push_back(bottom_right);
                mesh.indices.push_back(bottom_left);
            }
        }

        engine::geometry::update_bounds(mesh);
        return mesh;
    }

    engine::runtime::RuntimeHostDependencies make_default_dependencies()
    {
        engine::runtime::RuntimeHostDependencies deps{};
        deps.mesh = make_runtime_skinning_mesh();
        auto registry = std::make_shared<engine::runtime::SubsystemRegistry>(
            engine::runtime::make_default_subsystem_registry());
        deps.subsystem_registry = registry;

        deps.binding.joints.clear();
        engine::animation::RigJoint root{};
        root.name = "root";
        root.parent = engine::animation::RigBinding::kInvalidIndex;
        root.inverse_bind_pose = engine::math::Transform<float>::Identity();
        deps.binding.joints.push_back(root);
        deps.binding.resize_vertices(deps.mesh.rest_positions.size());
        for (auto& vertex : deps.binding.vertices)
        {
            vertex.clear();
            MAYBE_UNUSED_CONST_AUTO added = vertex.add_influence(0U, 1.0F);
            (void)added;
            vertex.normalize_weights();
        }
        return deps;
    }
} // namespace

namespace engine::runtime::detail
{
    void update_scene_validation_alert_state(RuntimeDiagnostics& diagnostics,
                                             const scene::validation::HierarchyValidationReport& report,
                                             double simulation_time,
                                             double wall_seconds) noexcept
    {
        if (!report.ok())
        {
            diagnostics.scene_validation_failure_frame_count += 1U;
            diagnostics.scene_validation_consecutive_failure_frames += 1U;
            diagnostics.scene_validation_max_consecutive_failure_frames = std::max(
                diagnostics.scene_validation_max_consecutive_failure_frames,
                diagnostics.scene_validation_consecutive_failure_frames);
            diagnostics.last_scene_validation_failure_simulation_time = simulation_time;
            diagnostics.last_scene_validation_failure_wall_seconds = wall_seconds;
        }
        else
        {
            diagnostics.scene_validation_consecutive_failure_frames = 0U;
        }

        diagnostics.scene_validation_alert_level = evaluate_scene_validation_alert(
            diagnostics.scene_validation_consecutive_failure_frames);
    }
}

namespace engine::runtime
{
#if ENGINE_ENABLE_RENDERING
    namespace
    {
        rendering::ResearchBaselineOptions& shared_research_rendering_options() noexcept
        {
            static rendering::ResearchBaselineOptions options{};
            return options;
        }
    } // namespace
#endif

    struct RuntimeHost::Impl
    {
        RuntimeHostDependencies dependencies{};
        bool initialized{false};
        double simulation_time{0.0};
        animation::AnimationController controller{};
        animation::AnimationRigPose pose{};
        geometry::SurfaceMesh mesh{};
        animation::RigBinding binding{};
        physics::PhysicsWorld world{};
        RuntimeLoopPlan loop_plan{};
        compute::ExecutionReport last_report{};
        std::vector<math::vec3> body_positions{};
        std::vector<std::string> joint_names{};
        scene::Scene scene{};
        std::vector<scene::Entity> joint_entities{};
        std::vector<runtime_frame_state::scene_node_state> scene_nodes{};
        std::vector<std::string_view> subsystem_names{};
        std::vector<math::Transform<float>> joint_global_transforms{};
        std::vector<math::Transform<float>> skinning_transforms{};
        using Clock = std::chrono::steady_clock;
        Clock::time_point runtime_start_time{Clock::now()};
        RuntimeDiagnostics diagnostics{};
        std::unordered_map<std::string, std::size_t> stage_lookup{};
        std::unordered_map<std::string, std::size_t> subsystem_lookup{};
#if ENGINE_ENABLE_RENDERING
        rendering::components::RenderGeometry render_geometry{};
        std::string renderable_name{"runtime.renderable"};
        scene::Entity render_entity{};
        rendering::ForwardPipeline forward_pipeline{};
        rendering::ResearchBaselineOptions research_options{};
#endif
#if ENGINE_ENABLE_ASSETS
        assets::MeshCache* mesh_cache{nullptr};
        assets::PointCloudCache* point_cloud_cache{nullptr};
#endif

        static void throw_if_invalid_dependencies(const RuntimeHostDependencies& deps)
        {
            const auto errors = validate_dependencies(deps);
            if (!errors.empty())
            {
                throw std::runtime_error(format_dependency_errors(errors));
            }
        }

        void validate_dispatcher_factory() const
        {
            std::unique_ptr<compute::Dispatcher> dispatcher_instance{};
            if (dependencies.dispatcher_factory)
            {
                dispatcher_instance = dependencies.dispatcher_factory();
            }
            else
            {
                dispatcher_instance = compute::make_cpu_dispatcher();
            }
            if (dispatcher_instance == nullptr)
            {
                throw std::runtime_error("RuntimeHost dispatcher_factory produced a null dispatcher");
            }
        }

        explicit Impl(RuntimeHostDependencies deps)
        {
            throw_if_invalid_dependencies(deps);
            dependencies = std::move(deps);
#if ENGINE_ENABLE_RENDERING
            render_geometry = dependencies.render_geometry;
            if (!dependencies.renderable_name.empty())
            {
                renderable_name = dependencies.renderable_name;
            }
            research_options = shared_research_rendering_options();
#endif
            reset_state();
        }

        void ensure_subsystem_plugins_loaded()
        {
            if (!dependencies.subsystem_plugins.empty())
            {
                return;
            }

            if (dependencies.subsystem_registry == nullptr)
            {
                return;
            }

            std::vector<std::string_view> selection{};
            selection.reserve(dependencies.enabled_subsystems.size());
            for (const auto& name : dependencies.enabled_subsystems)
            {
                selection.push_back(name);
            }

            if (selection.empty())
            {
                dependencies.subsystem_plugins = dependencies.subsystem_registry->load_defaults();
            }
            else
            {
                dependencies.subsystem_plugins = dependencies.subsystem_registry->load(selection);
            }
        }

        void rebuild_subsystem_cache()
        {
            ensure_subsystem_plugins_loaded();
            subsystem_names.clear();
            subsystem_names.reserve(dependencies.subsystem_plugins.size());
            for (const auto& plugin : dependencies.subsystem_plugins)
            {
                if (plugin != nullptr)
                {
                    subsystem_names.push_back(plugin->name());
                }
            }
            sync_subsystem_metrics();
        }

        static double duration_to_ms(Clock::duration duration)
        {
            return std::chrono::duration<double, std::milli>(duration).count();
        }

        RuntimeStageTiming& ensure_stage_timing(const std::string& name)
        {
            auto it = stage_lookup.find(name);
            if (it != stage_lookup.end())
            {
                return diagnostics.stage_timings[it->second];
            }
            RuntimeStageTiming timing{};
            timing.name = name;
            diagnostics.stage_timings.push_back(std::move(timing));
            const std::size_t index = diagnostics.stage_timings.size() - 1U;
            stage_lookup.emplace(name, index);
            return diagnostics.stage_timings[index];
        }

        RuntimeSubsystemTiming& ensure_subsystem_timing(const std::string& name)
        {
            auto it = subsystem_lookup.find(name);
            if (it != subsystem_lookup.end())
            {
                return diagnostics.subsystem_timings[it->second];
            }
            RuntimeSubsystemTiming timing{};
            timing.name = name;
            diagnostics.subsystem_timings.push_back(std::move(timing));
            const std::size_t index = diagnostics.subsystem_timings.size() - 1U;
            subsystem_lookup[name] = index;
            return diagnostics.subsystem_timings[index];
        }

        static std::string animation_dispatch_category(std::string_view name)
        {
            if (name.empty())
            {
                return std::string{"unknown"};
            }
            const auto separator = name.find('.');
            if (separator == std::string_view::npos)
            {
                return std::string{name};
            }
            if (separator == 0)
            {
                return std::string{"unknown"};
            }
            return std::string{name.substr(0, separator)};
        }

        static std::string animation_queue_label(compute::TimingDomain domain)
        {
            switch (domain)
            {
            case compute::TimingDomain::Cpu:
                return std::string{"cpu"};
            case compute::TimingDomain::Gpu:
                return std::string{"gpu"};
            case compute::TimingDomain::Unknown:
            default:
                return std::string{"unknown"};
            }
        }

        void update_animation_telemetry(const compute::ExecutionReport& report)
        {
            RuntimeAnimationTelemetry telemetry{};
            telemetry.clip_track_count = static_cast<std::uint64_t>(controller.clip.tracks.size());
            telemetry.pose_joint_count = static_cast<std::uint64_t>(pose.joints.size());
            telemetry.clip_duration = controller.clip.duration;
            telemetry.playback_time = controller.playback_time;
            telemetry.playback_speed = controller.playback_speed;

            std::vector<animation::benchmarking::DispatchTelemetry> dispatches{};
            dispatches.reserve(report.execution_order.size());
            const std::string queue_label = animation_queue_label(report.clock_domain);

            for (std::size_t index = 0; index < report.execution_order.size(); ++index)
            {
                animation::benchmarking::DispatchTelemetry entry{};
                entry.name = report.execution_order[index];
                entry.category = animation_dispatch_category(entry.name);
                entry.queue = queue_label;
                const double duration = index < report.kernel_durations.size()
                                            ? report.kernel_durations[index]
                                            : 0.0;
                entry.duration_ms = duration;
                dispatches.push_back(std::move(entry));
            }

            const auto category_totals =
                animation::benchmarking::aggregate_category_totals(dispatches);
            telemetry.category_totals.clear();
            telemetry.category_totals.reserve(category_totals.size());
            for (const auto& entry : category_totals)
            {
                RuntimeAnimationDispatchTotal total{};
                total.label = std::string(entry.label);
                total.duration_ms = entry.duration_ms;
                telemetry.category_totals.push_back(std::move(total));
            }

            const auto queue_totals =
                animation::benchmarking::aggregate_queue_totals(dispatches);
            telemetry.queue_totals.clear();
            telemetry.queue_totals.reserve(queue_totals.size());
            for (const auto& entry : queue_totals)
            {
                RuntimeAnimationDispatchTotal total{};
                total.label = std::string(entry.label);
                total.duration_ms = entry.duration_ms;
                telemetry.queue_totals.push_back(std::move(total));
            }

            diagnostics.animation = std::move(telemetry);
        }

        static std::string_view classify_exception(const std::exception& exception) noexcept
        {
            const std::type_info& info = typeid(exception);
            if (info == typeid(std::invalid_argument))
            {
                return "std::invalid_argument";
            }
            if (info == typeid(std::out_of_range))
            {
                return "std::out_of_range";
            }
            if (info == typeid(std::logic_error))
            {
                return "std::logic_error";
            }
            if (info == typeid(std::runtime_error))
            {
                return "std::runtime_error";
            }
            if (info == typeid(std::system_error))
            {
                return "std::system_error";
            }
            if (info == typeid(std::bad_alloc))
            {
                return "std::bad_alloc";
            }
            return "std::exception";
        }

        void record_subsystem_failure(const std::string& name,
                                      std::string_view category,
                                      std::string_view message,
                                      double duration_ms)
        {
            RuntimeSubsystemTiming& timing = ensure_subsystem_timing(name);
            timing.initialize_failure_count += 1U;
            timing.last_initialize_failure_category.assign(category);
            timing.last_initialize_failure_message.assign(message);
            timing.last_initialize_failure_ms = duration_ms;
        }

        void record_initialize_failure(const std::string& name,
                                       std::string_view category,
                                       std::string_view message,
                                       double duration_ms)
        {
            record_subsystem_failure(name, category, message, duration_ms);

            diagnostics.initialize_failure_count += 1U;
            diagnostics.has_initialize_failure = true;

            RuntimeInitializationFailure failure{};
            failure.runtime = scene_name();
            failure.subsystem = name;
            failure.category.assign(category);
            failure.message.assign(message);
            failure.duration_ms = duration_ms;
            diagnostics.last_initialize_failure = std::move(failure);

            rebuild_metric_snapshot();

            spdlog::error(
                "runtime.lifecycle.initialize_failure runtime={} subsystem={} category={} message={} duration_ms={:.3f} failures={}",
                diagnostics.last_initialize_failure.runtime,
                diagnostics.last_initialize_failure.subsystem,
                diagnostics.last_initialize_failure.category,
                diagnostics.last_initialize_failure.message,
                diagnostics.last_initialize_failure.duration_ms,
                diagnostics.initialize_failure_count);
        }

        void sync_subsystem_metrics()
        {
            std::unordered_set<std::string> active{};
            active.reserve(dependencies.subsystem_plugins.size());
            for (const auto& plugin : dependencies.subsystem_plugins)
            {
                if (plugin == nullptr)
                {
                    continue;
                }
                std::string name{plugin->name()};
                ensure_subsystem_timing(name);
                active.insert(std::move(name));
            }

            for (std::size_t index = 0; index < diagnostics.subsystem_timings.size();)
            {
                const auto& entry = diagnostics.subsystem_timings[index];
                if (active.find(entry.name) == active.end())
                {
                    subsystem_lookup.erase(entry.name);
                    diagnostics.subsystem_timings.erase(
                        diagnostics.subsystem_timings.begin() +
                        static_cast<std::vector<RuntimeSubsystemTiming>::difference_type>(index));
                    for (std::size_t update = index; update < diagnostics.subsystem_timings.size(); ++update)
                    {
                        subsystem_lookup[diagnostics.subsystem_timings[update].name] = update;
                    }
                    continue;
                }
                ++index;
            }
            rebuild_metric_snapshot();
        }

        static std::vector<core::telemetry::Label> make_single_label(std::string_view key,
                                                                     std::string_view value)
        {
            std::vector<core::telemetry::Label> labels{};
            labels.reserve(1);
            core::telemetry::Label label{};
            label.key.assign(key);
            label.value.assign(value);
            labels.push_back(std::move(label));
            return labels;
        }

        static std::vector<core::telemetry::Label> make_operation_labels(io::GeometryIoOperation operation)
        {
            return make_single_label("operation", io::to_string(operation));
        }

        static std::vector<core::telemetry::Label> make_operation_error_labels(io::GeometryIoOperation operation,
                                                                               io::GeometryIoError error)
        {
            auto labels = make_operation_labels(operation);
            core::telemetry::Label error_label{};
            error_label.key = "error";
            error_label.value.assign(io::to_string(error));
            labels.push_back(std::move(error_label));
            return labels;
        }

        void rebuild_metric_snapshot()
        {
            core::telemetry::MetricSet snapshot{};
            snapshot.descriptors.reserve(64);
            snapshot.samples.reserve(64);

            const auto clamp_to_int = [](auto value) -> std::int64_t
            {
                const auto as_u64 = static_cast<std::uint64_t>(value);
                const auto max_value = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
                return static_cast<std::int64_t>(std::min(as_u64, max_value));
            };

            const auto add_counter = [&](std::string_view name,
                                         std::string_view description,
                                         std::int64_t value,
                                         std::vector<core::telemetry::Label> labels = {})
            {
                const std::size_t index = snapshot.descriptors.size();
                core::telemetry::MetricDescriptor descriptor{};
                descriptor.name.assign(name);
                descriptor.kind = core::telemetry::MetricKind::Counter;
                descriptor.unit = core::telemetry::MetricUnit::Count;
                descriptor.description.assign(description);
                descriptor.labels = std::move(labels);
                snapshot.descriptors.push_back(std::move(descriptor));

                core::telemetry::MetricSample sample{};
                sample.descriptor_index = index;
                sample.value = value;
                snapshot.samples.push_back(std::move(sample));
            };

            const auto add_gauge = [&](std::string_view name,
                                       std::string_view description,
                                       double value,
                                       core::telemetry::MetricUnit unit,
                                       std::vector<core::telemetry::Label> labels = {})
            {
                const std::size_t index = snapshot.descriptors.size();
                core::telemetry::MetricDescriptor descriptor{};
                descriptor.name.assign(name);
                descriptor.kind = core::telemetry::MetricKind::Gauge;
                descriptor.unit = unit;
                descriptor.description.assign(description);
                descriptor.labels = std::move(labels);
                snapshot.descriptors.push_back(std::move(descriptor));

                core::telemetry::MetricSample sample{};
                sample.descriptor_index = index;
                sample.value = value;
                snapshot.samples.push_back(std::move(sample));
            };

            add_counter("runtime.lifecycle.initialize.count",
                        "Total RuntimeHost::initialize invocations",
                        clamp_to_int(diagnostics.initialize_count));
            add_counter("runtime.lifecycle.initialize.failures",
                        "Total RuntimeHost::initialize failures",
                        clamp_to_int(diagnostics.initialize_failure_count));
            add_counter("runtime.lifecycle.shutdown.count",
                        "Total RuntimeHost::shutdown invocations",
                        clamp_to_int(diagnostics.shutdown_count));
            add_counter("runtime.lifecycle.tick.count",
                        "Total RuntimeHost::tick invocations",
                        clamp_to_int(diagnostics.tick_count));

            add_gauge("runtime.lifecycle.last_initialize_ms",
                      "Duration of the most recent initialize() call",
                      diagnostics.last_initialize_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.max_initialize_ms",
                      "Longest initialize() duration recorded",
                      diagnostics.max_initialize_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.last_shutdown_ms",
                      "Duration of the most recent shutdown() call",
                      diagnostics.last_shutdown_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.max_shutdown_ms",
                      "Longest shutdown() duration recorded",
                      diagnostics.max_shutdown_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.last_tick_ms",
                      "Duration of the most recent tick() call",
                      diagnostics.last_tick_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.max_tick_ms",
                      "Longest tick() duration recorded",
                      diagnostics.max_tick_ms,
                      core::telemetry::MetricUnit::Milliseconds);
            add_gauge("runtime.lifecycle.average_tick_ms",
                      "Average tick() duration",
                      diagnostics.average_tick_ms,
                      core::telemetry::MetricUnit::Milliseconds);

            const auto& streaming = diagnostics.streaming;
            add_gauge("runtime.streaming.worker_count",
                      "Configured asynchronous streaming worker threads",
                      static_cast<double>(streaming.worker_count),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.streaming.queue_capacity",
                      "Maximum number of pending streaming tasks",
                      static_cast<double>(streaming.queue_capacity),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.streaming.pending_tasks",
                      "Current number of enqueued streaming tasks",
                      static_cast<double>(streaming.pending_tasks),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.streaming.active_workers",
                      "Number of workers executing streaming tasks",
                      static_cast<double>(streaming.active_workers),
                      core::telemetry::MetricUnit::Count);
            add_counter("runtime.streaming.total_enqueued",
                        "Total streaming requests enqueued",
                        clamp_to_int(streaming.total_enqueued));
            add_counter("runtime.streaming.total_executed",
                        "Total streaming requests executed",
                        clamp_to_int(streaming.total_executed));
            add_gauge("runtime.streaming.pending_handles",
                      "Assets awaiting completion in streaming caches",
                      static_cast<double>(streaming.streaming_pending),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.streaming.loading_handles",
                      "Assets currently decoding in streaming caches",
                      static_cast<double>(streaming.streaming_loading),
                      core::telemetry::MetricUnit::Count);
            add_counter("runtime.streaming.total_requests",
                        "Total asset streaming requests observed",
                        clamp_to_int(streaming.streaming_total_requests));
            add_counter("runtime.streaming.total_completed",
                        "Completed asset streaming requests",
                        clamp_to_int(streaming.streaming_total_completed));
            add_counter("runtime.streaming.total_failed",
                        "Failed asset streaming requests",
                        clamp_to_int(streaming.streaming_total_failed));
            add_counter("runtime.streaming.total_cancelled",
                        "Cancelled asset streaming requests",
                        clamp_to_int(streaming.streaming_total_cancelled));
            add_counter("runtime.streaming.total_rejected",
                        "Rejected asset streaming requests due to capacity",
                        clamp_to_int(streaming.streaming_total_rejected));
            const auto geometry_errors = engine::io::geometry_io_error_codes();
            for (std::size_t error_index = 0; error_index < geometry_errors.size(); ++error_index)
            {
                add_counter("runtime.streaming.geometry_failures",
                            "Failed asset streaming requests grouped by geometry error code",
                            clamp_to_int(streaming.streaming_geometry_failures[error_index]),
                            make_single_label("error", io::to_string(geometry_errors[error_index])));
            }

            const auto& hot_reload = diagnostics.hot_reload;
            add_counter("runtime.hot_reload.attempt_count",
                        "Hot reload attempts observed across all caches",
                        clamp_to_int(hot_reload.attempt_count));
            add_counter("runtime.hot_reload.failure_count",
                        "Hot reload attempts that resulted in failure",
                        clamp_to_int(hot_reload.failure_count));
            add_counter("runtime.hot_reload.cancelled_count",
                        "Hot reload attempts cancelled before completion",
                        clamp_to_int(hot_reload.cancelled_count));
            add_counter("runtime.hot_reload.rejected_count",
                        "Hot reload attempts rejected due to capacity limits",
                        clamp_to_int(hot_reload.rejected_count));
            add_gauge("runtime.hot_reload.pending_count",
                      "Hot reload requests waiting for completion",
                      static_cast<double>(hot_reload.pending_count),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.hot_reload.loading_count",
                      "Hot reload requests currently decoding",
                      static_cast<double>(hot_reload.loading_count),
                      core::telemetry::MetricUnit::Count);

            add_gauge("runtime.animation.clip_duration",
                      "Duration of the active animation clip in seconds",
                      diagnostics.animation.clip_duration,
                      core::telemetry::MetricUnit::Seconds);
            add_gauge("runtime.animation.playback_time",
                      "Playback time of the active animation controller in seconds",
                      diagnostics.animation.playback_time,
                      core::telemetry::MetricUnit::Seconds);
            add_gauge("runtime.animation.playback_speed",
                      "Playback speed multiplier applied to the animation controller",
                      diagnostics.animation.playback_speed,
                      core::telemetry::MetricUnit::None);
            add_gauge("runtime.animation.pose_joint_count",
                      "Number of joints present in the evaluated animation pose",
                      static_cast<double>(diagnostics.animation.pose_joint_count),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.animation.clip_track_count",
                      "Number of joint tracks in the active animation clip",
                      static_cast<double>(diagnostics.animation.clip_track_count),
                      core::telemetry::MetricUnit::Count);

            for (const auto& entry : diagnostics.animation.category_totals)
            {
                add_gauge("runtime.animation.category.last_ms",
                          "Dispatcher cost attributed to animation categories",
                          entry.duration_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("category", entry.label));
            }

            for (const auto& entry : diagnostics.animation.queue_totals)
            {
                add_gauge("runtime.animation.queue.last_ms",
                          "Dispatcher queue cost attributed to animation workloads",
                          entry.duration_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("queue", entry.label));
            }

            const auto geometry_snapshot = geometry::GeometrySpatialTelemetry::instance().snapshot();
            constexpr std::array<std::string_view, geometry::geometry_spatial_query_operation_count()>
                geometry_operation_names{
                    "octree_build",
                    "octree_query_aabb",
                    "octree_query_sphere",
                    "octree_query_ray",
                    "octree_query_segment",
                    "octree_query_knn",
                    "octree_query_nearest",
                };
            static_assert(geometry_operation_names.size() == geometry::geometry_spatial_query_operation_count());

            const auto operation_labels = [&](std::size_t operation_index)
            {
                return make_single_label("operation", geometry_operation_names[operation_index]);
            };

            const auto add_geometry_counter = [&](
                std::string_view name,
                std::string_view description,
                std::uint64_t value,
                std::size_t operation_index)
            {
                add_counter(name,
                            description,
                            clamp_to_int(value),
                            operation_labels(operation_index));
            };

            const auto add_geometry_gauge = [&](
                std::string_view name,
                std::string_view description,
                std::uint64_t value,
                std::size_t operation_index)
            {
                add_gauge(name,
                          description,
                          static_cast<double>(value),
                          core::telemetry::MetricUnit::Count,
                          operation_labels(operation_index));
            };

            for (std::size_t op_index = 0; op_index < geometry_operation_names.size(); ++op_index)
            {
                const auto& operation_metrics = geometry_snapshot.operations[op_index];

                add_geometry_counter("runtime.geometry.spatial.invocations",
                                     "Spatial query invocations grouped by operation",
                                     operation_metrics.invocations,
                                     op_index);
                add_geometry_counter("runtime.geometry.spatial.result_total",
                                     "Spatial query results accumulated across all invocations",
                                     operation_metrics.total_results,
                                     op_index);
                add_geometry_gauge("runtime.geometry.spatial.last_results",
                                   "Result count returned by the most recent spatial query invocation",
                                   operation_metrics.last_results,
                                   op_index);
                add_geometry_gauge("runtime.geometry.spatial.max_results",
                                   "Maximum result count observed for the spatial query operation",
                                   operation_metrics.max_results,
                                   op_index);
            }

#if ENGINE_ENABLE_RENDERING
            const auto shading_mode_name = [](rendering::ResearchShadingMode mode) noexcept
            {
                switch (mode)
                {
                case rendering::ResearchShadingMode::Forward:
                    return std::string_view{"forward"};
                case rendering::ResearchShadingMode::Deferred:
                    return std::string_view{"deferred"};
                }
                return std::string_view{"forward"};
            };

            const auto phase_name = [](rendering::PassPhase phase)
            {
                std::ostringstream stream;
                stream << phase;
                return stream.str();
            };

            const auto make_pass_labels = [&](const rendering::ResearchBaselinePassTelemetry& telemetry)
            {
                std::vector<core::telemetry::Label> labels{};
                labels.reserve(2);

                core::telemetry::Label pass_label{};
                pass_label.key = "pass";
                pass_label.value = telemetry.name;
                labels.push_back(std::move(pass_label));

                core::telemetry::Label phase_label{};
                phase_label.key = "phase";
                phase_label.value = phase_name(telemetry.phase);
                labels.push_back(std::move(phase_label));

                return labels;
            };

            const auto research_snapshot = rendering::ResearchBaselineTelemetry::instance().snapshot();

            const std::array modes{rendering::ResearchShadingMode::Forward, rendering::ResearchShadingMode::Deferred};
            for (std::size_t index = 0; index < modes.size(); ++index)
            {
                add_counter("rendering.research.shading_mode.selection",
                            "Research baseline shading mode selections grouped by mode",
                            clamp_to_int(research_snapshot.mode_selection_counts[index]),
                            make_single_label("mode", shading_mode_name(modes[index])));
            }

            add_gauge("rendering.research.shading_mode.active",
                      "Active research baseline shading mode (0 = forward, 1 = deferred)",
                      research_snapshot.active_mode == rendering::ResearchShadingMode::Deferred ? 1.0 : 0.0,
                      core::telemetry::MetricUnit::None);

            const std::array overlay_names{
                std::string_view{"normals"},
                std::string_view{"uv"},
                std::string_view{"material"},
                std::string_view{"light_volume"},
            };

            for (std::size_t index = 0; index < overlay_names.size(); ++index)
            {
                add_counter("rendering.research.overlay.selection",
                            "Research baseline overlay enable selections grouped by overlay",
                            clamp_to_int(research_snapshot.overlay_selection_counts[index]),
                            make_single_label("overlay", overlay_names[index]));
                add_gauge("rendering.research.overlay.enabled",
                          "Active state for research baseline overlays (1 = enabled)",
                          research_snapshot.overlays_enabled[index] ? 1.0 : 0.0,
                          core::telemetry::MetricUnit::None,
                          make_single_label("overlay", overlay_names[index]));
            }

            for (const auto& pass : research_snapshot.passes)
            {
                add_counter("rendering.research.pass.draw_calls_total",
                            "Total draw calls recorded by research baseline passes",
                            clamp_to_int(pass.total_draw_calls),
                            make_pass_labels(pass));
                add_gauge("rendering.research.pass.last_draw_calls",
                          "Draw calls submitted during the most recent research baseline pass execution",
                          static_cast<double>(pass.last_draw_calls),
                          core::telemetry::MetricUnit::Count,
                          make_pass_labels(pass));
                add_gauge("rendering.research.pass.last_gpu_time_ms",
                          "Most recent GPU execution time for the research baseline pass (milliseconds)",
                          pass.last_gpu_time_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_pass_labels(pass));
                add_gauge("rendering.research.pass.max_gpu_time_ms",
                          "Maximum recorded GPU execution time for the research baseline pass (milliseconds)",
                          pass.max_gpu_time_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_pass_labels(pass));
            }
#endif

            const auto make_dimension_labels = [](std::size_t dimension)
            {
                std::vector<core::telemetry::Label> labels{};
                labels.reserve(1);
                core::telemetry::Label dimension_label{};
                dimension_label.key = "dimension";
                dimension_label.value = std::to_string(dimension);
                labels.push_back(std::move(dimension_label));
                return labels;
            };

            const auto make_matrix_labels = [](std::size_t rows, std::size_t columns)
            {
                std::vector<core::telemetry::Label> labels{};
                labels.reserve(2);
                core::telemetry::Label row_label{};
                row_label.key = "rows";
                row_label.value = std::to_string(rows);
                labels.push_back(std::move(row_label));
                core::telemetry::Label column_label{};
                column_label.key = "columns";
                column_label.value = std::to_string(columns);
                labels.push_back(std::move(column_label));
                return labels;
            };

            const auto conversion_snapshot = math::telemetry::ConversionTelemetry::instance().snapshot();
            for (const auto& entry : conversion_snapshot.vectors)
            {
                add_counter("runtime.math.conversions.vector.samples",
                            "Vector conversion round-trip samples grouped by dimension",
                            clamp_to_int(entry.sample_count),
                            make_dimension_labels(entry.dimension));
                add_gauge("runtime.math.conversions.vector.max_abs_error",
                          "Maximum absolute error observed for vector conversion round-trips",
                          entry.max_abs_error,
                          core::telemetry::MetricUnit::None,
                          make_dimension_labels(entry.dimension));
                add_gauge("runtime.math.conversions.vector.max_relative_error",
                          "Maximum relative error observed for vector conversion round-trips",
                          entry.max_relative_error,
                          core::telemetry::MetricUnit::None,
                          make_dimension_labels(entry.dimension));
                add_gauge("runtime.math.conversions.vector.mean_abs_error",
                          "Mean absolute error observed for vector conversion round-trips",
                          entry.mean_abs_error,
                          core::telemetry::MetricUnit::None,
                          make_dimension_labels(entry.dimension));
                add_gauge("runtime.math.conversions.vector.mean_relative_error",
                          "Mean relative error observed for vector conversion round-trips",
                          entry.mean_relative_error,
                          core::telemetry::MetricUnit::None,
                          make_dimension_labels(entry.dimension));
            }

            for (const auto& entry : conversion_snapshot.matrices)
            {
                add_counter("runtime.math.conversions.matrix.samples",
                            "Matrix conversion round-trip samples grouped by size",
                            clamp_to_int(entry.sample_count),
                            make_matrix_labels(entry.rows, entry.columns));
                add_gauge("runtime.math.conversions.matrix.max_abs_error",
                          "Maximum absolute error observed for matrix conversion round-trips",
                          entry.max_abs_error,
                          core::telemetry::MetricUnit::None,
                          make_matrix_labels(entry.rows, entry.columns));
                add_gauge("runtime.math.conversions.matrix.max_relative_error",
                          "Maximum relative error observed for matrix conversion round-trips",
                          entry.max_relative_error,
                          core::telemetry::MetricUnit::None,
                          make_matrix_labels(entry.rows, entry.columns));
                add_gauge("runtime.math.conversions.matrix.mean_abs_error",
                          "Mean absolute error observed for matrix conversion round-trips",
                          entry.mean_abs_error,
                          core::telemetry::MetricUnit::None,
                          make_matrix_labels(entry.rows, entry.columns));
                add_gauge("runtime.math.conversions.matrix.mean_relative_error",
                          "Mean relative error observed for matrix conversion round-trips",
                          entry.mean_relative_error,
                          core::telemetry::MetricUnit::None,
                          make_matrix_labels(entry.rows, entry.columns));
            }

            const auto error_codes = io::geometry_io_error_codes();
            for (std::size_t op_index = 0; op_index < io::geometry_io_operation_count(); ++op_index)
            {
                const auto operation = static_cast<io::GeometryIoOperation>(op_index);
                const auto& entry = diagnostics.geometry_io.operations[op_index];
                add_counter("io.geometry.requests",
                            "Geometry IO operation attempts",
                            clamp_to_int(entry.attempts),
                            make_operation_labels(operation));
                add_counter("io.geometry.successes",
                            "Successful Geometry IO operations",
                            clamp_to_int(entry.successes),
                            make_operation_labels(operation));

                for (std::size_t error_index = 0; error_index < error_codes.size(); ++error_index)
                {
                    add_counter("io.geometry.failures",
                                "Geometry IO operation failures by error",
                                clamp_to_int(entry.failures_by_error[error_index]),
                                make_operation_error_labels(operation, error_codes[error_index]));
                }
            }

            for (const auto& timing : diagnostics.stage_timings)
            {
                const auto labels = make_single_label("stage", timing.name);
                add_gauge("runtime.stage.last_ms",
                          "Duration of the most recent stage execution",
                          timing.last_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          labels);
                add_gauge("runtime.stage.average_ms",
                          "Average execution time for the stage",
                          timing.average_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("stage", timing.name));
                add_gauge("runtime.stage.max_ms",
                          "Maximum execution time for the stage",
                          timing.max_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("stage", timing.name));
                add_counter("runtime.stage.sample_count",
                            "Samples recorded for the stage",
                            clamp_to_int(timing.sample_count),
                            make_single_label("stage", timing.name));
            }

            for (const auto& subsystem : diagnostics.subsystem_timings)
            {
                const auto labels = make_single_label("subsystem", subsystem.name);
                add_gauge("runtime.subsystem.last_initialize_ms",
                          "Duration of the most recent subsystem initialize()",
                          subsystem.last_initialize_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          labels);
                add_gauge("runtime.subsystem.last_tick_ms",
                          "Duration of the most recent subsystem tick()",
                          subsystem.last_tick_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("subsystem", subsystem.name));
                add_gauge("runtime.subsystem.last_shutdown_ms",
                          "Duration of the most recent subsystem shutdown()",
                          subsystem.last_shutdown_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("subsystem", subsystem.name));
                add_gauge("runtime.subsystem.max_initialize_ms",
                          "Maximum subsystem initialize() duration",
                          subsystem.max_initialize_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("subsystem", subsystem.name));
                add_gauge("runtime.subsystem.max_tick_ms",
                          "Maximum subsystem tick() duration",
                          subsystem.max_tick_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("subsystem", subsystem.name));
                add_gauge("runtime.subsystem.max_shutdown_ms",
                          "Maximum subsystem shutdown() duration",
                          subsystem.max_shutdown_ms,
                          core::telemetry::MetricUnit::Milliseconds,
                          make_single_label("subsystem", subsystem.name));
                add_counter("runtime.subsystem.initialize_count",
                            "Subsystem initialize() invocations",
                            clamp_to_int(subsystem.initialize_count),
                            make_single_label("subsystem", subsystem.name));
                add_counter("runtime.subsystem.tick_count",
                            "Subsystem tick() invocations",
                            clamp_to_int(subsystem.tick_count),
                            make_single_label("subsystem", subsystem.name));
                add_counter("runtime.subsystem.shutdown_count",
                            "Subsystem shutdown() invocations",
                            clamp_to_int(subsystem.shutdown_count),
                            make_single_label("subsystem", subsystem.name));
            }

            const auto& physics_metrics = diagnostics.physics_collision;
            add_gauge("runtime.physics.manifold_count",
                      "Active contact manifolds detected during the most recent physics step",
                      static_cast<double>(physics_metrics.manifold_count),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.physics.contact_count",
                      "Active contact points detected during the most recent physics step",
                      static_cast<double>(physics_metrics.contact_count),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.physics.max_penetration",
                      "Maximum penetration depth observed during the most recent physics step",
                      static_cast<double>(physics_metrics.max_penetration),
                      core::telemetry::MetricUnit::None);
            add_gauge("runtime.physics.solver_iterations",
                      "Constraint solver iterations executed during the most recent physics step",
                      static_cast<double>(physics_metrics.solver_iterations),
                      core::telemetry::MetricUnit::Count);

            const auto& validation = diagnostics.scene_validation.metrics;
            add_gauge("runtime.scene_validation.issue_count",
                      "Total hierarchy validation issues detected in the most recent frame",
                      static_cast<double>(validation.issue_count),
                      core::telemetry::MetricUnit::Count);

            const auto add_issue_metric = [&](std::string_view type, std::size_t value)
            {
                add_gauge("runtime.scene_validation.issues",
                          "Hierarchy validation issues by type",
                          static_cast<double>(value),
                          core::telemetry::MetricUnit::Count,
                          make_single_label("type", type));
            };

            add_issue_metric("cycle", validation.cycle_count);
            add_issue_metric("dangling_parent", validation.dangling_parent_count);
            add_issue_metric("missing_parent_hierarchy", validation.missing_parent_hierarchy_count);
            add_issue_metric("non_finite_transform", validation.non_finite_transform_count);
            add_issue_metric("transform_mismatch", validation.transform_mismatch_count);
            add_gauge("runtime.scene_validation.consecutive_failure_frames",
                      "Consecutive frames that reported hierarchy validation failures",
                      static_cast<double>(diagnostics.scene_validation_consecutive_failure_frames),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.scene_validation.max_consecutive_failure_frames",
                      "Maximum consecutive frames with hierarchy validation failures since initialization",
                      static_cast<double>(diagnostics.scene_validation_max_consecutive_failure_frames),
                      core::telemetry::MetricUnit::Count);
            add_counter("runtime.scene_validation.failure_frame_count",
                        "Total frames that reported hierarchy validation failures since initialization",
                        clamp_to_int(diagnostics.scene_validation_failure_frame_count));
            add_gauge("runtime.scene_validation.last_failure_simulation_time",
                      "Simulation time recorded for the most recent hierarchy validation failure",
                      diagnostics.last_scene_validation_failure_simulation_time,
                      core::telemetry::MetricUnit::Seconds);
            add_gauge("runtime.scene_validation.last_failure_wall_seconds",
                      "Seconds since runtime initialization when the most recent hierarchy validation failure occurred",
                      diagnostics.last_scene_validation_failure_wall_seconds,
                      core::telemetry::MetricUnit::Seconds);
            add_gauge("runtime.scene_validation.alert_threshold.warning_frames",
                      "Consecutive failure frames required to enter the warning alert state",
                      static_cast<double>(kHierarchyFailureWarningThreshold),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.scene_validation.alert_threshold.critical_frames",
                      "Consecutive failure frames required to enter the critical alert state",
                      static_cast<double>(kHierarchyFailureCriticalThreshold),
                      core::telemetry::MetricUnit::Count);
            add_gauge("runtime.scene_validation.alert_level",
                      "Hierarchy validation alert level (0 = none, 1 = warning, 2 = critical)",
                      static_cast<double>(static_cast<std::uint32_t>(diagnostics.scene_validation_alert_level)),
                      core::telemetry::MetricUnit::None);

            diagnostics.metrics = std::move(snapshot);
#if ENGINE_ENABLE_ASSETS
            refresh_handle_validation();
#endif
        }

#if ENGINE_ENABLE_ASSETS
        void refresh_handle_validation()
        {
            diagnostics.handle_validation = assets::HandleValidationTelemetry::instance().snapshot();

            auto& snapshot = diagnostics.metrics;
            const auto emit_counter = [&](std::string_view name, std::string_view description, std::int64_t value)
            {
                core::telemetry::MetricDescriptor descriptor{};
                descriptor.name.assign(name);
                descriptor.kind = core::telemetry::MetricKind::Counter;
                descriptor.unit = core::telemetry::MetricUnit::Count;
                descriptor.description.assign(description);
                descriptor.labels = {};
                const std::size_t index = snapshot.descriptors.size();
                snapshot.descriptors.push_back(std::move(descriptor));

                core::telemetry::MetricSample sample{};
                sample.descriptor_index = index;
                sample.value = value;
                snapshot.samples.push_back(std::move(sample));
            };

            for (const auto& entry : diagnostics.handle_validation)
            {
                const auto success_name = std::string{"runtime.handles."} + entry.type + ".success";
                emit_counter(success_name,
                             "Total successful handle validations for " + entry.type,
                             static_cast<std::int64_t>(entry.success_count));

                const auto failure_name = std::string{"runtime.handles."} + entry.type + ".failure";
                emit_counter(failure_name,
                             "Total failed handle validations for " + entry.type,
                             static_cast<std::int64_t>(entry.failure_count));
            }
        }
#endif

        void record_initialize_duration(Clock::duration duration)
        {
            const double ms = duration_to_ms(duration);
            diagnostics.last_initialize_ms = ms;
            diagnostics.max_initialize_ms = std::max(diagnostics.max_initialize_ms, ms);
            diagnostics.initialize_count += 1U;
            rebuild_metric_snapshot();
        }

        void record_shutdown_duration(Clock::duration duration)
        {
            const double ms = duration_to_ms(duration);
            diagnostics.last_shutdown_ms = ms;
            diagnostics.max_shutdown_ms = std::max(diagnostics.max_shutdown_ms, ms);
            diagnostics.shutdown_count += 1U;
            rebuild_metric_snapshot();
        }

        void record_tick_duration(Clock::duration duration)
        {
            const double ms = duration_to_ms(duration);
            diagnostics.last_tick_ms = ms;
            diagnostics.max_tick_ms = std::max(diagnostics.max_tick_ms, ms);
            diagnostics.tick_count += 1U;
            const double count = static_cast<double>(diagnostics.tick_count);
            if (count > 0.0)
            {
                diagnostics.average_tick_ms += (ms - diagnostics.average_tick_ms) / count;
            }
            refresh_streaming_metrics();
            rebuild_metric_snapshot();
        }

        void refresh_streaming_metrics() noexcept
        {
            diagnostics.streaming = streaming_metrics();
            diagnostics.geometry_io = io::GeometryIoTelemetry::instance().snapshot();
#if ENGINE_ENABLE_ASSETS
            const auto hot_reload_snapshot = assets::AssetHotReloadTelemetry::instance().snapshot();
            diagnostics.hot_reload.attempt_count = hot_reload_snapshot.hot_reload_attempts;
            diagnostics.hot_reload.failure_count = hot_reload_snapshot.failure_count;
            diagnostics.hot_reload.cancelled_count = hot_reload_snapshot.cancelled_count;
            diagnostics.hot_reload.rejected_count = hot_reload_snapshot.rejected_count;
            diagnostics.hot_reload.pending_count = diagnostics.streaming.streaming_pending;
            diagnostics.hot_reload.loading_count = diagnostics.streaming.streaming_loading;
            diagnostics.hot_reload.total_requests = hot_reload_snapshot.hot_reload_attempts;
            diagnostics.hot_reload.last_error = hot_reload_snapshot.last_error;
            diagnostics.hot_reload.error_hint = hot_reload_snapshot.error_hint;
            diagnostics.hot_reload.recent_failures.clear();
            diagnostics.hot_reload.recent_failures.reserve(hot_reload_snapshot.recent_failures.size());
            for (const auto& failure : hot_reload_snapshot.recent_failures)
            {
                HotReloadFailureSummary summary{};
                summary.identifier = failure.identifier;
                summary.error = failure.error;
                summary.hint = failure.hint;
                diagnostics.hot_reload.recent_failures.emplace_back(std::move(summary));
            }
#else
            diagnostics.hot_reload = {};
#endif
        }

        void record_stage_sample(std::string_view name, double duration_seconds)
        {
            RuntimeStageTiming& timing = ensure_stage_timing(std::string{name});
            const double duration_ms = duration_seconds * 1000.0;
            timing.last_ms = duration_ms;
            timing.sample_count += 1U;
            const double samples = static_cast<double>(timing.sample_count);
            if (samples > 0.0)
            {
                timing.average_ms += (duration_ms - timing.average_ms) / samples;
            }
            timing.max_ms = std::max(timing.max_ms, duration_ms);
        }

        void record_stage_timings(const compute::ExecutionReport& report)
        {
            const std::size_t count =
                std::min(report.execution_order.size(), report.kernel_durations.size());
            for (std::size_t index = 0; index < count; ++index)
            {
                record_stage_sample(report.execution_order[index], report.kernel_durations[index]);
            }
        }

        enum class SubsystemPhase
        {
            Initialize,
            Tick,
            Shutdown
        };

        void record_subsystem_event(const std::string& name, Clock::duration duration, SubsystemPhase phase)
        {
            RuntimeSubsystemTiming& timing = ensure_subsystem_timing(name);
            const double ms = duration_to_ms(duration);
            switch (phase)
            {
            case SubsystemPhase::Initialize:
                timing.last_initialize_ms = ms;
                timing.max_initialize_ms = std::max(timing.max_initialize_ms, ms);
                timing.initialize_count += 1U;
                break;
            case SubsystemPhase::Tick:
                timing.last_tick_ms = ms;
                timing.max_tick_ms = std::max(timing.max_tick_ms, ms);
                timing.tick_count += 1U;
                break;
            case SubsystemPhase::Shutdown:
                timing.last_shutdown_ms = ms;
                timing.max_shutdown_ms = std::max(timing.max_shutdown_ms, ms);
                timing.shutdown_count += 1U;
                break;
            }
        }

        [[nodiscard]] const RuntimeDiagnostics& diagnostics_view() const noexcept
        {
            return diagnostics;
        }

#if ENGINE_ENABLE_ASSETS
        template <typename Handle>
        [[nodiscard]] assets::AssetLoadFuture<Handle> make_streaming_configuration_failure(
            const assets::AssetLoadRequest& request, std::string message) const
        {
            auto channel = assets::detail::make_asset_load_channel<Handle>();
            auto& promise = channel.first;
            auto future = std::move(channel.second);
            auto error = assets::make_asset_load_error(
                assets::AssetLoadErrorCategory::ValidationError, std::move(message));
            promise.set_failed(error);
            assets::AssetStreamingTelemetry::instance().on_failure(error);
            assets::AssetStreamingTelemetry::instance().on_rejected();
            assets::AssetHotReloadTelemetry::instance().record_rejected();
            assets::AssetHotReloadTelemetry::instance().record_failure(error, request.identifier);
            return future;
        }

        [[nodiscard]] assets::AssetLoadFuture<assets::MeshHandle>
        request_mesh_asset(const assets::AssetLoadRequest& request)
        {
            if (!initialized)
            {
                spdlog::error(
                    "RuntimeHost::request_mesh_asset called before initialization (identifier='{}')",
                    request.identifier);
                return make_streaming_configuration_failure<assets::MeshHandle>(
                    request,
                    "RuntimeHost must be initialized before requesting asynchronous asset loads");
            }

            if (mesh_cache == nullptr)
            {
                spdlog::error(
                    "RuntimeHost::request_mesh_asset missing MeshCache configuration (identifier='{}')",
                    request.identifier);
                return make_streaming_configuration_failure<assets::MeshHandle>(
                    request, "RuntimeHost asset streaming requires a configured MeshCache");
            }

            return mesh_cache->load_async(request, core::threading::IoThreadPool::instance());
        }

        [[nodiscard]] assets::AssetLoadFuture<assets::PointCloudHandle>
        request_point_cloud_asset(const assets::AssetLoadRequest& request)
        {
            if (!initialized)
            {
                spdlog::error(
                    "RuntimeHost::request_point_cloud_asset called before initialization (identifier='{}')",
                    request.identifier);
                return make_streaming_configuration_failure<assets::PointCloudHandle>(
                    request,
                    "RuntimeHost must be initialized before requesting asynchronous asset loads");
            }

            if (point_cloud_cache == nullptr)
            {
                spdlog::error(
                    "RuntimeHost::request_point_cloud_asset missing PointCloudCache configuration (identifier='{}')",
                    request.identifier);
                return make_streaming_configuration_failure<assets::PointCloudHandle>(
                    request, "RuntimeHost asset streaming requires a configured PointCloudCache");
            }

            return point_cloud_cache->load_async(request, core::threading::IoThreadPool::instance());
        }

        [[nodiscard]] assets::AssetLoadState mesh_asset_state(std::string_view identifier) const
        {
            if (!initialized)
            {
                spdlog::warn(
                    "RuntimeHost::mesh_asset_state requested before initialization (identifier='{}')",
                    identifier);
                return assets::AssetLoadState::Failed;
            }

            if (mesh_cache == nullptr)
            {
                spdlog::warn(
                    "RuntimeHost::mesh_asset_state missing MeshCache configuration (identifier='{}')",
                    identifier);
                return assets::AssetLoadState::Failed;
            }

            return mesh_cache->async_state(identifier);
        }

        [[nodiscard]] assets::AssetLoadState point_cloud_asset_state(std::string_view identifier) const
        {
            if (!initialized)
            {
                spdlog::warn(
                    "RuntimeHost::point_cloud_asset_state requested before initialization (identifier='{}')",
                    identifier);
                return assets::AssetLoadState::Failed;
            }

            if (point_cloud_cache == nullptr)
            {
                spdlog::warn(
                    "RuntimeHost::point_cloud_asset_state missing PointCloudCache configuration (identifier='{}')",
                    identifier);
                return assets::AssetLoadState::Failed;
            }

            return point_cloud_cache->async_state(identifier);
        }
#endif

#if ENGINE_ENABLE_RENDERING
        void ensure_render_entity()
        {
            if (renderable_name.empty())
            {
                renderable_name = "runtime.renderable";
            }
            auto& registry = scene.registry();
            if (!render_entity.valid())
            {
                render_entity = scene.create_entity();
                const auto entt_entity = render_entity.id();
                auto& name_component = registry.emplace<scene::components::Name>(entt_entity);
                name_component.value = renderable_name;
                registry.emplace<scene::components::LocalTransform>(entt_entity);
                registry.emplace<scene::components::WorldTransform>(entt_entity);
                registry.emplace<scene::components::Hierarchy>(entt_entity);
            }
            const auto entt_entity = render_entity.id();
            registry.emplace_or_replace<rendering::components::RenderGeometry>(entt_entity, render_geometry);
            scene::systems::mark_transform_dirty(registry, entt_entity);
        }

        void set_research_rendering_options(const rendering::ResearchBaselineOptions& options) noexcept
        {
            research_options = options;
#if ENGINE_ENABLE_RENDERING
            shared_research_rendering_options() = options;
#endif
            auto& telemetry = rendering::ResearchBaselineTelemetry::instance();
            telemetry.set_shading_mode(options.shading_mode);
            telemetry.set_overlays(options);
        }
#endif

        void reset_state()
        {
            initialized = false;
            simulation_time = 0.0;
            runtime_start_time = Clock::now();
            controller = dependencies.controller;
            pose = animation::evaluate_controller(controller);
            mesh = dependencies.mesh;
            binding = dependencies.binding;
            binding.resize_vertices(mesh.rest_positions.size());
            joint_global_transforms.resize(binding.joints.size());
            skinning_transforms.resize(binding.joints.size());
            geometry::recompute_vertex_normals(mesh);
            geometry::update_bounds(mesh);
            world = dependencies.world;
            validate_dispatcher_factory();
            last_report = {};
            body_positions.clear();
            joint_names.clear();
            scene_nodes.clear();
            joint_entities.clear();
            diagnostics = {};
            stage_lookup.clear();
            subsystem_lookup.clear();
            scene = scene::Scene{scene_name()};
#if ENGINE_ENABLE_RENDERING
            render_entity = scene::Entity{};
            render_geometry = dependencies.render_geometry;
            if (!dependencies.renderable_name.empty())
            {
                renderable_name = dependencies.renderable_name;
            }
#endif
#if ENGINE_ENABLE_ASSETS
            mesh_cache = dependencies.asset_streaming.mesh_cache;
            point_cloud_cache = dependencies.asset_streaming.point_cloud_cache;
#endif
            refresh_physics_metrics();
            rebuild_subsystem_cache();
            auto plan_result = build_default_loop_plan();
            if (!plan_result)
            {
                spdlog::error(
                    "Failed to build runtime loop plan: {}",
                    plan_result.error().message());
                loop_plan = {};
            }
            else
            {
                loop_plan = std::move(plan_result).value();
            }
        }

        RuntimeResult<RuntimeLoopPlan> build_default_loop_plan()
        {
            RuntimeLoopBuilder builder{};
            auto stage_result = builder.add_stage(
                "animation.evaluate",
                RuntimeLoopPhase::Simulation,
                [this](double dt)
                {
                    engine::animation::advance_controller(controller, dt);
                    pose = engine::animation::evaluate_controller(controller);
                });
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "physics.accumulate",
                RuntimeLoopPhase::Simulation,
                [this](double)
                {
                    engine::physics::clear_forces(world);
                    if (!pose.joints.empty() && engine::physics::body_count(world) > 0)
                    {
                        if (const auto* root = pose.find("root"))
                        {
                            const math::vec3 drive = root->translation * 4.0F;
                            engine::physics::apply_force(world, 0, drive);
                        }
                    }
                },
                {"animation.evaluate"});
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "physics.integrate",
                RuntimeLoopPhase::Simulation,
                [this](double dt)
                {
                    engine::physics::integrate(world, dt);
                    refresh_body_positions();
                },
                {"physics.accumulate"});
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "geometry.deform",
                RuntimeLoopPhase::Simulation,
                [this](double)
                {
                    math::vec3 root_translation{0.0F, 0.0F, 0.0F};
                    if (!body_positions.empty())
                    {
                        root_translation = body_positions.front();
                    }

                    if (!animation::skinning::validate_binding(binding) || binding.joints.empty())
                    {
                        math::vec3 translation = root_translation;
                        if (const auto* root_pose = pose.find("root"))
                        {
                            translation += root_pose->translation;
                        }
                        engine::geometry::apply_uniform_translation(mesh, translation);
                        engine::geometry::recompute_vertex_normals(mesh);
                        return;
                    }

                    if (joint_global_transforms.size() != binding.joints.size())
                    {
                        joint_global_transforms.resize(binding.joints.size());
                    }
                    if (skinning_transforms.size() != binding.joints.size())
                    {
                        skinning_transforms.resize(binding.joints.size());
                    }

                    animation::skinning::build_global_joint_transforms(
                        binding, pose, joint_global_transforms, root_translation);
                    animation::skinning::build_skinning_transforms(
                        binding, joint_global_transforms, skinning_transforms);
                    engine::geometry::deform::apply_linear_blend_skinning(
                        binding, skinning_transforms, mesh);
                },
                {"physics.integrate"});
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "geometry.finalize",
                RuntimeLoopPhase::Simulation,
                [this](double dt)
                {
                    engine::geometry::update_bounds(mesh);
                    refresh_joint_names();
                    math::vec3 translation{0.0F, 0.0F, 0.0F};
                    if (!body_positions.empty())
                    {
                        translation = body_positions.front();
                    }
                    synchronize_scene_graph(translation, dt);
                    simulation_time += dt;
                },
                {"geometry.deform"});
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "runtime.plugins",
                RuntimeLoopPhase::Simulation,
                [this](double dt)
                {
                    const engine::core::plugin::SubsystemUpdateContext update_context{dt};
                    for (const auto& plugin : dependencies.subsystem_plugins)
                    {
                        if (plugin == nullptr)
                        {
                            continue;
                        }
                        const std::string name{plugin->name()};
                        const auto start = Clock::now();
                        plugin->tick(update_context);
                        const auto duration = Clock::now() - start;
                        record_subsystem_event(name, duration, SubsystemPhase::Tick);
                    }
                },
                {"geometry.finalize"},
                false);
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            stage_result = builder.add_stage(
                "diagnostics.refresh",
                RuntimeLoopPhase::Diagnostics,
                [this](double)
                {
                    engine::physics::update_contact_manifolds(world);
                    refresh_physics_metrics();
                    update_animation_telemetry(last_report);
                },
                {"runtime.plugins"},
                false);
            if (!stage_result)
            {
                return RuntimeResult<RuntimeLoopPlan>{stage_result.error()};
            }

            return builder.build();
        }

        void configure(RuntimeHostDependencies deps)
        {
            if (initialized)
            {
                throw std::runtime_error("RuntimeHost cannot be configured while initialized");
            }

            throw_if_invalid_dependencies(deps);
            dependencies = std::move(deps);
            reset_state();
        }

        [[nodiscard]] std::string_view runtime_name_view() const noexcept
        {
            if (dependencies.scene_name.empty())
            {
                return std::string_view{"runtime.scene"};
            }
            return dependencies.scene_name;
        }

        [[nodiscard]] std::string scene_name() const
        {
            return std::string{runtime_name_view()};
        }

        void ensure_default_world()
        {
            if (engine::physics::body_count(world) == 0U)
            {
                engine::physics::RigidBody body{};
                body.mass = 2.0F;
                body.position = engine::math::vec3{0.0F, 0.25F, 0.0F};
                MAYBE_UNUSED_CONST_AUTO id = engine::physics::add_body(world, body);
                (void)id;
            }
        }

        void refresh_body_positions()
        {
            body_positions.clear();
            const auto count = engine::physics::body_count(world);
            body_positions.reserve(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                body_positions.push_back(engine::physics::body_at(world, index).position);
            }
        }

        void refresh_joint_names()
        {
            joint_names.clear();
            joint_names.reserve(pose.joints.size());
            for (const auto& entry : pose.joints)
            {
                joint_names.push_back(entry.first);
            }
        }

        void refresh_physics_metrics() noexcept
        {
            diagnostics.physics_collision = engine::physics::collision_telemetry(world);
        }

        void rebuild_scene_entities()
        {
            scene = scene::Scene{scene_name()};
            joint_entities.clear();
            joint_entities.reserve(pose.joints.size());

            for (const auto& entry : pose.joints)
            {
                auto entity = scene.create_entity();
                auto& name_component = entity.emplace<scene::components::Name>();
                name_component.value = entry.first;
                entity.emplace<scene::components::LocalTransform>();
                entity.emplace<scene::components::WorldTransform>();
                entity.emplace<scene::components::Hierarchy>();
                scene::systems::mark_transform_dirty(scene.registry(), entity.id());
                joint_entities.push_back(entity);
            }
#if ENGINE_ENABLE_RENDERING
            ensure_render_entity();
#endif
        }

        void synchronize_scene_graph(const math::vec3& body_translation, double frame_dt)
        {
            if (joint_entities.size() != pose.joints.size())
            {
                rebuild_scene_entities();
            }

            auto& registry = scene.registry();
            scene_nodes.clear();
#if ENGINE_ENABLE_RENDERING
            ensure_render_entity();
#endif

            for (std::size_t index = 0; index < joint_entities.size() && index < pose.joints.size(); ++index)
            {
                auto entity = joint_entities[index];
                if (!entity.valid())
                {
                    continue;
                }

                const auto entt_entity = entity.id();
                auto& local = registry.get<scene::components::LocalTransform>(entt_entity);
                const auto& pose_entry = pose.joints[index];
                local.value.scale = pose_entry.second.scale;
                local.value.rotation = pose_entry.second.rotation;
                local.value.translation = pose_entry.second.translation;
                if (pose_entry.first == "root")
                {
                    local.value.translation += body_translation;
                }

                auto* name_component = registry.try_get<scene::components::Name>(entt_entity);
                if (name_component == nullptr)
                {
                    name_component = &registry.emplace<scene::components::Name>(entt_entity);
                }
                name_component->value = pose_entry.first;

                scene::systems::mark_transform_dirty(registry, entt_entity);
            }

#if ENGINE_ENABLE_RENDERING
            if (render_entity.valid())
            {
                const auto entt_entity = render_entity.id();
                auto* local = registry.try_get<scene::components::LocalTransform>(entt_entity);
                if (local == nullptr)
                {
                    local = &registry.emplace<scene::components::LocalTransform>(entt_entity);
                }
                math::Transform<float> transform = math::Transform<float>::Identity();
                if (const auto* root = pose.find("root"))
                {
                    transform.scale = root->scale;
                    transform.rotation = root->rotation;
                    transform.translation = root->translation + body_translation;
                }
                else
                {
                    transform.translation = body_translation;
                }
                local->value = transform;
                scene::systems::mark_transform_dirty(registry, entt_entity);
            }
#endif

            scene::systems::propagate_transforms(registry);

            diagnostics.scene_validation = scene::validation::validate_hierarchy(scene);
            DiagnosticsBridge::instance().publish_hierarchy_report(
                diagnostics.scene_validation,
                simulation_time);
            const auto now = Clock::now();
            const double wall_seconds = std::chrono::duration<double>(now - runtime_start_time).count();
            const double frame_simulation_time = simulation_time + frame_dt;
            detail::update_scene_validation_alert_state(
                diagnostics,
                diagnostics.scene_validation,
                frame_simulation_time,
                wall_seconds);

            for (const auto& entity : joint_entities)
            {
                if (!entity.valid())
                {
                    continue;
                }

                const auto entt_entity = entity.id();
                const auto* name_component = registry.try_get<scene::components::Name>(entt_entity);
                const auto* world_transform = registry.try_get<scene::components::WorldTransform>(entt_entity);
                if (name_component == nullptr || world_transform == nullptr)
                {
                    continue;
                }

                runtime_frame_state::scene_node_state node{};
                node.name = name_component->value;
                node.transform = world_transform->value;
                scene_nodes.push_back(std::move(node));
            }
#if ENGINE_ENABLE_RENDERING
            if (render_entity.valid())
            {
                const auto entt_entity = render_entity.id();
                const auto* name_component = registry.try_get<scene::components::Name>(entt_entity);
                const auto* world_transform = registry.try_get<scene::components::WorldTransform>(entt_entity);
                if (name_component != nullptr && world_transform != nullptr)
                {
                    runtime_frame_state::scene_node_state node{};
                    node.name = name_component->value;
                    node.transform = world_transform->value;
                    scene_nodes.push_back(std::move(node));
                }
            }
#endif
        }

        void initialize()
        {
            if (initialized)
            {
                return;
            }

            const auto initialize_start = Clock::now();
            core::threading::IoThreadPool::instance().configure(dependencies.streaming_config);
            reset_state();
            ensure_default_world();
            refresh_body_positions();
            refresh_joint_names();
            rebuild_scene_entities();
            const math::vec3 translation = body_positions.empty()
                                               ? math::vec3{0.0F, 0.0F, 0.0F}
                                               : body_positions.front();
            synchronize_scene_graph(translation, 0.0);
            const engine::core::plugin::SubsystemLifecycleContext lifecycle{runtime_name_view()};
            std::vector<std::shared_ptr<core::plugin::ISubsystemInterface>> started_plugins{};
            started_plugins.reserve(dependencies.subsystem_plugins.size());
            try
            {
                for (const auto& plugin : dependencies.subsystem_plugins)
                {
                    if (plugin == nullptr)
                    {
                        continue;
                    }

                    const std::string name{plugin->name()};
                    const auto start = Clock::now();
                    try
                    {
                        plugin->initialize(lifecycle);
                        started_plugins.push_back(plugin);
                        const auto duration = Clock::now() - start;
                        record_subsystem_event(name, duration, SubsystemPhase::Initialize);
                    }
                    catch (const std::exception& exception)
                    {
                        const auto duration = Clock::now() - start;
                        record_initialize_failure(name,
                                                  classify_exception(exception),
                                                  exception.what(),
                                                  duration_to_ms(duration));
                        throw;
                    }
                    catch (...)
                    {
                        const auto duration = Clock::now() - start;
                        record_initialize_failure(
                            name,
                            "non_std_exception",
                            "Subsystem initialize threw a non-standard exception",
                            duration_to_ms(duration));
                        throw;
                    }
                }
                initialized = true;
            }
            catch (...)
            {
                for (auto it = started_plugins.rbegin(); it != started_plugins.rend(); ++it)
                {
                    if (*it == nullptr)
                    {
                        continue;
                    }

                    const std::string name{(*it)->name()};
                    const auto start = Clock::now();
                    (*it)->shutdown(lifecycle);
                    const auto duration = Clock::now() - start;
                    record_subsystem_event(name, duration, SubsystemPhase::Shutdown);
                }

                core::threading::IoThreadPool::instance().shutdown();
                throw;
            }

            record_initialize_duration(Clock::now() - initialize_start);
            runtime_start_time = Clock::now();
            refresh_streaming_metrics();
            rebuild_metric_snapshot();
        }

        void shutdown() noexcept
        {
            if (!initialized)
            {
                return;
            }

            const auto shutdown_start = Clock::now();
            initialized = false;
            const engine::core::plugin::SubsystemLifecycleContext lifecycle{runtime_name_view()};
            for (auto it = dependencies.subsystem_plugins.rbegin(); it != dependencies.subsystem_plugins.rend(); ++it)
            {
                if (*it != nullptr)
                {
                    const std::string name{(*it)->name()};
                    const auto start = Clock::now();
                    (*it)->shutdown(lifecycle);
                    const auto duration = Clock::now() - start;
                    record_subsystem_event(name, duration, SubsystemPhase::Shutdown);
                }
            }
            core::threading::IoThreadPool::instance().shutdown();
            last_report.execution_order.clear();
            last_report.kernel_durations.clear();
            scene = scene::Scene{};
            joint_entities.clear();
            scene_nodes.clear();
            body_positions.clear();
            joint_names.clear();
            reset_state();
            record_shutdown_duration(Clock::now() - shutdown_start);
            refresh_streaming_metrics();
            rebuild_metric_snapshot();
        }

        runtime_frame_state tick(double dt)
        {
            if (!initialized)
            {
                throw std::runtime_error("RuntimeHost must be initialized before tick()");
            }

            const auto tick_start = Clock::now();
            last_report.execution_order.clear();
            last_report.kernel_durations.clear();
            last_report.dependency_graph.nodes.clear();
            last_report.clock_domain = compute::TimingDomain::Cpu;
            last_report.clock_name = "runtime.loop";

            const auto& stages = loop_plan.stages();
            std::unordered_map<std::string, std::size_t> stage_index_map{};
            stage_index_map.reserve(stages.size());
            for (std::size_t index = 0; index < stages.size(); ++index)
            {
                stage_index_map.emplace(stages[index].name, index);
            }

            std::vector<std::size_t> report_indices(
                stages.size(), std::numeric_limits<std::size_t>::max());

            for (std::size_t index = 0; index < stages.size(); ++index)
            {
                const auto& stage = stages[index];
                const auto stage_start = Clock::now();
                if (stage.function)
                {
                    stage.function(dt);
                }
                const auto stage_duration = Clock::now() - stage_start;
                const double seconds = std::chrono::duration<double>(stage_duration).count();
                record_stage_sample(stage.name, seconds);
                if (stage.record_in_execution_report)
                {
                    const std::size_t report_index = last_report.execution_order.size();
                    report_indices[index] = report_index;
                    last_report.execution_order.push_back(stage.name);
                    last_report.kernel_durations.push_back(seconds);
                }
            }

            compute::DependencyGraph graph{};
            graph.nodes.reserve(last_report.execution_order.size());
            for (std::size_t index = 0; index < stages.size(); ++index)
            {
                const auto& stage = stages[index];
                if (!stage.record_in_execution_report)
                {
                    continue;
                }

                compute::DependencyGraph::Node node{};
                node.name = stage.name;
                for (const auto& dependency : stage.dependencies)
                {
                    const auto it = stage_index_map.find(dependency);
                    if (it == stage_index_map.end())
                    {
                        continue;
                    }
                    const std::size_t dependency_index = it->second;
                    const std::size_t report_index = report_indices[dependency_index];
                    if (report_index != std::numeric_limits<std::size_t>::max())
                    {
                        node.dependencies.push_back(report_index);
                    }
                }
                graph.nodes.push_back(std::move(node));
            }
            last_report.dependency_graph = std::move(graph);

            record_tick_duration(Clock::now() - tick_start);

            runtime_frame_state frame{};
            frame.simulation_time = simulation_time;
            frame.pose = pose;
            frame.bounds = mesh.bounds;
            frame.body_positions = body_positions;
            frame.dispatch_report = last_report;
            frame.scene_nodes = scene_nodes;
            return frame;
        }

        const geometry::SurfaceMesh& current_mesh() const
        {
            if (!initialized)
            {
                throw std::runtime_error("RuntimeHost must be initialized before accessing the mesh");
            }
            return mesh;
        }
    };

    RuntimeHost::RuntimeHost() : RuntimeHost(make_default_dependencies())
    {
    }

    RuntimeHost::RuntimeHost(RuntimeHostDependencies dependencies)
        : impl_(std::make_unique<Impl>(std::move(dependencies)))
    {
    }

    RuntimeHost::RuntimeHost(RuntimeHost&&) noexcept = default;

    RuntimeHost& RuntimeHost::operator=(RuntimeHost&&) noexcept = default;

    RuntimeHost::~RuntimeHost() = default;

    void RuntimeHost::configure(RuntimeHostDependencies dependencies)
    {
        impl_->configure(std::move(dependencies));
    }

    void RuntimeHost::initialize()
    {
        impl_->initialize();
    }

    void RuntimeHost::shutdown() noexcept
    {
        impl_->shutdown();
    }

    bool RuntimeHost::is_initialized() const noexcept
    {
        return impl_->initialized;
    }

    runtime_frame_state RuntimeHost::tick(double dt)
    {
        return impl_->tick(dt);
    }

    const geometry::SurfaceMesh& RuntimeHost::current_mesh() const
    {
        return impl_->current_mesh();
    }

    const animation::AnimationRigPose& RuntimeHost::current_pose() const
    {
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before accessing the pose");
        }
        return impl_->pose;
    }

    const std::vector<math::vec3>& RuntimeHost::body_positions() const
    {
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before accessing body positions");
        }
        return impl_->body_positions;
    }

    const std::vector<std::string>& RuntimeHost::joint_names() const
    {
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before accessing joint names");
        }
        return impl_->joint_names;
    }

    const compute::ExecutionReport& RuntimeHost::last_dispatch_report() const
    {
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before accessing dispatch reports");
        }
        return impl_->last_report;
    }

    const std::vector<runtime_frame_state::scene_node_state>& RuntimeHost::scene_nodes() const
    {
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before accessing scene nodes");
        }
        return impl_->scene_nodes;
    }

    double RuntimeHost::simulation_time() const noexcept
    {
        return impl_->simulation_time;
    }

    std::span<const std::string_view> RuntimeHost::subsystem_names() const noexcept
    {
        return impl_->subsystem_names;
    }

    const RuntimeDiagnostics& RuntimeHost::diagnostics() const noexcept
    {
        return impl_->diagnostics_view();
    }

#if ENGINE_ENABLE_ASSETS
    assets::AssetLoadFuture<assets::MeshHandle>
    RuntimeHost::request_mesh_asset(const assets::AssetLoadRequest& request)
    {
        return impl_->request_mesh_asset(request);
    }

    assets::AssetLoadFuture<assets::PointCloudHandle>
    RuntimeHost::request_point_cloud_asset(const assets::AssetLoadRequest& request)
    {
        return impl_->request_point_cloud_asset(request);
    }

    assets::AssetLoadState RuntimeHost::mesh_asset_state(std::string_view identifier) const
    {
        return impl_->mesh_asset_state(identifier);
    }

    assets::AssetLoadState RuntimeHost::point_cloud_asset_state(std::string_view identifier) const
    {
        return impl_->point_cloud_asset_state(identifier);
    }
#endif

#if ENGINE_ENABLE_RENDERING
    void RuntimeHost::submit_render_graph(RenderSubmissionContext& context)
    {
        if (impl_ == nullptr)
        {
            throw std::runtime_error("RuntimeHost has no implementation");
        }
        if (!impl_->initialized)
        {
            throw std::runtime_error("RuntimeHost must be initialized before submitting a render graph");
        }
        impl_->ensure_render_entity();
        rendering::ForwardPipeline* pipeline = context.pipeline;
        if (pipeline == nullptr)
        {
            pipeline = &impl_->forward_pipeline;
        }
        pipeline->render(impl_->scene, context);
        impl_->diagnostics.frame_graph_serialization = context.frame_graph.serialize();
        const auto& events = context.frame_graph.resource_events();
        impl_->diagnostics.frame_graph_events.assign(events.begin(), events.end());
    }

    void RuntimeHost::configure_research_rendering(const rendering::ResearchBaselineOptions& options) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }
        impl_->set_research_rendering_options(options);
    }
#endif

    namespace
    {
        engine::runtime::RuntimeHost& global_host()
        {
            static engine::runtime::RuntimeHost host{};
            return host;
        }

        engine::runtime::RuntimeHost& ensure_initialized_host()
        {
            auto& host = global_host();
            if (!host.is_initialized())
            {
                host.initialize();
            }
            return host;
        }
    } // namespace

    void configure_with_default_subsystems()
    {
        global_host().configure(make_default_dependencies());
    }

    void configure_with_default_subsystems(std::span<const std::string_view> enabled_subsystems)
    {
        auto dependencies = make_default_dependencies();
        dependencies.enabled_subsystems.assign(enabled_subsystems.begin(), enabled_subsystems.end());
        dependencies.subsystem_plugins.clear();
        global_host().configure(std::move(dependencies));
    }

    std::vector<std::string> default_subsystem_names()
    {
        const auto registry = make_default_subsystem_registry();
        const auto registered = registry.registered_names();
        std::vector<std::string> names{};
        names.reserve(registered.size());
        for (const auto name : registered)
        {
            names.emplace_back(name);
        }
        return names;
    }

    StreamingMetrics streaming_metrics() noexcept
    {
        StreamingMetrics metrics{};
        const auto stats = core::threading::IoThreadPool::instance().statistics();
        metrics.worker_count = stats.configured_workers;
        metrics.queue_capacity = stats.queue_capacity;
        metrics.pending_tasks = stats.pending_tasks;
        metrics.active_workers = stats.active_workers;
        metrics.total_enqueued = stats.total_enqueued;
        metrics.total_executed = stats.total_executed;

#if ENGINE_ENABLE_ASSETS
        const auto snapshot = assets::AssetStreamingTelemetry::instance().snapshot();
        metrics.streaming_pending = snapshot.pending;
        metrics.streaming_loading = snapshot.loading;
        metrics.streaming_total_requests = snapshot.total_requests;
        metrics.streaming_total_completed = snapshot.total_completed;
        metrics.streaming_total_failed = snapshot.total_failed;
        metrics.streaming_total_cancelled = snapshot.total_cancelled;
        metrics.streaming_total_rejected = snapshot.total_rejected;
        const auto geometry_errors = engine::io::geometry_io_error_codes();
        for (std::size_t index = 0; index < geometry_errors.size(); ++index)
        {
            metrics.streaming_geometry_failures[index] = snapshot.geometry_failures[index];
            metrics.streaming_geometry_failure_labels[index] = io::to_string(geometry_errors[index]);
        }
#endif
        return metrics;
    }

    const RuntimeDiagnostics& diagnostics() noexcept
    {
        return global_host().diagnostics();
    }

#if ENGINE_ENABLE_ASSETS
    assets::AssetLoadFuture<assets::MeshHandle>
    request_mesh_asset(const assets::AssetLoadRequest& request)
    {
        auto& host = ensure_initialized_host();
        return host.request_mesh_asset(request);
    }

    assets::AssetLoadFuture<assets::PointCloudHandle>
    request_point_cloud_asset(const assets::AssetLoadRequest& request)
    {
        auto& host = ensure_initialized_host();
        return host.request_point_cloud_asset(request);
    }

    assets::AssetLoadState mesh_asset_state(std::string_view identifier)
    {
        return ensure_initialized_host().mesh_asset_state(identifier);
    }

    assets::AssetLoadState point_cloud_asset_state(std::string_view identifier)
    {
        return ensure_initialized_host().point_cloud_asset_state(identifier);
    }
#endif

    std::string_view module_name() noexcept
    {
        return "runtime";
    }

    std::size_t module_count() noexcept
    {
        return global_host().subsystem_names().size();
    }

    std::string_view module_name_at(std::size_t index) noexcept
    {
        const auto names = global_host().subsystem_names();
        if (index >= names.size())
        {
            return {};
        }
        return names[index];
    }

    void initialize()
    {
        global_host().initialize();
    }

    void shutdown()
    {
        global_host().shutdown();
    }

    void configure(RuntimeHostDependencies dependencies)
    {
        global_host().configure(std::move(dependencies));
    }

    runtime_frame_state tick(double dt)
    {
        auto& host = ensure_initialized_host();
        return host.tick(dt);
    }

#if ENGINE_ENABLE_RENDERING
    void submit_render_graph(RuntimeHost::RenderSubmissionContext& context)
    {
        auto& host = ensure_initialized_host();
        host.submit_render_graph(context);
    }

    void configure_research_rendering(const rendering::ResearchBaselineOptions& options) noexcept
    {
        auto& host = global_host();
        host.configure_research_rendering(options);
    }
#endif

    const geometry::SurfaceMesh& current_mesh()
    {
        return ensure_initialized_host().current_mesh();
    }

    bool is_initialized() noexcept
    {
        return global_host().is_initialized();
    }

    const animation::AnimationRigPose& current_pose()
    {
        return ensure_initialized_host().current_pose();
    }

    const std::vector<math::vec3>& body_positions()
    {
        return ensure_initialized_host().body_positions();
    }

    const std::vector<std::string>& joint_names()
    {
        return ensure_initialized_host().joint_names();
    }

    const compute::ExecutionReport& last_dispatch_report()
    {
        return ensure_initialized_host().last_dispatch_report();
    }

    const std::vector<runtime_frame_state::scene_node_state>& scene_nodes()
    {
        return ensure_initialized_host().scene_nodes();
    }

    double simulation_time() noexcept
    {
        return global_host().simulation_time();
    }

    extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept
    {
        return engine::runtime::module_name().data();
    }

    extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept
    {
        return engine::runtime::module_count();
    }

    extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept
    {
        const auto name = engine::runtime::module_name_at(index);
        return name.empty() ? nullptr : name.data();
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_default_modules() noexcept
    {
        try
        {
            engine::runtime::configure_with_default_subsystems();
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_modules(
        const char* const* module_names,
        std::size_t count) noexcept
    {
        try
        {
            if (module_names == nullptr)
            {
                engine::runtime::configure_with_default_subsystems();
                return;
            }

            std::vector<std::string_view> enabled{};
            enabled.reserve(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                const char* name = module_names[index];
                if (name == nullptr || name[0] == '\0')
                {
                    continue;
                }
                enabled.emplace_back(name);
            }

            engine::runtime::configure_with_default_subsystems(enabled);
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_initialize() noexcept
    {
        engine::runtime::initialize();
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_shutdown() noexcept
    {
        engine::runtime::shutdown();
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_tick(double dt) noexcept
    {
        try
        {
            static_cast<void>(engine::runtime::tick(dt));
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_body_count() noexcept
    {
        try
        {
            return engine::runtime::body_positions().size();
        }
        catch (...)
        {
            return 0U;
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_body_position(std::size_t index, float* out_position) noexcept
    {
        try
        {
            const auto& positions = engine::runtime::body_positions();
            if (!out_position || index >= positions.size())
            {
                return;
            }
            const auto& value = positions[index];
            out_position[0] = value[0];
            out_position[1] = value[1];
            out_position[2] = value[2];
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_joint_count() noexcept
    {
        try
        {
            return engine::runtime::joint_names().size();
        }
        catch (...)
        {
            return 0U;
        }
    }

    extern "C" ENGINE_RUNTIME_API const char* engine_runtime_joint_name(std::size_t index) noexcept
    {
        try
        {
            const auto& names = engine::runtime::joint_names();
            if (index >= names.size())
            {
                return nullptr;
            }
            return names[index].c_str();
        }
        catch (...)
        {
            return nullptr;
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_joint_translation(
        std::size_t index, float* out_translation) noexcept
    {
        try
        {
            if (!out_translation)
            {
                return;
            }
            const auto& pose = engine::runtime::current_pose();
            if (index >= pose.joints.size())
            {
                return;
            }
            const auto& pose_entry = pose.joints[index];
            out_translation[0] = pose_entry.second.translation[0];
            out_translation[1] = pose_entry.second.translation[1];
            out_translation[2] = pose_entry.second.translation[2];
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_mesh_bounds(float* out_min, float* out_max) noexcept
    {
        try
        {
            if (!out_min || !out_max)
            {
                return;
            }
            const auto& bounds = engine::runtime::current_mesh().bounds;
            out_min[0] = bounds.min[0];
            out_min[1] = bounds.min[1];
            out_min[2] = bounds.min[2];
            out_max[0] = bounds.max[0];
            out_max[1] = bounds.max[1];
            out_max[2] = bounds.max[2];
        }
        catch (...)
        {
        }
    }

    extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_dispatch_count() noexcept
    {
        try
        {
            return engine::runtime::last_dispatch_report().execution_order.size();
        }
        catch (...)
        {
            return 0U;
        }
    }

    extern "C" ENGINE_RUNTIME_API const char* engine_runtime_dispatch_name(std::size_t index) noexcept
    {
        try
        {
            const auto& report = engine::runtime::last_dispatch_report();
            if (index >= report.execution_order.size())
            {
                return nullptr;
            }
            return report.execution_order[index].c_str();
        }
        catch (...)
        {
            return nullptr;
        }
    }

    extern "C" ENGINE_RUNTIME_API double engine_runtime_dispatch_duration(std::size_t index) noexcept
    {
        try
        {
            const auto& report = engine::runtime::last_dispatch_report();
            if (index >= report.kernel_durations.size())
            {
                return 0.0;
            }
            return report.kernel_durations[index];
        }
        catch (...)
        {
            return 0.0;
        }
    }

    extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_scene_node_count() noexcept
    {
        try
        {
            return engine::runtime::scene_nodes().size();
        }
        catch (...)
        {
            return 0U;
        }
    }

    extern "C" ENGINE_RUNTIME_API const char* engine_runtime_scene_node_name(std::size_t index) noexcept
    {
        try
        {
            const auto& nodes = engine::runtime::scene_nodes();
            if (index >= nodes.size())
            {
                return nullptr;
            }
            return nodes[index].name.c_str();
        }
        catch (...)
        {
            return nullptr;
        }
    }

    extern "C" ENGINE_RUNTIME_API void engine_runtime_scene_node_transform(
        std::size_t index,
        float* out_scale,
        float* out_rotation,
        float* out_translation) noexcept
    {
        try
        {
            const auto& nodes = engine::runtime::scene_nodes();
            if (index >= nodes.size())
            {
                return;
            }
            const auto& node = nodes[index];
            if (out_scale != nullptr)
            {
                out_scale[0] = node.transform.scale[0];
                out_scale[1] = node.transform.scale[1];
                out_scale[2] = node.transform.scale[2];
            }
            if (out_rotation != nullptr)
            {
                out_rotation[0] = node.transform.rotation.w;
                out_rotation[1] = node.transform.rotation.x;
                out_rotation[2] = node.transform.rotation.y;
                out_rotation[3] = node.transform.rotation.z;
            }
            if (out_translation != nullptr)
            {
                out_translation[0] = node.transform.translation[0];
                out_translation[1] = node.transform.translation[1];
                out_translation[2] = node.transform.translation[2];
            }
        }
        catch (...)
        {
        }
    }
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_research_rendering(
    const struct ::engine_runtime_research_rendering_options* options) noexcept
{
#if ENGINE_ENABLE_RENDERING
    engine::rendering::ResearchBaselineOptions baseline{};
    if (options != nullptr)
    {
        baseline.width = options->width;
        baseline.height = options->height;
        if (options->shading_mode == ENGINE_RUNTIME_RESEARCH_SHADING_MODE_FORWARD)
        {
            baseline.shading_mode = engine::rendering::ResearchShadingMode::Forward;
        }
        else
        {
            baseline.shading_mode = engine::rendering::ResearchShadingMode::Deferred;
        }
        baseline.enable_normals_overlay = options->overlay_normals != 0U;
        baseline.enable_uv_overlay = options->overlay_uv != 0U;
        baseline.enable_material_overlay = options->overlay_material != 0U;
        baseline.enable_light_volume_overlay = options->overlay_light_volume != 0U;
    }
    try
    {
        engine::runtime::configure_research_rendering(baseline);
    }
    catch (...)
    {
    }
#else
    (void)options;
#endif
}

extern "C" ENGINE_RUNTIME_API std::size_t

engine_runtime_streaming_geometry_failure_capacity_value() noexcept
{
    return engine::io::geometry_io_error_count();
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_streaming_metrics(
    struct ::engine_runtime_streaming_metrics* out_metrics) noexcept
{
    if (out_metrics == nullptr)
    {
        return;
    }

    const auto metrics = engine::runtime::streaming_metrics();
    out_metrics->worker_count = metrics.worker_count;
    out_metrics->queue_capacity = metrics.queue_capacity;
    out_metrics->pending_tasks = metrics.pending_tasks;
    out_metrics->active_workers = metrics.active_workers;
    out_metrics->total_enqueued = metrics.total_enqueued;
    out_metrics->total_executed = metrics.total_executed;
    out_metrics->streaming_pending = metrics.streaming_pending;
    out_metrics->streaming_loading = metrics.streaming_loading;
    out_metrics->streaming_total_requests = metrics.streaming_total_requests;
    out_metrics->streaming_total_completed = metrics.streaming_total_completed;
    out_metrics->streaming_total_failed = metrics.streaming_total_failed;
    out_metrics->streaming_total_cancelled = metrics.streaming_total_cancelled;
    out_metrics->streaming_total_rejected = metrics.streaming_total_rejected;
    const auto geometry_errors = engine::io::geometry_io_error_codes();
    out_metrics->streaming_geometry_failure_count =
        static_cast<std::uint32_t>(geometry_errors.size());
    for (std::size_t index = 0; index < geometry_errors.size(); ++index)
    {
        out_metrics->streaming_geometry_failures[index] = metrics.streaming_geometry_failures[index];
        out_metrics->streaming_geometry_failure_labels[index] = engine::io::to_string(geometry_errors[index]).data();
    }
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_diagnostic_streaming_metrics(
    struct ::engine_runtime_streaming_metrics* out_metrics) noexcept
{
    if (out_metrics == nullptr)
    {
        return;
    }

    const auto& metrics = engine::runtime::diagnostics().streaming;
    out_metrics->worker_count = metrics.worker_count;
    out_metrics->queue_capacity = metrics.queue_capacity;
    out_metrics->pending_tasks = metrics.pending_tasks;
    out_metrics->active_workers = metrics.active_workers;
    out_metrics->total_enqueued = metrics.total_enqueued;
    out_metrics->total_executed = metrics.total_executed;
    out_metrics->streaming_pending = metrics.streaming_pending;
    out_metrics->streaming_loading = metrics.streaming_loading;
    out_metrics->streaming_total_requests = metrics.streaming_total_requests;
    out_metrics->streaming_total_completed = metrics.streaming_total_completed;
    out_metrics->streaming_total_failed = metrics.streaming_total_failed;
    out_metrics->streaming_total_cancelled = metrics.streaming_total_cancelled;
    out_metrics->streaming_total_rejected = metrics.streaming_total_rejected;
    const auto geometry_errors = engine::io::geometry_io_error_codes();
    out_metrics->streaming_geometry_failure_count =
        static_cast<std::uint32_t>(geometry_errors.size());
    for (std::size_t index = 0; index < geometry_errors.size(); ++index)
    {
        out_metrics->streaming_geometry_failures[index] = metrics.streaming_geometry_failures[index];
        out_metrics->streaming_geometry_failure_labels[index] = engine::io::to_string(geometry_errors[index]).data();
    }
}

extern "C" ENGINE_RUNTIME_API void engine_runtime_diagnostic_hot_reload_metrics(
    struct ::engine_runtime_hot_reload_metrics* out_metrics) noexcept
{
    if (out_metrics == nullptr)
    {
        return;
    }

    const auto& hot_reload = engine::runtime::diagnostics().hot_reload;
    out_metrics->attempt_count = hot_reload.attempt_count;
    out_metrics->failure_count = hot_reload.failure_count;
    out_metrics->cancelled_count = hot_reload.cancelled_count;
    out_metrics->rejected_count = hot_reload.rejected_count;
    out_metrics->pending_count = hot_reload.pending_count;
    out_metrics->loading_count = hot_reload.loading_count;
    out_metrics->total_requests = hot_reload.total_requests;
    out_metrics->last_error = hot_reload.last_error.c_str();
    out_metrics->error_hint = hot_reload.error_hint.c_str();
}

extern "C" ENGINE_RUNTIME_API std::uint32_t

engine_runtime_diagnostic_hot_reload_recent_failure_count() noexcept
{
    const auto& failures = engine::runtime::diagnostics().hot_reload.recent_failures;
    return static_cast<std::uint32_t>(failures.size());
}

extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_identifier(std::uint32_t index) noexcept
{
    const auto& failures = engine::runtime::diagnostics().hot_reload.recent_failures;
    if (index >= failures.size())
    {
        return nullptr;
    }

    return failures[index].identifier.c_str();
}

extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_error(std::uint32_t index) noexcept
{
    const auto& failures = engine::runtime::diagnostics().hot_reload.recent_failures;
    if (index >= failures.size())
    {
        return nullptr;
    }

    return failures[index].error.c_str();
}

extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_hint(std::uint32_t index) noexcept
{
    const auto& failures = engine::runtime::diagnostics().hot_reload.recent_failures;
    if (index >= failures.size())
    {
        return nullptr;
    }

    return failures[index].hint.c_str();
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_initialize_count() noexcept
{
    return engine::runtime::diagnostics().initialize_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_initialize_failure_count() noexcept
{
    return engine::runtime::diagnostics().initialize_failure_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_shutdown_count() noexcept
{
    return engine::runtime::diagnostics().shutdown_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_tick_count() noexcept
{
    return engine::runtime::diagnostics().tick_count;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_initialize_ms() noexcept
{
    return engine::runtime::diagnostics().last_initialize_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_shutdown_ms() noexcept
{
    return engine::runtime::diagnostics().last_shutdown_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_tick_ms() noexcept
{
    return engine::runtime::diagnostics().last_tick_ms;
}

extern "C" ENGINE_RUNTIME_API bool engine_runtime_diagnostic_has_initialize_failure() noexcept
{
    return engine::runtime::diagnostics().has_initialize_failure;
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_runtime() noexcept
{
    const auto& diagnostics = engine::runtime::diagnostics();
    if (!diagnostics.has_initialize_failure)
    {
        return "";
    }
    return diagnostics.last_initialize_failure.runtime.c_str();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_subsystem() noexcept
{
    const auto& diagnostics = engine::runtime::diagnostics();
    if (!diagnostics.has_initialize_failure)
    {
        return "";
    }
    return diagnostics.last_initialize_failure.subsystem.c_str();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_category() noexcept
{
    const auto& diagnostics = engine::runtime::diagnostics();
    if (!diagnostics.has_initialize_failure)
    {
        return "";
    }
    return diagnostics.last_initialize_failure.category.c_str();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_message() noexcept
{
    const auto& diagnostics = engine::runtime::diagnostics();
    if (!diagnostics.has_initialize_failure)
    {
        return "";
    }
    return diagnostics.last_initialize_failure.message.c_str();
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_initialize_failure_duration_ms() noexcept
{
    const auto& diagnostics = engine::runtime::diagnostics();
    if (!diagnostics.has_initialize_failure)
    {
        return 0.0;
    }
    return diagnostics.last_initialize_failure.duration_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_average_tick_ms() noexcept
{
    return engine::runtime::diagnostics().average_tick_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_max_tick_ms() noexcept
{
    return engine::runtime::diagnostics().max_tick_ms;
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_stage_count() noexcept
{
    return engine::runtime::diagnostics().stage_timings.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_stage_name(std::size_t index) noexcept
{
    const auto& stages = engine::runtime::diagnostics().stage_timings;
    if (index >= stages.size())
    {
        return nullptr;
    }
    return stages[index].name.c_str();
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_last_ms(std::size_t index) noexcept
{
    const auto& stages = engine::runtime::diagnostics().stage_timings;
    if (index >= stages.size())
    {
        return 0.0;
    }
    return stages[index].last_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_average_ms(std::size_t index) noexcept
{
    const auto& stages = engine::runtime::diagnostics().stage_timings;
    if (index >= stages.size())
    {
        return 0.0;
    }
    return stages[index].average_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_max_ms(std::size_t index) noexcept
{
    const auto& stages = engine::runtime::diagnostics().stage_timings;
    if (index >= stages.size())
    {
        return 0.0;
    }
    return stages[index].max_ms;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_stage_samples(std::size_t index) noexcept
{
    const auto& stages = engine::runtime::diagnostics().stage_timings;
    if (index >= stages.size())
    {
        return 0;
    }
    return stages[index].sample_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_animation_clip_track_count() noexcept
{
    return engine::runtime::diagnostics().animation.clip_track_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_animation_pose_joint_count() noexcept
{
    return engine::runtime::diagnostics().animation.pose_joint_count;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_animation_clip_duration() noexcept
{
    return engine::runtime::diagnostics().animation.clip_duration;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_animation_playback_time() noexcept
{
    return engine::runtime::diagnostics().animation.playback_time;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_animation_playback_speed() noexcept
{
    return engine::runtime::diagnostics().animation.playback_speed;
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_animation_category_count() noexcept
{
    return engine::runtime::diagnostics().animation.category_totals.size();
}

extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_animation_category_label(std::size_t index) noexcept
{
    const auto& categories = engine::runtime::diagnostics().animation.category_totals;
    if (index >= categories.size())
    {
        return nullptr;
    }
    return categories[index].label.c_str();
}

extern "C" ENGINE_RUNTIME_API double
engine_runtime_diagnostic_animation_category_duration_ms(std::size_t index) noexcept
{
    const auto& categories = engine::runtime::diagnostics().animation.category_totals;
    if (index >= categories.size())
    {
        return 0.0;
    }
    return categories[index].duration_ms;
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_animation_queue_count() noexcept
{
    return engine::runtime::diagnostics().animation.queue_totals.size();
}

extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_animation_queue_label(std::size_t index) noexcept
{
    const auto& queues = engine::runtime::diagnostics().animation.queue_totals;
    if (index >= queues.size())
    {
        return nullptr;
    }
    return queues[index].label.c_str();
}

extern "C" ENGINE_RUNTIME_API double
engine_runtime_diagnostic_animation_queue_duration_ms(std::size_t index) noexcept
{
    const auto& queues = engine::runtime::diagnostics().animation.queue_totals;
    if (index >= queues.size())
    {
        return 0.0;
    }
    return queues[index].duration_ms;
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_subsystem_count() noexcept
{
    return engine::runtime::diagnostics().subsystem_timings.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_name(std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return nullptr;
    }
    return subsystems[index].name.c_str();
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_initialize_ms(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].last_initialize_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_tick_ms(std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].last_tick_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_shutdown_ms(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].last_shutdown_ms;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_initialize_count(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0;
    }
    return subsystems[index].initialize_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_tick_count(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0;
    }
    return subsystems[index].tick_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_shutdown_count(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0;
    }
    return subsystems[index].shutdown_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_initialize_failure_count(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0;
    }
    return subsystems[index].initialize_failure_count;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_initialize_ms(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].max_initialize_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_tick_ms(std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].max_tick_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_shutdown_ms(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].max_shutdown_ms;
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_initialize_failure_ms(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return 0.0;
    }
    return subsystems[index].last_initialize_failure_ms;
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_last_initialize_failure_category(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return "";
    }
    return subsystems[index].last_initialize_failure_category.c_str();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_last_initialize_failure_message(
    std::size_t index) noexcept
{
    const auto& subsystems = engine::runtime::diagnostics().subsystem_timings;
    if (index >= subsystems.size())
    {
        return "";
    }
    return subsystems[index].last_initialize_failure_message.c_str();
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_issue_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.issue_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_cycle_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.cycle_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_dangling_parent_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.dangling_parent_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_missing_parent_hierarchy_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.missing_parent_hierarchy_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_non_finite_transform_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.non_finite_transform_count;
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_transform_mismatch_count() noexcept
{
    return engine::runtime::diagnostics().scene_validation.metrics.transform_mismatch_count;
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_scene_issue_total() noexcept
{
    return engine::runtime::diagnostics().scene_validation.issues.size();
}

extern "C" ENGINE_RUNTIME_API std::uint32_t engine_runtime_diagnostic_scene_issue_entity(std::size_t index) noexcept
{
    const auto& issues = engine::runtime::diagnostics().scene_validation.issues;
    if (index >= issues.size())
    {
        return 0U;
    }

    return static_cast<std::uint32_t>(issues[index].entity);
}

extern "C" ENGINE_RUNTIME_API std::uint32_t engine_runtime_diagnostic_scene_issue_related(std::size_t index) noexcept
{
    const auto& issues = engine::runtime::diagnostics().scene_validation.issues;
    if (index >= issues.size())
    {
        return 0U;
    }

    return static_cast<std::uint32_t>(issues[index].related);
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_scene_issue_type_name(
    std::size_t index) noexcept
{
    const auto& issues = engine::runtime::diagnostics().scene_validation.issues;
    if (index >= issues.size())
    {
        return "unknown";
    }

    return hierarchy_issue_type_name(issues[index].type);
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_scene_issue_message(std::size_t index) noexcept
{
    const auto& issues = engine::runtime::diagnostics().scene_validation.issues;
    if (index >= issues.size())
    {
        return "";
    }

    return issues[index].message.c_str();
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_metric_count() noexcept
{
    return engine::runtime::diagnostics().metrics.descriptors.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_name(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.descriptors.size())
    {
        return "";
    }
    return metrics.descriptors[index].name.c_str();
}

extern "C" ENGINE_RUNTIME_API int engine_runtime_diagnostic_metric_kind(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.descriptors.size())
    {
        return 0;
    }
    return static_cast<int>(metrics.descriptors[index].kind);
}

extern "C" ENGINE_RUNTIME_API int engine_runtime_diagnostic_metric_unit(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.descriptors.size())
    {
        return 0;
    }
    return static_cast<int>(metrics.descriptors[index].unit);
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_description(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.descriptors.size())
    {
        return "";
    }
    return metrics.descriptors[index].description.c_str();
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_metric_label_count(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.descriptors.size())
    {
        return 0;
    }
    return metrics.descriptors[index].labels.size();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_label_key(std::size_t metric_index,
    std::size_t label_index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (metric_index >= metrics.descriptors.size())
    {
        return "";
    }
    const auto& labels = metrics.descriptors[metric_index].labels;
    if (label_index >= labels.size())
    {
        return "";
    }
    return labels[label_index].key.c_str();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_label_value(std::size_t metric_index,
    std::size_t label_index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (metric_index >= metrics.descriptors.size())
    {
        return "";
    }
    const auto& labels = metrics.descriptors[metric_index].labels;
    if (label_index >= labels.size())
    {
        return "";
    }
    return labels[label_index].value.c_str();
}

extern "C" ENGINE_RUNTIME_API bool engine_runtime_diagnostic_metric_is_integral(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.samples.size())
    {
        return false;
    }
    return engine::core::telemetry::is_integral(metrics.samples[index].value);
}

extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_metric_value(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.samples.size())
    {
        return 0.0;
    }
    return engine::core::telemetry::as_double(metrics.samples[index].value);
}

extern "C" ENGINE_RUNTIME_API std::int64_t engine_runtime_diagnostic_metric_value_int(std::size_t index) noexcept
{
    const auto& metrics = engine::runtime::diagnostics().metrics;
    if (index >= metrics.samples.size())
    {
        return 0;
    }
    return engine::core::telemetry::as_int(metrics.samples[index].value);
}