#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/runtime/api.hpp"
#include "engine/runtime/subsystem_registry.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
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
    engine::physics::add_body(world, body);
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

TEST(RuntimeHost, SubmitsRenderGraphThroughVulkanScheduler) {
    engine::runtime::RuntimeHostDependencies deps{};
    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        engine::assets::MeshHandle{std::string{"runtime.mesh"}},
        engine::assets::MaterialHandle{std::string{"runtime.material"}});
    deps.renderable_name = "runtime.renderable";

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    const auto frame = host.tick(0.016);
    ASSERT_FALSE(frame.scene_nodes.empty());  // NOLINT

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"runtime.material"}},
        engine::assets::ShaderHandle{std::string{"runtime.shader"}},
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

    ASSERT_EQ(scheduler.submissions().size(), 1U);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.pass_name, "ForwardGeometry");
    EXPECT_EQ(submission.command_buffer.queue.api, engine::rendering::resources::GraphicsApi::Vulkan);

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
