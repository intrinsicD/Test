#include "engine/runtime/api.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include "engine/animation/deformation/linear_blend_skinning.hpp"
#include "engine/geometry/deform/linear_blend_skinning.hpp"

#if ENGINE_ENABLE_ASSETS
#    include "engine/assets/api.hpp"
#    include "engine/assets/async.hpp"
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

namespace engine::runtime
{
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
        std::unique_ptr<compute::Dispatcher> dispatcher{compute::make_cpu_dispatcher()};
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
        RuntimeDiagnostics diagnostics{};
        std::unordered_map<std::string, std::size_t> stage_lookup{};
        std::unordered_map<std::string, std::size_t> subsystem_lookup{};
#if ENGINE_ENABLE_RENDERING
        rendering::components::RenderGeometry render_geometry{};
        std::string renderable_name{"runtime.renderable"};
        scene::Entity render_entity{};
        rendering::ForwardPipeline forward_pipeline{};
#endif

        explicit Impl(RuntimeHostDependencies deps)
            : dependencies(std::move(deps))
        {
#if ENGINE_ENABLE_RENDERING
            render_geometry = dependencies.render_geometry;
            if (!dependencies.renderable_name.empty())
            {
                renderable_name = dependencies.renderable_name;
            }
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
        }

        void record_initialize_duration(Clock::duration duration)
        {
            const double ms = duration_to_ms(duration);
            diagnostics.last_initialize_ms = ms;
            diagnostics.max_initialize_ms = std::max(diagnostics.max_initialize_ms, ms);
            diagnostics.initialize_count += 1U;
        }

        void record_shutdown_duration(Clock::duration duration)
        {
            const double ms = duration_to_ms(duration);
            diagnostics.last_shutdown_ms = ms;
            diagnostics.max_shutdown_ms = std::max(diagnostics.max_shutdown_ms, ms);
            diagnostics.shutdown_count += 1U;
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
        }

        void record_stage_timings(const compute::ExecutionReport& report)
        {
            const std::size_t count =
                std::min(report.execution_order.size(), report.kernel_durations.size());
            for (std::size_t index = 0; index < count; ++index)
            {
                const std::string& name = report.execution_order[index];
                RuntimeStageTiming& timing = ensure_stage_timing(name);
                const double duration_ms = report.kernel_durations[index] * 1000.0;
                timing.last_ms = duration_ms;
                timing.sample_count += 1U;
                const double samples = static_cast<double>(timing.sample_count);
                if (samples > 0.0)
                {
                    timing.average_ms += (duration_ms - timing.average_ms) / samples;
                }
                timing.max_ms = std::max(timing.max_ms, duration_ms);
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
#endif

        void reset_state()
        {
            initialized = false;
            simulation_time = 0.0;
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
            if (dispatcher == nullptr)
            {
                dispatcher = compute::make_cpu_dispatcher();
            }
            dispatcher->clear();
            last_report = {};
            body_positions.clear();
            joint_names.clear();
            scene_nodes.clear();
            joint_entities.clear();
            scene = scene::Scene{scene_name()};
#if ENGINE_ENABLE_RENDERING
            render_entity = scene::Entity{};
            render_geometry = dependencies.render_geometry;
            if (!dependencies.renderable_name.empty())
            {
                renderable_name = dependencies.renderable_name;
            }
#endif
            rebuild_subsystem_cache();
        }

        void configure(RuntimeHostDependencies deps)
        {
            if (initialized)
            {
                throw std::runtime_error("RuntimeHost cannot be configured while initialized");
            }

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

        void synchronize_scene_graph(const math::vec3& body_translation)
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
            synchronize_scene_graph(translation);
            const engine::core::plugin::SubsystemLifecycleContext lifecycle{runtime_name_view()};
            for (const auto& plugin : dependencies.subsystem_plugins)
            {
                if (plugin != nullptr)
                {
                    const std::string name{plugin->name()};
                    const auto start = Clock::now();
                    plugin->initialize(lifecycle);
                    const auto duration = Clock::now() - start;
                    record_subsystem_event(name, duration, SubsystemPhase::Initialize);
                }
            }
            initialized = true;
            record_initialize_duration(Clock::now() - initialize_start);
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
            if (dispatcher != nullptr)
            {
                dispatcher->clear();
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
        }

        runtime_frame_state tick(double dt)
        {
            if (!initialized)
            {
                throw std::runtime_error("RuntimeHost must be initialized before tick()");
            }

            const auto tick_start = Clock::now();
            if (dispatcher == nullptr)
            {
                dispatcher = compute::make_cpu_dispatcher();
            }
            dispatcher->clear();

            auto& dispatcher_ref = *dispatcher;

            const auto animation_kernel = dispatcher_ref.add_kernel(
                "animation.evaluate",
                [&]()
                {
                    engine::animation::advance_controller(controller, dt);
                    pose = engine::animation::evaluate_controller(controller);
                });

            const auto physics_forces = dispatcher_ref.add_kernel(
                "physics.accumulate",
                [&]()
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
                {animation_kernel});

            const auto physics_integrate = dispatcher_ref.add_kernel(
                "physics.integrate",
                [&]()
                {
                    engine::physics::integrate(world, dt);
                    refresh_body_positions();
                },
                {physics_forces});

            const auto deform = dispatcher_ref.add_kernel(
                "geometry.deform",
                [&]()
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

                    animation::skinning::build_global_joint_transforms(binding, pose, joint_global_transforms,
                                                                        root_translation);
                    animation::skinning::build_skinning_transforms(binding, joint_global_transforms,
                                                                    skinning_transforms);
                    engine::geometry::deform::apply_linear_blend_skinning(binding, skinning_transforms, mesh);
                },
                {physics_integrate});

            MAYBE_UNUSED_CONST_AUTO finalize_kernel = dispatcher_ref.add_kernel(
                "geometry.finalize",
                [&]()
                {
                    engine::geometry::update_bounds(mesh);
                    refresh_joint_names();
                    const math::vec3 translation = body_positions.empty()
                                                       ? math::vec3{0.0F, 0.0F, 0.0F}
                                                       : body_positions.front();
                    synchronize_scene_graph(translation);
                },
                {deform});

            last_report = dispatcher_ref.dispatch();
            record_stage_timings(last_report);
            simulation_time += dt;
            const engine::core::plugin::SubsystemUpdateContext update_context{dt};
            for (const auto& plugin : dependencies.subsystem_plugins)
            {
                if (plugin != nullptr)
                {
                    const std::string name{plugin->name()};
                    const auto start = Clock::now();
                    plugin->tick(update_context);
                    const auto duration = Clock::now() - start;
                    record_subsystem_event(name, duration, SubsystemPhase::Tick);
                }
            }
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
        pipeline->render(impl_->scene, context.resources, context.materials, context.device_resources,
                         context.scheduler, context.encoders, context.frame_graph);
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
#endif
        return metrics;
    }

    const RuntimeDiagnostics& diagnostics() noexcept
    {
        return global_host().diagnostics();
    }

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
}

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_initialize_count() noexcept
{
    return engine::runtime::diagnostics().initialize_count;
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
