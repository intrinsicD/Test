#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "engine/core/telemetry/schema.hpp"
#include "engine/physics/api.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/backend/vulkan/resource_translation.hpp"
#include "engine/runtime/api.hpp"
#include "engine/assets/validation.hpp"
#include "engine/runtime/diagnostics_bridge.hpp"
#include "engine/runtime/subsystem_registry.hpp"
#include "engine/scene/validation.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/command_encoder.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"

namespace {

std::vector<std::string_view> expected_default_modules()
{
    std::vector<std::string_view> modules{};
#if ENGINE_ENABLE_ANIMATION
    modules.emplace_back("animation");
#endif
#if ENGINE_ENABLE_ASSETS
    modules.emplace_back("assets");
#endif
#if ENGINE_ENABLE_COMPUTE
    modules.emplace_back("compute");
#endif
#if ENGINE_ENABLE_COMPUTE && ENGINE_ENABLE_COMPUTE_CUDA
    modules.emplace_back("compute.cuda");
#endif
#if ENGINE_ENABLE_CORE
    modules.emplace_back("core");
#endif
#if ENGINE_ENABLE_GEOMETRY
    modules.emplace_back("geometry");
#endif
#if ENGINE_ENABLE_IO
    modules.emplace_back("io");
#endif
#if ENGINE_ENABLE_PHYSICS
    modules.emplace_back("physics");
#endif
#if ENGINE_ENABLE_PLATFORM
    modules.emplace_back("platform");
#endif
#if ENGINE_ENABLE_RENDERING
    modules.emplace_back("rendering");
#endif
#if ENGINE_ENABLE_SCENE
    modules.emplace_back("scene");
#endif
    return modules;
}

std::optional<std::size_t> find_metric_index(const engine::core::telemetry::MetricSet& metrics,
                                             std::string_view name,
                                             std::optional<std::pair<std::string_view, std::string_view>> label = std::nullopt)
{
    for (std::size_t index = 0; index < metrics.descriptors.size(); ++index)
    {
        const auto& descriptor = metrics.descriptors[index];
        if (descriptor.name != name)
        {
            continue;
        }
        if (!label.has_value())
        {
            return index;
        }
        for (const auto& entry : descriptor.labels)
        {
            if (entry.key == label->first && entry.value == label->second)
            {
                return index;
            }
        }
    }

    return std::nullopt;
}

class TestSubsystem final : public engine::core::plugin::ISubsystemInterface {
public:
    TestSubsystem(std::string name, std::vector<std::string> dependencies)
        : name_(std::move(name)), dependencies_storage_(std::move(dependencies))
    {
        dependency_views_.reserve(dependencies_storage_.size());
        for (const auto& dependency : dependencies_storage_)
        {
            dependency_views_.push_back(dependency);
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override
    {
        return name_;
    }

    [[nodiscard]] std::span<const std::string_view> dependencies() const noexcept override
    {
        return dependency_views_;
    }

    void initialize(const engine::core::plugin::SubsystemLifecycleContext&) override {}

    void shutdown(const engine::core::plugin::SubsystemLifecycleContext&) noexcept override {}

    void tick(const engine::core::plugin::SubsystemUpdateContext&) override {}

private:
    std::string name_{};
    std::vector<std::string> dependencies_storage_{};
    std::vector<std::string_view> dependency_views_{};
};

std::shared_ptr<engine::core::plugin::ISubsystemInterface> make_test_subsystem(
    std::string name,
    std::vector<std::string> dependencies = {})
{
    return std::make_shared<TestSubsystem>(std::move(name), std::move(dependencies));
}

class RecordingLifecycleSubsystem final : public engine::core::plugin::ISubsystemInterface
{
public:
    explicit RecordingLifecycleSubsystem(std::string name, std::vector<std::string> dependencies = {})
        : name_(std::move(name)), dependencies_storage_(std::move(dependencies))
    {
        dependency_views_.reserve(dependencies_storage_.size());
        for (const auto& dependency : dependencies_storage_)
        {
            dependency_views_.push_back(dependency);
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override
    {
        return name_;
    }

    [[nodiscard]] std::span<const std::string_view> dependencies() const noexcept override
    {
        return dependency_views_;
    }

    void initialize(const engine::core::plugin::SubsystemLifecycleContext& context) override
    {
        initialize_calls += 1U;
        initialize_contexts.emplace_back(context.runtime_name);
        if (remaining_failures > 0U)
        {
            --remaining_failures;
            throw std::runtime_error("RecordingLifecycleSubsystem forced failure");
        }
    }

    void shutdown(const engine::core::plugin::SubsystemLifecycleContext& context) noexcept override
    {
        shutdown_calls += 1U;
        shutdown_contexts.emplace_back(context.runtime_name);
    }

    void tick(const engine::core::plugin::SubsystemUpdateContext&) override {}

    void fail_initialization(std::size_t count = 1U) noexcept
    {
        remaining_failures = count;
    }

    void clear_failures() noexcept
    {
        remaining_failures = 0U;
    }

    std::string name_{};
    std::vector<std::string> dependencies_storage_{};
    std::vector<std::string_view> dependency_views_{};
    std::size_t initialize_calls{0U};
    std::size_t shutdown_calls{0U};
    std::vector<std::string> initialize_contexts{};
    std::vector<std::string> shutdown_contexts{};
    std::size_t remaining_failures{0U};
};

std::shared_ptr<RecordingLifecycleSubsystem> make_recording_subsystem(
    std::string name,
    std::vector<std::string> dependencies = {})
{
    return std::make_shared<RecordingLifecycleSubsystem>(std::move(name), std::move(dependencies));
}

struct ScopedHandleValidators
{
    ScopedHandleValidators()
    {
        auto& registry = engine::assets::HandleValidatorRegistry::instance();
        mesh = registry.register_mesh_validator([](const engine::assets::MeshHandle&) { return true; });
        graph = registry.register_graph_validator([](const engine::assets::GraphHandle&) { return true; });
        cloud = registry.register_point_cloud_validator([](const engine::assets::PointCloudHandle&) { return true; });
        material = registry.register_material_validator([](const engine::assets::MaterialHandle&) { return true; });
        shader = registry.register_shader_validator([](const engine::assets::ShaderHandle&) { return true; });
    }

    [[maybe_unused]] std::shared_ptr<void> mesh{};
    [[maybe_unused]] std::shared_ptr<void> graph{};
    [[maybe_unused]] std::shared_ptr<void> cloud{};
    [[maybe_unused]] std::shared_ptr<void> material{};
    [[maybe_unused]] std::shared_ptr<void> shader{};
};

class RecordingRenderResourceProvider final : public engine::rendering::RenderResourceProvider
{
public:
    void require_mesh(const engine::assets::MeshHandle& handle) override
    {
        meshes.push_back(handle);
    }

    void require_graph(const engine::assets::GraphHandle& handle) override
    {
        graphs.push_back(handle);
    }

    void require_point_cloud(const engine::assets::PointCloudHandle& handle) override
    {
        point_clouds.push_back(handle);
    }

    void require_material(const engine::assets::MaterialHandle& handle) override
    {
        materials.push_back(handle);
    }

    void require_shader(const engine::assets::ShaderHandle& handle) override
    {
        shaders.push_back(handle);
    }

    std::vector<engine::assets::MeshHandle> meshes;
    std::vector<engine::assets::GraphHandle> graphs;
    std::vector<engine::assets::PointCloudHandle> point_clouds;
    std::vector<engine::assets::MaterialHandle> materials;
    std::vector<engine::assets::ShaderHandle> shaders;
};

class RecordingCommandEncoder final : public engine::rendering::CommandEncoder
{
public:
    void draw_geometry(const engine::rendering::GeometryDrawCommand& command) override
    {
        draws.push_back(command);
    }

    std::vector<engine::rendering::GeometryDrawCommand> draws;
};

class RecordingCommandEncoderProvider final : public engine::rendering::CommandEncoderProvider
{
public:
    struct DescriptorRecord
    {
        std::string pass_name;
        engine::rendering::QueueType queue{engine::rendering::QueueType::Graphics};
        engine::rendering::CommandBufferHandle command_buffer{};
    };

    std::unique_ptr<engine::rendering::CommandEncoder> begin_encoder(
        const engine::rendering::CommandEncoderDescriptor& descriptor) override
    {
        begin_records.push_back(DescriptorRecord{std::string{descriptor.pass_name}, descriptor.queue,
                                                 descriptor.command_buffer});
        return std::make_unique<RecordingCommandEncoder>();
    }

    void end_encoder(const engine::rendering::CommandEncoderDescriptor& descriptor,
                     std::unique_ptr<engine::rendering::CommandEncoder> encoder) override
    {
        end_records.push_back(DescriptorRecord{std::string{descriptor.pass_name}, descriptor.queue,
                                               descriptor.command_buffer});
        auto* recording = dynamic_cast<RecordingCommandEncoder*>(encoder.release());
        if (recording != nullptr)
        {
            completed_encoders.emplace_back(recording);
        }
    }

    std::vector<DescriptorRecord> begin_records;
    std::vector<DescriptorRecord> end_records;
    std::vector<std::unique_ptr<RecordingCommandEncoder>> completed_encoders;
};

class LocalRecordingScheduler final : public engine::rendering::IGpuScheduler
{
public:
    engine::rendering::QueueType select_queue(const engine::rendering::RenderPass&,
                                              engine::rendering::QueueType preferred) override
    {
        return preferred;
    }

    engine::rendering::CommandBufferHandle request_command_buffer(engine::rendering::QueueType,
                                                                  std::string_view) override
    {
        return engine::rendering::CommandBufferHandle{++next_command_buffer_};
    }

    void submit(const engine::rendering::GpuSubmitInfo& info) override
    {
        submissions.push_back(info);
        if (info.fence != nullptr)
        {
            info.fence->signal(info.fence_value);
        }
        for (const auto& wait : info.waits)
        {
            if (wait.semaphore != nullptr)
            {
                wait.semaphore->wait(wait.value);
            }
        }
        for (const auto& signal : info.signals)
        {
            if (signal.semaphore != nullptr)
            {
                signal.semaphore->signal(signal.value);
            }
        }
    }

    void recycle(engine::rendering::CommandBufferHandle) override {}

    std::vector<engine::rendering::GpuSubmitInfo> submissions;

private:
    std::size_t next_command_buffer_{0};
};

}  // namespace

TEST(RuntimeModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::runtime::module_name(), "runtime");
    EXPECT_STREQ(engine_runtime_module_name(), "runtime");
}

TEST(RuntimeModule, ExecutesSimulationPipeline) {
    engine::runtime::shutdown();
    engine::runtime::initialize();

    const auto frame = engine::runtime::tick(0.016);
    EXPECT_GE(frame.dispatch_report.execution_order.size(), 4U);
    EXPECT_EQ(frame.dispatch_report.execution_order.front(), "animation.evaluate");
    EXPECT_EQ(frame.dispatch_report.execution_order.back(), "geometry.finalize");
    ASSERT_EQ(frame.dispatch_report.execution_order.size(), frame.dispatch_report.kernel_durations.size());
    for (const auto duration : frame.dispatch_report.kernel_durations)
    {
        EXPECT_GE(duration, 0.0);
    }

    EXPECT_FALSE(frame.pose.joints.empty());
    const auto* root = frame.pose.find("root");
    ASSERT_TRUE(root != nullptr);

    const auto& mesh = engine::runtime::current_mesh();
    EXPECT_GE(mesh.bounds.max[1], mesh.bounds.min[1]);
    EXPECT_FALSE(frame.body_positions.empty());
    EXPECT_FALSE(frame.scene_nodes.empty());
    const auto& root_node = frame.scene_nodes.front();
    EXPECT_EQ(root_node.name, "root");
    const float expected_root_height = frame.body_positions.front()[1] + root->translation[1];
    EXPECT_NEAR(root_node.transform.translation[1], expected_root_height, 1e-4F);
    EXPECT_EQ(engine_runtime_scene_node_count(), frame.scene_nodes.size());
    EXPECT_STREQ(engine_runtime_scene_node_name(0), "root");
    float scales[3]{};
    float rotations[4]{};
    float translations[3]{};
    engine_runtime_scene_node_transform(0, scales, rotations, translations);
    EXPECT_FLOAT_EQ(translations[1], root_node.transform.translation[1]);

    const auto dispatch_count = engine_runtime_dispatch_count();
    ASSERT_EQ(dispatch_count, frame.dispatch_report.execution_order.size());
    for (std::size_t index = 0; index < dispatch_count; ++index)
    {
        EXPECT_EQ(engine_runtime_dispatch_duration(index), frame.dispatch_report.kernel_durations[index]);
    }

    engine::runtime::shutdown();
}

TEST(RuntimeHost, EnforcesLifecycleSemantics) {
    engine::runtime::RuntimeHost host{};
    EXPECT_FALSE(host.is_initialized());
    EXPECT_THROW(host.tick(0.016), std::runtime_error);

    host.initialize();
    EXPECT_TRUE(host.is_initialized());
    const auto first_frame = host.tick(0.016);
    EXPECT_NEAR(first_frame.simulation_time, 0.016, 1e-9);

    host.shutdown();
    EXPECT_FALSE(host.is_initialized());

    host.shutdown();
    host.initialize();
    const auto second_frame = host.tick(0.008);
    EXPECT_NEAR(second_frame.simulation_time, 0.008, 1e-9);

    host.shutdown();
    EXPECT_FALSE(host.is_initialized());
}

TEST(RuntimeHost, AcceptsInjectedDependencies) {
    engine::animation::AnimationClip clip{};
    clip.name = "custom";
    clip.duration = 1.0;
    engine::animation::JointTrack track{};
    track.joint_name = "custom_joint";
    track.keyframes.push_back(engine::animation::Keyframe{});
    clip.tracks.push_back(track);

    engine::runtime::RuntimeHostDependencies deps{};
    deps.controller = engine::animation::make_linear_controller(std::move(clip));
    deps.scene_name = "custom.scene";

    engine::physics::PhysicsWorld world{};
    world.gravity = engine::math::vec3{0.0F, -1.0F, 0.0F};
    engine::physics::RigidBody body{};
    body.mass = 3.0F;
    body.position = engine::math::vec3{1.0F, 2.0F, 3.0F};
    [[maybe_unused]] const auto injected_body = engine::physics::add_body(world, body);
    deps.world = world;

    engine::geometry::SurfaceMesh mesh = engine::geometry::make_unit_quad();
    engine::geometry::apply_uniform_translation(mesh, engine::math::vec3{0.0F, 2.0F, 0.0F});
    engine::geometry::update_bounds(mesh);
    deps.mesh = mesh;

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    EXPECT_TRUE(host.is_initialized());
    EXPECT_FALSE(host.body_positions().empty());
    ASSERT_FALSE(host.joint_names().empty());
    EXPECT_EQ(host.joint_names().front(), "custom_joint");
    EXPECT_NEAR(host.current_mesh().bounds.min[1], mesh.bounds.min[1], 1e-5F);
    host.shutdown();
}

TEST(RuntimeHost, AppliesLinearBlendSkinning) {
    engine::animation::AnimationClip clip{};
    clip.name = "skinning";
    clip.duration = 1.0;

    engine::animation::JointTrack root_track{};
    root_track.joint_name = "root";
    root_track.keyframes.push_back(engine::animation::Keyframe{
        0.0,
        engine::animation::JointPose{engine::math::vec3{0.0F, 0.0F, 0.0F},
                                     engine::math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                     engine::math::vec3{1.0F, 1.0F, 1.0F}}});
    root_track.keyframes.push_back(root_track.keyframes.front());

    engine::animation::JointTrack child_track{};
    child_track.joint_name = "child";
    child_track.keyframes.push_back(engine::animation::Keyframe{
        0.0,
        engine::animation::JointPose{engine::math::vec3{0.0F, 2.0F, 0.0F},
                                     engine::math::normalize(engine::math::angle_axis(
                                         engine::math::radians(90.0F), engine::math::vec3{0.0F, 0.0F, 1.0F})),
                                     engine::math::vec3{1.0F, 1.0F, 1.0F}}});
    child_track.keyframes.push_back(child_track.keyframes.front());

    clip.tracks.push_back(root_track);
    clip.tracks.push_back(child_track);

    engine::runtime::RuntimeHostDependencies deps{};
    deps.controller = engine::animation::make_linear_controller(std::move(clip));

    engine::geometry::SurfaceMesh mesh{};
    mesh.rest_positions = {
        engine::math::vec3{0.0F, 0.0F, 0.0F},
        engine::math::vec3{0.0F, 2.0F, 0.0F},
        engine::math::vec3{0.0F, 3.0F, 0.0F},
    };
    mesh.positions = mesh.rest_positions;
    mesh.indices = {0U, 1U, 2U};
    deps.mesh = mesh;

    engine::animation::RigBinding binding{};
    engine::animation::RigJoint root{};
    root.name = "root";
    root.parent = engine::animation::RigBinding::kInvalidIndex;
    root.inverse_bind_pose = engine::math::Transform<float>::Identity();
    binding.joints.push_back(root);

    engine::animation::RigJoint child{};
    child.name = "child";
    child.parent = 0U;
    child.inverse_bind_pose.translation = engine::math::vec3{0.0F, -2.0F, 0.0F};
    binding.joints.push_back(child);

    binding.resize_vertices(mesh.rest_positions.size());
    binding.vertices[0].clear();
    ASSERT_TRUE(binding.vertices[0].add_influence(0U, 1.0F));
    binding.vertices[0].normalize_weights();
    binding.vertices[1].clear();
    ASSERT_TRUE(binding.vertices[1].add_influence(0U, 0.5F));
    ASSERT_TRUE(binding.vertices[1].add_influence(1U, 0.5F));
    binding.vertices[1].normalize_weights();
    binding.vertices[2].clear();
    ASSERT_TRUE(binding.vertices[2].add_influence(1U, 1.0F));
    binding.vertices[2].normalize_weights();
    deps.binding = binding;

    engine::physics::PhysicsWorld world{};
    engine::physics::RigidBody anchor{};
    anchor.mass = 0.0F;
    anchor.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    [[maybe_unused]] const auto anchor_index = engine::physics::add_body(world, anchor);
    deps.world = world;

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    const auto frame = host.tick(0.0);
    ASSERT_FALSE(frame.scene_nodes.empty());  // NOLINT

    const auto& skinned_mesh = host.current_mesh();
    ASSERT_EQ(skinned_mesh.positions.size(), 3U);
    EXPECT_NEAR(skinned_mesh.positions[0][0], 0.0F, 1.0e-5F);
    EXPECT_NEAR(skinned_mesh.positions[0][1], 0.0F, 1.0e-5F);
    EXPECT_NEAR(skinned_mesh.positions[2][0], -1.0F, 1.0e-3F);
    EXPECT_NEAR(skinned_mesh.positions[2][1], 2.0F, 1.0e-3F);

    host.shutdown();
}

TEST(RuntimeHost, SubmitsRenderGraphThroughVulkanScheduler) {
    ScopedHandleValidators handle_validators;
    engine::runtime::RuntimeHostDependencies deps{};
    engine::assets::MeshHandle mesh_handle{std::string{"runtime.mesh"}};
    engine::assets::MeshHandle::pool_handle_type mesh_raw{};
    mesh_raw.index = 0U;
    mesh_raw.generation = 1U;
    mesh_handle.bind(mesh_raw);
    engine::assets::MaterialHandle material_handle{std::string{"runtime.material"}};
    engine::assets::MaterialHandle::pool_handle_type material_raw{};
    material_raw.index = 0U;
    material_raw.generation = 1U;
    material_handle.bind(material_raw);
    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        mesh_handle,
        material_handle);
    deps.renderable_name = "runtime.renderable";

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    const auto frame = host.tick(0.016);
    ASSERT_FALSE(frame.scene_nodes.empty());  // NOLINT

    engine::rendering::MaterialSystem materials;
    engine::assets::ShaderHandle shader_handle{std::string{"runtime.shader"}};
    engine::assets::ShaderHandle::pool_handle_type shader_raw{};
    shader_raw.index = 0U;
    shader_raw.generation = 1U;
    shader_handle.bind(shader_raw);
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        material_handle,
        shader_handle,
    });

    RecordingRenderResourceProvider render_resources;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider(
        engine::rendering::resources::GraphicsApi::Vulkan);
    engine::rendering::backend::vulkan::VulkanGpuScheduler scheduler(device_provider);
    RecordingCommandEncoderProvider command_encoders;
    engine::rendering::FrameGraph graph;

    engine::runtime::RuntimeHost::RenderSubmissionContext context{
        render_resources,
        materials,
        device_provider,
        scheduler,
        command_encoders,
        graph,
        nullptr,
    };

    host.submit_render_graph(context);

    const auto& acquired_resources = device_provider.acquired();
    ASSERT_EQ(acquired_resources.size(), 2U);  // NOLINT
    for (const auto& record : acquired_resources)
    {
        const auto& info = record.info;
        const auto description =
            engine::rendering::backend::vulkan::translate_resource(info);
        const auto* image_description = std::get_if<
            engine::rendering::backend::vulkan::VulkanImageResourceDescription>(&description);
        ASSERT_NE(image_description, nullptr);
        EXPECT_EQ(image_description->image.extent.width, 1280U);
        EXPECT_EQ(image_description->image.extent.height, 720U);
        EXPECT_EQ(image_description->image.extent.depth, 1U);
        EXPECT_EQ(image_description->image.mipLevels, 1U);
        EXPECT_EQ(image_description->image.arrayLayers, 1U);
        EXPECT_EQ(image_description->image.samples, VK_SAMPLE_COUNT_1_BIT);

        if (info.name == "ForwardColor")
        {
            EXPECT_EQ(image_description->image.format, VK_FORMAT_R16G16B16A16_SFLOAT);
            EXPECT_EQ(image_description->image.usage,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            EXPECT_EQ(image_description->initial_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            EXPECT_EQ(image_description->final_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            EXPECT_EQ(image_description->subresource_range.aspectMask, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        else if (info.name == "ForwardDepth")
        {
            EXPECT_EQ(image_description->image.format, VK_FORMAT_D24_UNORM_S8_UINT);
            EXPECT_EQ(image_description->image.usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            EXPECT_EQ(image_description->initial_layout,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            EXPECT_EQ(image_description->final_layout,
                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            EXPECT_EQ(image_description->subresource_range.aspectMask,
                      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }
        else
        {
            FAIL() << "Unexpected frame-graph resource: " << info.name;
        }
    }

    const auto& released_resources = device_provider.released();
    ASSERT_EQ(released_resources.size(), acquired_resources.size());  // NOLINT
    for (std::size_t index = 0; index < released_resources.size(); ++index)
    {
        EXPECT_EQ(released_resources[index].info.name, acquired_resources[index].info.name);
    }

    const auto& diagnostics = host.diagnostics();
    EXPECT_FALSE(diagnostics.frame_graph_serialization.empty());
    EXPECT_NE(diagnostics.frame_graph_serialization.find("\"ForwardGeometry\""), std::string::npos);
    ASSERT_EQ(diagnostics.frame_graph_events.size(), 4U);
    EXPECT_EQ(diagnostics.frame_graph_events[0].type,
              engine::rendering::ResourceEvent::Type::Acquire);
    EXPECT_EQ(diagnostics.frame_graph_events[0].resource_name, "ForwardColor");
    EXPECT_EQ(diagnostics.frame_graph_events[1].type,
              engine::rendering::ResourceEvent::Type::Acquire);
    EXPECT_EQ(diagnostics.frame_graph_events[1].resource_name, "ForwardDepth");
    EXPECT_EQ(diagnostics.frame_graph_events[2].type,
              engine::rendering::ResourceEvent::Type::Release);
    EXPECT_EQ(diagnostics.frame_graph_events[2].resource_name, "ForwardColor");
    EXPECT_EQ(diagnostics.frame_graph_events[3].type,
              engine::rendering::ResourceEvent::Type::Release);
    EXPECT_EQ(diagnostics.frame_graph_events[3].resource_name, "ForwardDepth");
    for (const auto& event : diagnostics.frame_graph_events)
    {
        EXPECT_EQ(event.pass_name, "ForwardGeometry");
    }

    ASSERT_EQ(scheduler.submissions().size(), 1U);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.pass_name, "ForwardGeometry");
    EXPECT_EQ(submission.command_buffer.queue.api, engine::rendering::resources::GraphicsApi::Vulkan);

    ASSERT_EQ(submission.begin_barriers.size(), acquired_resources.size());  // NOLINT
    for (const auto& barrier : submission.begin_barriers)
    {
        const auto info = graph.resource_info(barrier.resource);
        const auto vk_barrier = engine::rendering::backend::vulkan::translate_barrier(barrier);
        EXPECT_EQ(vk_barrier.source_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        EXPECT_EQ(vk_barrier.destination_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        EXPECT_EQ(vk_barrier.source_access, VK_ACCESS_MEMORY_READ_BIT);
        EXPECT_EQ(vk_barrier.destination_access, VK_ACCESS_MEMORY_WRITE_BIT);
        EXPECT_TRUE(info.name == "ForwardColor" || info.name == "ForwardDepth");
    }

    ASSERT_EQ(submission.end_barriers.size(), acquired_resources.size());  // NOLINT
    for (const auto& barrier : submission.end_barriers)
    {
        const auto info = graph.resource_info(barrier.resource);
        const auto vk_barrier = engine::rendering::backend::vulkan::translate_barrier(barrier);
        EXPECT_EQ(vk_barrier.source_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        EXPECT_EQ(vk_barrier.destination_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        EXPECT_EQ(vk_barrier.source_access, VK_ACCESS_MEMORY_WRITE_BIT);
        EXPECT_EQ(vk_barrier.destination_access, VK_ACCESS_MEMORY_READ_BIT);
        EXPECT_TRUE(info.name == "ForwardColor" || info.name == "ForwardDepth");
    }

    ASSERT_EQ(command_encoders.completed_encoders.size(), 1U);  // NOLINT
    const auto& encoder = *command_encoders.completed_encoders.front();
    ASSERT_EQ(encoder.draws.size(), 1U);  // NOLINT
    const auto& draw = encoder.draws.front();
    ASSERT_TRUE(std::holds_alternative<engine::assets::MeshHandle>(draw.geometry));  // NOLINT
    EXPECT_EQ(std::get<engine::assets::MeshHandle>(draw.geometry).id(), std::string{"runtime.mesh"});
    EXPECT_EQ(draw.material.id(), std::string{"runtime.material"});

    const auto renderable_node_it = std::find_if(
        frame.scene_nodes.begin(), frame.scene_nodes.end(),
        [](const auto& node) { return node.name == "runtime.renderable"; });
    const bool has_renderable_node = renderable_node_it != frame.scene_nodes.end();
    EXPECT_TRUE(has_renderable_node);
    const auto* renderable_node_ptr = has_renderable_node ? &*renderable_node_it : nullptr;
    ASSERT_NE(renderable_node_ptr, nullptr);
    const auto& renderable_node = *renderable_node_ptr;
    EXPECT_EQ(renderable_node.transform.translation, draw.transform.translation);

    ASSERT_EQ(render_resources.meshes.size(), 1U);  // NOLINT
    EXPECT_EQ(render_resources.meshes.front().id(), std::string{"runtime.mesh"});
    ASSERT_EQ(render_resources.materials.size(), 1U);  // NOLINT
    EXPECT_EQ(render_resources.materials.front().id(), std::string{"runtime.material"});
    EXPECT_EQ(device_provider.frames_begun(), 1U);
    EXPECT_EQ(device_provider.frames_completed(), 1U);

    RecordingRenderResourceProvider measurement_resources;
    engine::rendering::resources::RecordingGpuResourceProvider measurement_device(
        engine::rendering::resources::GraphicsApi::Vulkan);
    LocalRecordingScheduler measurement_scheduler;
    RecordingCommandEncoderProvider measurement_encoders;
    engine::rendering::FrameGraph measurement_graph;
    engine::runtime::RuntimeHost::RenderSubmissionContext measurement_context{
        measurement_resources,
        materials,
        measurement_device,
        measurement_scheduler,
        measurement_encoders,
        measurement_graph,
        nullptr,
    };

    constexpr int iterations = 50;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        host.submit_render_graph(measurement_context);
    }
    const auto end = std::chrono::steady_clock::now();
    const double average_ms = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
    std::cout << "[runtime.render] average_submit_ms=" << average_ms << '\n';
    EXPECT_LT(average_ms, 1.0);
    EXPECT_EQ(measurement_device.frames_begun(), static_cast<std::size_t>(iterations));
    EXPECT_EQ(measurement_device.frames_completed(), static_cast<std::size_t>(iterations));

    host.shutdown();
}

TEST(RuntimeHost, LoadsSubsystemsFromRegistrySelection) {
    auto registry = std::make_shared<engine::runtime::SubsystemRegistry>();
    registry->register_subsystem(engine::runtime::SubsystemDescriptor{
        "alpha",
        {},
        []() { return make_test_subsystem("alpha"); },
        false});
    registry->register_subsystem(engine::runtime::SubsystemDescriptor{
        "beta",
        {"alpha"},
        []() { return make_test_subsystem("beta", {"alpha"}); },
        false});

    engine::runtime::RuntimeHostDependencies deps{};
    deps.subsystem_registry = registry;
    deps.enabled_subsystems = {"beta"};

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    const auto names = host.subsystem_names();
    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "beta");
    host.shutdown();
}

TEST(SubsystemRegistry, RejectsDependencyCycles)
{
    engine::runtime::SubsystemRegistry registry{};
    registry.register_subsystem(engine::runtime::SubsystemDescriptor{
        "alpha",
        {"beta"},
        []() { return make_test_subsystem("alpha", {"beta"}); },
        false});

    try
    {
        registry.register_subsystem(engine::runtime::SubsystemDescriptor{
            "beta",
            {"alpha"},
            []() { return make_test_subsystem("beta", {"alpha"}); },
            false});
        FAIL() << "Expected cycle detection to throw";
    }
    catch (const std::invalid_argument& error)
    {
        const std::string_view message{error.what()};
        EXPECT_NE(message.find("alpha -> beta -> alpha"), std::string_view::npos);
    }
}

TEST(RuntimeHostDependencies, RejectsSubsystemDependencyCycles)
{
    engine::runtime::RuntimeHostDependencies deps{};
    deps.subsystem_plugins = {
        make_test_subsystem("alpha", {"beta"}),
        make_test_subsystem("beta", {"alpha"}),
    };

    try
    {
        const engine::runtime::RuntimeHost host{deps};
        (void)host;
        FAIL() << "Expected dependency validation failure";
    }
    catch (const std::runtime_error& error)
    {
        const std::string message{error.what()};
        EXPECT_NE(message.find("dependency_cycle"), std::string::npos);
        EXPECT_NE(message.find("alpha -> beta -> alpha"), std::string::npos);
    }
}

TEST(RuntimeHost, ProvidesLifecycleContextForSubsystems)
{
    auto plugin = make_recording_subsystem("alpha");

    engine::runtime::RuntimeHostDependencies deps{};
    deps.scene_name = "runtime.alpha";
    deps.subsystem_plugins = {plugin};

    engine::runtime::RuntimeHost host{deps};
    host.initialize();

    ASSERT_EQ(plugin->initialize_calls, 1U);
    ASSERT_EQ(plugin->initialize_contexts.size(), 1U);
    EXPECT_EQ(plugin->initialize_contexts.front(), deps.scene_name);

    host.shutdown();

    ASSERT_EQ(plugin->shutdown_calls, 1U);
    ASSERT_EQ(plugin->shutdown_contexts.size(), 1U);
    EXPECT_EQ(plugin->shutdown_contexts.front(), deps.scene_name);
}

TEST(RuntimeHost, ShutsDownInitializedSubsystemsWhenLaterPluginFails)
{
    auto first = make_recording_subsystem("alpha");
    auto second = make_recording_subsystem("beta");
    second->fail_initialization();

    engine::runtime::RuntimeHostDependencies deps{};
    deps.scene_name = "runtime.failure";
    deps.subsystem_plugins = {first, second};

    engine::runtime::RuntimeHost host{deps};

    EXPECT_THROW(host.initialize(), std::runtime_error);
    EXPECT_FALSE(host.is_initialized());

    EXPECT_EQ(first->initialize_calls, 1U);
    EXPECT_EQ(first->shutdown_calls, 1U);
    EXPECT_EQ(second->initialize_calls, 1U);
    EXPECT_EQ(second->shutdown_calls, 0U);
    ASSERT_FALSE(first->initialize_contexts.empty());
    ASSERT_FALSE(first->shutdown_contexts.empty());
    EXPECT_EQ(first->initialize_contexts.front(), deps.scene_name);
    EXPECT_EQ(first->shutdown_contexts.front(), deps.scene_name);

    const auto& diagnostics = host.diagnostics();
    const auto timing_it = std::find_if(
        diagnostics.subsystem_timings.begin(),
        diagnostics.subsystem_timings.end(),
        [](const engine::runtime::RuntimeSubsystemTiming& timing) { return timing.name == "alpha"; });
    ASSERT_NE(timing_it, diagnostics.subsystem_timings.end());
    EXPECT_EQ(timing_it->initialize_count, 1U);
    EXPECT_EQ(timing_it->shutdown_count, 1U);
    EXPECT_EQ(timing_it->tick_count, 0U);

    host.shutdown();
}

TEST(RuntimeHost, RecoversAfterSubsystemInitializationFailure)
{
    auto first = make_recording_subsystem("alpha");
    auto second = make_recording_subsystem("beta");
    second->fail_initialization();

    engine::runtime::RuntimeHostDependencies deps{};
    deps.subsystem_plugins = {first, second};

    engine::runtime::RuntimeHost host{deps};

    EXPECT_THROW(host.initialize(), std::runtime_error);
    EXPECT_FALSE(host.is_initialized());
    EXPECT_EQ(first->initialize_calls, 1U);
    EXPECT_EQ(first->shutdown_calls, 1U);

    second->clear_failures();

    EXPECT_NO_THROW(host.initialize());
    EXPECT_TRUE(host.is_initialized());
    EXPECT_EQ(first->initialize_calls, 2U);
    EXPECT_EQ(second->initialize_calls, 2U);

    host.shutdown();

    EXPECT_EQ(first->shutdown_calls, 2U);
    EXPECT_EQ(second->shutdown_calls, 1U);
}

TEST(RuntimeHost, StreamingMetricsReflectConfiguration)
{
    engine::runtime::RuntimeHostDependencies deps{};
    deps.streaming_config.worker_count = 1;
    deps.streaming_config.queue_capacity = 4;

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    const auto metrics = engine::runtime::streaming_metrics();
    EXPECT_EQ(metrics.worker_count, deps.streaming_config.worker_count);
    EXPECT_EQ(metrics.queue_capacity, deps.streaming_config.queue_capacity);
    host.shutdown();
}

TEST(RuntimeHost, DiagnosticsExposeStreamingMetrics)
{
    engine::runtime::RuntimeHost host{};
    host.initialize();

    const auto direct_metrics = engine::runtime::streaming_metrics();
    const auto& diagnostics = host.diagnostics();

    EXPECT_EQ(diagnostics.streaming.worker_count, direct_metrics.worker_count);
    EXPECT_EQ(diagnostics.streaming.queue_capacity, direct_metrics.queue_capacity);
    EXPECT_EQ(diagnostics.streaming.pending_tasks, direct_metrics.pending_tasks);
    EXPECT_EQ(diagnostics.streaming.active_workers, direct_metrics.active_workers);
    EXPECT_EQ(diagnostics.streaming.total_enqueued, direct_metrics.total_enqueued);
    EXPECT_EQ(diagnostics.streaming.total_executed, direct_metrics.total_executed);
    EXPECT_EQ(diagnostics.streaming.streaming_pending, direct_metrics.streaming_pending);
    EXPECT_EQ(diagnostics.streaming.streaming_loading, direct_metrics.streaming_loading);
    EXPECT_EQ(diagnostics.streaming.streaming_total_requests, direct_metrics.streaming_total_requests);
    EXPECT_EQ(diagnostics.streaming.streaming_total_completed, direct_metrics.streaming_total_completed);
    EXPECT_EQ(diagnostics.streaming.streaming_total_failed, direct_metrics.streaming_total_failed);
    EXPECT_EQ(diagnostics.streaming.streaming_total_cancelled, direct_metrics.streaming_total_cancelled);
    EXPECT_EQ(diagnostics.streaming.streaming_total_rejected, direct_metrics.streaming_total_rejected);

    host.shutdown();
}

TEST(RuntimeHost, DiagnosticsExposeMetricSchema)
{
    engine::runtime::RuntimeHost host{};
    host.initialize();
    host.tick(0.016);

    const auto& diagnostics = host.diagnostics();
    const auto& metrics = diagnostics.metrics;

    ASSERT_FALSE(metrics.descriptors.empty());
    ASSERT_EQ(metrics.descriptors.size(), metrics.samples.size());

    const auto tick_metric = find_metric_index(metrics, "runtime.lifecycle.tick.count");
    ASSERT_TRUE(tick_metric.has_value());
    EXPECT_EQ(metrics.samples[*tick_metric].descriptor_index, *tick_metric);
    EXPECT_EQ(engine::core::telemetry::as_int(metrics.samples[*tick_metric].value),
              static_cast<std::int64_t>(diagnostics.tick_count));

    const auto stage_metric = find_metric_index(
        metrics,
        "runtime.stage.last_ms",
        std::make_pair(std::string_view{"stage"}, std::string_view{"physics.integrate"}));
    ASSERT_TRUE(stage_metric.has_value());
    EXPECT_EQ(metrics.samples[*stage_metric].descriptor_index, *stage_metric);
    EXPECT_GE(engine::core::telemetry::as_double(metrics.samples[*stage_metric].value), 0.0);

    const auto streaming_metric = find_metric_index(metrics, "runtime.streaming.worker_count");
    ASSERT_TRUE(streaming_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*streaming_metric].value),
                     static_cast<double>(diagnostics.streaming.worker_count));

    const auto issue_metric = find_metric_index(
        metrics,
        "runtime.scene_validation.issues",
        std::make_pair(std::string_view{"type"}, std::string_view{"cycle"}));
    ASSERT_TRUE(issue_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*issue_metric].value), 0.0);

    host.shutdown();
}

TEST(RuntimeHost, DiagnosticsIncludePhysicsTelemetry)
{
    engine::runtime::RuntimeHostDependencies deps{};

    engine::physics::RigidBody ground{};
    ground.mass = 0.0F;
    ground.collider = engine::physics::Collider::make_aabb(
        engine::geometry::Aabb{engine::math::vec3{-2.0F, -0.25F, -2.0F},
                               engine::math::vec3{2.0F, 0.0F, 2.0F}});
    MAYBE_UNUSED_CONST_AUTO ground_id = engine::physics::add_body(deps.world, ground);
    (void)ground_id;

    engine::physics::RigidBody sphere{};
    sphere.mass = 1.0F;
    sphere.position = engine::math::vec3{0.0F, 0.0F, 0.0F};
    sphere.collider = engine::physics::Collider::make_sphere(0.5F);
    MAYBE_UNUSED_CONST_AUTO sphere_id = engine::physics::add_body(deps.world, sphere);
    (void)sphere_id;

    deps.world.gravity = engine::math::vec3{0.0F, 0.0F, 0.0F};

    engine::runtime::RuntimeHost host{deps};
    host.initialize();

    host.tick(0.016);

    const auto& diagnostics = host.diagnostics();
    const auto& telemetry = diagnostics.physics_collision;

    EXPECT_GE(telemetry.manifold_count, 1U);
    EXPECT_GE(telemetry.contact_count, 1U);
    EXPECT_GE(telemetry.solver_iterations, 1U);
    EXPECT_GE(telemetry.max_penetration, 0.0F);

    const auto& metrics = diagnostics.metrics;
    const auto manifold_metric = find_metric_index(metrics, "runtime.physics.manifold_count");
    ASSERT_TRUE(manifold_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*manifold_metric].value),
                     static_cast<double>(telemetry.manifold_count));

    const auto contact_metric = find_metric_index(metrics, "runtime.physics.contact_count");
    ASSERT_TRUE(contact_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*contact_metric].value),
                     static_cast<double>(telemetry.contact_count));

    const auto penetration_metric = find_metric_index(metrics, "runtime.physics.max_penetration");
    ASSERT_TRUE(penetration_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*penetration_metric].value),
                     static_cast<double>(telemetry.max_penetration));

    const auto iteration_metric = find_metric_index(metrics, "runtime.physics.solver_iterations");
    ASSERT_TRUE(iteration_metric.has_value());
    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*iteration_metric].value),
                     static_cast<double>(telemetry.solver_iterations));

    host.shutdown();
}

TEST(RuntimeDiagnosticsCAPI, MetricEnumerationsExposeSchema)
{
    engine_runtime_shutdown();
    engine_runtime_initialize();
    engine_runtime_tick(0.016);

    const std::size_t count = engine_runtime_diagnostic_metric_count();
    ASSERT_GT(count, 0U);

    bool found_tick = false;
    bool found_stage_label = false;

    for (std::size_t index = 0; index < count; ++index)
    {
        const std::string name = engine_runtime_diagnostic_metric_name(index);
        const int kind = engine_runtime_diagnostic_metric_kind(index);
        const int unit = engine_runtime_diagnostic_metric_unit(index);
        const std::size_t label_count = engine_runtime_diagnostic_metric_label_count(index);

        if (name == "runtime.lifecycle.tick.count")
        {
            found_tick = true;
            EXPECT_EQ(kind, static_cast<int>(engine::core::telemetry::MetricKind::Counter));
            EXPECT_EQ(unit, static_cast<int>(engine::core::telemetry::MetricUnit::Count));
            EXPECT_TRUE(engine_runtime_diagnostic_metric_is_integral(index));
            EXPECT_EQ(engine_runtime_diagnostic_metric_value_int(index),
                      static_cast<std::int64_t>(engine_runtime_diagnostic_tick_count()));
        }

        if (name == "runtime.stage.last_ms" && label_count > 0U)
        {
            for (std::size_t label = 0; label < label_count; ++label)
            {
                const std::string key = engine_runtime_diagnostic_metric_label_key(index, label);
                const std::string value = engine_runtime_diagnostic_metric_label_value(index, label);
                if (key == "stage" && value == "physics.integrate")
                {
                    found_stage_label = true;
                    EXPECT_EQ(kind, static_cast<int>(engine::core::telemetry::MetricKind::Gauge));
                    EXPECT_EQ(unit, static_cast<int>(engine::core::telemetry::MetricUnit::Milliseconds));
                    EXPECT_FALSE(engine_runtime_diagnostic_metric_is_integral(index));
                    EXPECT_GE(engine_runtime_diagnostic_metric_value(index), 0.0);
                }
            }
        }
    }

    EXPECT_TRUE(found_tick);
    EXPECT_TRUE(found_stage_label);

    engine_runtime_shutdown();
}

TEST(RuntimeHost, RejectsDependenciesWithMismatchedMeshVertexCounts)
{
    engine::runtime::RuntimeHost host{};
    host.shutdown();

    engine::runtime::RuntimeHostDependencies deps{};
    deps.mesh.positions = {engine::math::vec3{0.0F, 0.0F, 0.0F}};
    deps.mesh.rest_positions = {
        engine::math::vec3{0.0F, 0.0F, 0.0F},
        engine::math::vec3{1.0F, 0.0F, 0.0F},
    };

    EXPECT_THROW(host.configure(std::move(deps)), std::runtime_error);
}

TEST(RuntimeHost, RejectsDependenciesWithInvalidBinding)
{
    engine::runtime::RuntimeHost host{};
    host.shutdown();

    engine::runtime::RuntimeHostDependencies deps{};
    engine::geometry::SurfaceMesh mesh{};
    mesh.rest_positions = {
        engine::math::vec3{0.0F, 0.0F, 0.0F},
        engine::math::vec3{0.5F, 0.0F, 0.0F},
        engine::math::vec3{1.0F, 0.0F, 0.0F},
    };
    mesh.positions = mesh.rest_positions;
    mesh.normals.assign(mesh.rest_positions.size(), engine::math::vec3{0.0F, 1.0F, 0.0F});
    mesh.indices = {0U, 1U, 2U};
    engine::geometry::update_bounds(mesh);
    deps.mesh = mesh;

    engine::animation::RigJoint root{};
    root.name = "root";
    root.parent = engine::animation::RigBinding::kInvalidIndex;
    deps.binding.joints = {root};
    deps.binding.resize_vertices(deps.mesh.rest_positions.size());
    deps.binding.vertices[0].clear();
    ASSERT_TRUE(deps.binding.vertices[0].add_influence(0U, 1.0F));
    deps.binding.vertices[0].normalize_weights();
    deps.binding.vertices[0].influences[0].joint = 5U;

    EXPECT_THROW(host.configure(std::move(deps)), std::runtime_error);
}

TEST(RuntimeHost, RejectsDependenciesWithInvalidClip)
{
    engine::runtime::RuntimeHost host{};
    host.shutdown();

    engine::runtime::RuntimeHostDependencies deps{};
    deps.controller.clip.name.clear();
    deps.controller.clip.duration = -1.0;
    deps.controller.clip.tracks.clear();

    EXPECT_THROW(host.configure(std::move(deps)), std::runtime_error);
}

TEST(RuntimeHost, AllowsDependenciesWithEmptyBinding)
{
    engine::runtime::RuntimeHost host{};
    host.shutdown();

    engine::runtime::RuntimeHostDependencies deps{};
    deps.binding.joints.clear();
    deps.binding.vertices.clear();

    EXPECT_NO_THROW(host.configure(std::move(deps)));
    host.initialize();
    EXPECT_TRUE(host.is_initialized());
    host.shutdown();
}

TEST(RuntimeHost, ReconfiguringDependenciesResetsState)
{
    engine::runtime::RuntimeHost host{};
    host.initialize();
    host.tick(0.016);
    host.shutdown();

    engine::runtime::RuntimeHostDependencies deps{};

    engine::geometry::SurfaceMesh custom_mesh{};
    custom_mesh.rest_positions = {
        engine::math::vec3{0.0F, 1.0F, 0.0F},
        engine::math::vec3{1.0F, 1.0F, 0.0F},
        engine::math::vec3{0.0F, 2.0F, 0.0F},
    };
    custom_mesh.positions = custom_mesh.rest_positions;
    custom_mesh.normals.assign(custom_mesh.rest_positions.size(), engine::math::vec3{0.0F, 1.0F, 0.0F});
    custom_mesh.indices = {0U, 1U, 2U};
    engine::geometry::update_bounds(custom_mesh);
    deps.mesh = custom_mesh;

    engine::animation::RigJoint root{};
    root.name = "hip";
    root.parent = engine::animation::RigBinding::kInvalidIndex;
    deps.binding.joints = {root};
    deps.binding.resize_vertices(deps.mesh.rest_positions.size());
    for (auto& vertex : deps.binding.vertices)
    {
        vertex.clear();
        ASSERT_TRUE(vertex.add_influence(0U, 1.0F));
        vertex.normalize_weights();
    }

    engine::animation::AnimationClip clip{};
    clip.name = "reset";
    clip.duration = 1.0;
    engine::animation::JointTrack track{};
    track.joint_name = "hip";
    track.keyframes.push_back(engine::animation::Keyframe{
        0.0,
        engine::animation::JointPose{engine::math::vec3{0.0F, 0.0F, 0.0F},
                                     engine::math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                     engine::math::vec3{1.0F, 1.0F, 1.0F}}});
    track.keyframes.push_back(engine::animation::Keyframe{
        0.5,
        engine::animation::JointPose{engine::math::vec3{0.0F, 1.0F, 0.0F},
                                     engine::math::normalize(engine::math::quat{0.9238795F, 0.0F, 0.3826834F, 0.0F}),
                                     engine::math::vec3{1.0F, 1.0F, 1.0F}}});
    clip.tracks.push_back(track);
    deps.controller = engine::animation::make_linear_controller(std::move(clip));

    host.configure(std::move(deps));
    host.initialize();
    ASSERT_TRUE(host.is_initialized());
    EXPECT_EQ(host.diagnostics().tick_count, 0U);
    ASSERT_EQ(host.joint_names().size(), 1U);
    EXPECT_EQ(host.joint_names().front(), "hip");

    const auto frame = host.tick(0.016);
    ASSERT_FALSE(frame.scene_nodes.empty());
    EXPECT_EQ(frame.scene_nodes.front().name, "hip");

    const auto& runtime_mesh = host.current_mesh();
    ASSERT_EQ(runtime_mesh.rest_positions.size(), custom_mesh.rest_positions.size());
    EXPECT_NEAR(runtime_mesh.rest_positions.front()[1], custom_mesh.rest_positions.front()[1], 1e-6F);

    host.shutdown();
}

TEST(RuntimeHost, ExposesLifecycleDiagnostics)
{
    engine::runtime::RuntimeHost host{};
    const auto& initial = host.diagnostics();
    EXPECT_EQ(initial.initialize_count, 0U);
    EXPECT_EQ(initial.tick_count, 0U);

    host.initialize();
    const auto& after_initialize = host.diagnostics();
    EXPECT_EQ(after_initialize.initialize_count, 1U);
    EXPECT_GE(after_initialize.last_initialize_ms, 0.0);
    EXPECT_FALSE(after_initialize.subsystem_timings.empty());

    host.tick(0.016);
    const auto& after_tick = host.diagnostics();
    EXPECT_EQ(after_tick.tick_count, 1U);
    EXPECT_GE(after_tick.last_tick_ms, 0.0);
    EXPECT_TRUE(after_tick.scene_validation.ok());
    EXPECT_EQ(after_tick.scene_validation.metrics.issue_count, 0U);
    const bool has_animation_stage = std::any_of(
        after_tick.stage_timings.begin(),
        after_tick.stage_timings.end(),
        [](const engine::runtime::RuntimeStageTiming& stage) { return stage.name == "animation.evaluate"; });
    EXPECT_TRUE(has_animation_stage);
    const bool subsystem_ticked = std::any_of(
        after_tick.subsystem_timings.begin(),
        after_tick.subsystem_timings.end(),
        [](const engine::runtime::RuntimeSubsystemTiming& timing) { return timing.tick_count > 0U; });
    EXPECT_TRUE(subsystem_ticked);

    host.shutdown();
    const auto& after_shutdown = host.diagnostics();
    EXPECT_EQ(after_shutdown.shutdown_count, 1U);
    EXPECT_GE(after_shutdown.last_shutdown_ms, 0.0);
}

TEST(RuntimeModule, ConfiguresGlobalHostWithRegistrySelection) {
    engine::runtime::shutdown();

    auto registry = std::make_shared<engine::runtime::SubsystemRegistry>();
    registry->register_subsystem(engine::runtime::SubsystemDescriptor{
        "alpha",
        {},
        []() { return make_test_subsystem("alpha"); },
        false});
    registry->register_subsystem(engine::runtime::SubsystemDescriptor{
        "beta",
        {"alpha"},
        []() { return make_test_subsystem("beta", {"alpha"}); },
        false});

    engine::runtime::RuntimeHostDependencies deps{};
    deps.subsystem_registry = registry;
    deps.enabled_subsystems = {"beta"};

    engine::runtime::configure(std::move(deps));

    EXPECT_FALSE(engine::runtime::is_initialized());
    ASSERT_EQ(engine::runtime::module_count(), 2U);
    EXPECT_EQ(engine::runtime::module_name_at(0), "alpha");
    EXPECT_EQ(engine::runtime::module_name_at(1), "beta");

    engine::runtime::initialize();
    EXPECT_TRUE(engine::runtime::is_initialized());
    engine::runtime::shutdown();

    auto default_registry = std::make_shared<engine::runtime::SubsystemRegistry>(
        engine::runtime::make_default_subsystem_registry());
    engine::runtime::RuntimeHostDependencies defaults{};
    defaults.subsystem_registry = default_registry;
    defaults.subsystem_plugins = default_registry->load_defaults();
    engine::runtime::configure(std::move(defaults));

    const auto expected = expected_default_modules();
    ASSERT_EQ(engine::runtime::module_count(), expected.size());
}

TEST(RuntimeModule, EnumeratesAllEngineModules) {
    const auto expected = expected_default_modules();

    ASSERT_EQ(engine::runtime::module_count(), expected.size());
    EXPECT_EQ(engine_runtime_module_count(), expected.size());

    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(engine::runtime::module_name_at(index), expected[index]);
        EXPECT_STREQ(engine_runtime_module_at(index), expected[index].data());
    }

    EXPECT_TRUE(engine::runtime::module_name_at(expected.size()).empty());
    EXPECT_EQ(engine_runtime_module_at(expected.size()), nullptr);
}

TEST(RuntimeModule, ReportsDefaultSubsystemNames) {
    const auto names = engine::runtime::default_subsystem_names();
    const auto expected = expected_default_modules();
    ASSERT_EQ(names.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(names[index], expected[index]);
    }
}

TEST(RuntimeModule, ConfigureWithDefaultSubsystemHelper) {
    engine::runtime::shutdown();
    engine::runtime::configure_with_default_subsystems();
    EXPECT_EQ(engine::runtime::module_count(), expected_default_modules().size());
}

TEST(RuntimeDiagnosticsBridge, DispatchesHierarchyCallbacks)
{
    auto& bridge = engine::runtime::DiagnosticsBridge::instance();
    bridge.reset_for_testing();

    engine::scene::validation::HierarchyValidationReport report{};
    engine::scene::validation::HierarchyValidationIssue issue{};
    issue.entity = static_cast<entt::entity>(42);
    issue.related = static_cast<entt::entity>(24);
    issue.type = engine::scene::validation::HierarchyIssueType::Cycle;
    issue.message = "cycle detected";
    report.issues.push_back(issue);
    report.metrics.issue_count = report.issues.size();
    report.metrics.cycle_count = 1U;

    bool invoked = false;
    const auto callback_id = bridge.register_hierarchy_callback(
        [&](const engine::scene::validation::HierarchyValidationReport& published, double time) {
            invoked = true;
            EXPECT_EQ(published.metrics.issue_count, 1U);
            EXPECT_DOUBLE_EQ(time, 1.5);
        });

    bridge.publish_hierarchy_report(report, 1.5);
    EXPECT_TRUE(invoked);

    bridge.unregister_hierarchy_callback(callback_id);
    bridge.reset_for_testing();
}

TEST(RuntimeDiagnosticsBridge, ExposesSceneIssuesThroughCAPI)
{
    engine::runtime::shutdown();
    engine::runtime::configure_with_default_subsystems();
    engine::runtime::initialize();

    engine::scene::validation::HierarchyValidationReport report{};
    report.metrics.issue_count = 1U;
    report.metrics.cycle_count = 1U;
    report.metrics.dangling_parent_count = 2U;
    report.metrics.missing_parent_hierarchy_count = 1U;
    report.metrics.non_finite_transform_count = 0U;
    report.metrics.transform_mismatch_count = 3U;

    engine::scene::validation::HierarchyValidationIssue issue{};
    issue.entity = static_cast<entt::entity>(101);
    issue.related = static_cast<entt::entity>(202);
    issue.type = engine::scene::validation::HierarchyIssueType::TransformMismatch;
    issue.message = "mismatched transform";
    report.issues.push_back(issue);

    auto& diagnostics = const_cast<engine::runtime::RuntimeDiagnostics&>(engine::runtime::diagnostics());
    diagnostics.scene_validation = report;

    EXPECT_EQ(engine_runtime_diagnostic_scene_issue_count(), 1U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_cycle_count(), 1U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_dangling_parent_count(), 2U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_missing_parent_hierarchy_count(), 1U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_non_finite_transform_count(), 0U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_transform_mismatch_count(), 3U);

    ASSERT_EQ(engine_runtime_diagnostic_scene_issue_total(), 1U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_issue_entity(0), 101U);
    EXPECT_EQ(engine_runtime_diagnostic_scene_issue_related(0), 202U);
    EXPECT_STREQ(engine_runtime_diagnostic_scene_issue_type_name(0), "transform_mismatch");
    EXPECT_STREQ(engine_runtime_diagnostic_scene_issue_message(0), "mismatched transform");

    engine::runtime::shutdown();
}

#if ENGINE_ENABLE_ANIMATION && ENGINE_ENABLE_SCENE
TEST(RuntimeModule, ConfigureSubsetViaHelpers) {
    engine::runtime::shutdown();
    const std::array selections{
        std::string_view{"animation"},
        std::string_view{"scene"},
    };
    engine::runtime::configure_with_default_subsystems(selections);
    ASSERT_EQ(engine::runtime::module_count(), selections.size());
    EXPECT_EQ(engine::runtime::module_name_at(0), selections[0]);
    EXPECT_EQ(engine::runtime::module_name_at(1), selections[1]);
}

TEST(RuntimeModule, ConfigureSubsetViaCInterface) {
    engine::runtime::shutdown();
    const char* modules[] = {"animation", "scene"};
    engine_runtime_configure_with_modules(modules, std::size(modules));
    ASSERT_EQ(engine::runtime::module_count(), std::size(modules));
    EXPECT_STREQ(engine_runtime_module_at(0), modules[0]);
    EXPECT_STREQ(engine_runtime_module_at(1), modules[1]);
}
#endif
