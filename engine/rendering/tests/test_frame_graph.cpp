#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/scene/scene.hpp"
#include "command_encoder_test_utils.hpp"
#include "scheduler_test_utils.hpp"

namespace
{
    class NullProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle&) override {}
        void require_graph(const engine::assets::GraphHandle&) override {}
        void require_point_cloud(const engine::assets::PointCloudHandle&) override {}
        void require_material(const engine::assets::MaterialHandle&) override {}
        void require_shader(const engine::assets::ShaderHandle&) override {}
    };

    engine::rendering::FrameGraphResourceDescriptor make_color_resource(std::string name)
    {
        engine::rendering::FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.format = engine::rendering::ResourceFormat::Rgba8Unorm;
        descriptor.dimension = engine::rendering::ResourceDimension::Texture2D;
        descriptor.usage = engine::rendering::ResourceUsage::ColorAttachment |
                           engine::rendering::ResourceUsage::ShaderRead;
        descriptor.initial_state = engine::rendering::ResourceState::ColorAttachment;
        descriptor.final_state = engine::rendering::ResourceState::ShaderRead;
        descriptor.width = 1920;
        descriptor.height = 1080;
        descriptor.depth = 1;
        descriptor.array_layers = 1;
        descriptor.mip_levels = 1;
        descriptor.sample_count = engine::rendering::ResourceSampleCount::Count1;
        return descriptor;
    }

    engine::rendering::FrameGraphResourceDescriptor make_depth_resource(std::string name)
    {
        engine::rendering::FrameGraphResourceDescriptor descriptor{};
        descriptor.name = std::move(name);
        descriptor.format = engine::rendering::ResourceFormat::Depth24Stencil8;
        descriptor.dimension = engine::rendering::ResourceDimension::Texture2D;
        descriptor.usage = engine::rendering::ResourceUsage::DepthStencilAttachment;
        descriptor.initial_state = engine::rendering::ResourceState::DepthStencilAttachment;
        descriptor.final_state = engine::rendering::ResourceState::DepthStencilAttachment;
        descriptor.width = 1920;
        descriptor.height = 1080;
        descriptor.depth = 1;
        descriptor.array_layers = 1;
        descriptor.mip_levels = 1;
        descriptor.sample_count = engine::rendering::ResourceSampleCount::Count1;
        return descriptor;
    }
}

TEST(FrameGraph, SchedulesPassesBasedOnDependencies)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource(make_depth_resource("Depth"));
    const auto color = graph.create_resource(make_color_resource("Color"));

    std::vector<std::string> order;

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "DepthPrepass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(depth); },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
            EXPECT_EQ(context.pass_phase(), engine::rendering::PassPhase::Setup);
            EXPECT_EQ(context.validation_severity(), engine::rendering::ValidationSeverity::Warning);
        },
        engine::rendering::QueueType::Graphics,
        engine::rendering::PassPhase::Setup,
        engine::rendering::ValidationSeverity::Warning));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "GBuffer",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(depth);
            builder.write(color);
        },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
            EXPECT_EQ(context.pass_phase(), engine::rendering::PassPhase::Geometry);
            EXPECT_EQ(context.validation_severity(), engine::rendering::ValidationSeverity::Error);
        },
        engine::rendering::QueueType::Compute,
        engine::rendering::PassPhase::Geometry,
        engine::rendering::ValidationSeverity::Error));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Lighting",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.read(color); },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
            EXPECT_EQ(context.pass_phase(), engine::rendering::PassPhase::Lighting);
            EXPECT_EQ(context.validation_severity(), engine::rendering::ValidationSeverity::Info);
        },
        engine::rendering::QueueType::Graphics,
        engine::rendering::PassPhase::Lighting,
        engine::rendering::ValidationSeverity::Info));

    graph.compile();

    engine::scene::Scene scene;
    engine::rendering::MaterialSystem materials;
    NullProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::tests::NullCommandEncoderProvider command_encoders;
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider, command_encoders};
    graph.execute(context);

    ASSERT_EQ(order.size(), 3);  // NOLINT
    EXPECT_EQ(order[0], "DepthPrepass");
    EXPECT_EQ(order[1], "GBuffer");
    EXPECT_EQ(order[2], "Lighting");
}

TEST(FrameGraph, TracksResourceLifetimes)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource(make_depth_resource("Depth"));
    const auto color = graph.create_resource(make_color_resource("Color"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "DepthPrepass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(depth); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "GBuffer",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(depth);
            builder.write(color);
        },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Lighting",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.read(color); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.compile();

    engine::scene::Scene scene;
    engine::rendering::MaterialSystem materials;
    NullProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::tests::NullCommandEncoderProvider command_encoders;
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider, command_encoders};
    graph.execute(context);

    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4);  // NOLINT

    EXPECT_EQ(device_provider.frames_begun(), 1U);
    EXPECT_EQ(device_provider.frames_completed(), 1U);
    ASSERT_EQ(device_provider.acquired().size(), 2);  // NOLINT
    ASSERT_EQ(device_provider.released().size(), 2);  // NOLINT

    const auto& acquired_depth = device_provider.acquired().front().info;
    EXPECT_EQ(acquired_depth.name, "Depth");
    EXPECT_EQ(acquired_depth.format, engine::rendering::ResourceFormat::Depth24Stencil8);
    EXPECT_EQ(acquired_depth.dimension, engine::rendering::ResourceDimension::Texture2D);
    EXPECT_TRUE(engine::rendering::has_flag(acquired_depth.usage,
                                            engine::rendering::ResourceUsage::DepthStencilAttachment));
    EXPECT_EQ(acquired_depth.initial_state, engine::rendering::ResourceState::DepthStencilAttachment);
    EXPECT_EQ(acquired_depth.final_state, engine::rendering::ResourceState::DepthStencilAttachment);
    EXPECT_EQ(acquired_depth.width, 1920U);
    EXPECT_EQ(acquired_depth.height, 1080U);
    EXPECT_EQ(acquired_depth.depth, 1U);
    EXPECT_EQ(acquired_depth.array_layers, 1U);
    EXPECT_EQ(acquired_depth.mip_levels, 1U);
    EXPECT_EQ(acquired_depth.sample_count, engine::rendering::ResourceSampleCount::Count1);

    const auto& acquired_color = device_provider.acquired().back().info;
    EXPECT_EQ(acquired_color.name, "Color");
    EXPECT_EQ(acquired_color.format, engine::rendering::ResourceFormat::Rgba8Unorm);
    EXPECT_EQ(acquired_color.dimension, engine::rendering::ResourceDimension::Texture2D);
    EXPECT_TRUE(engine::rendering::has_flag(acquired_color.usage,
                                            engine::rendering::ResourceUsage::ColorAttachment));
    EXPECT_TRUE(engine::rendering::has_flag(acquired_color.usage,
                                            engine::rendering::ResourceUsage::ShaderRead));
    EXPECT_EQ(acquired_color.initial_state, engine::rendering::ResourceState::ColorAttachment);
    EXPECT_EQ(acquired_color.final_state, engine::rendering::ResourceState::ShaderRead);
    EXPECT_EQ(acquired_color.width, 1920U);
    EXPECT_EQ(acquired_color.height, 1080U);
    EXPECT_EQ(acquired_color.depth, 1U);
    EXPECT_EQ(acquired_color.array_layers, 1U);
    EXPECT_EQ(acquired_color.mip_levels, 1U);
    EXPECT_EQ(acquired_color.sample_count, engine::rendering::ResourceSampleCount::Count1);

    EXPECT_EQ(events[0].type, engine::rendering::ResourceEvent::Type::Acquire);
    EXPECT_EQ(events[0].resource_name, "Depth");
    EXPECT_EQ(events[0].pass_name, "DepthPrepass");

    EXPECT_EQ(events[1].type, engine::rendering::ResourceEvent::Type::Release);
    EXPECT_EQ(events[1].resource_name, "Depth");
    EXPECT_EQ(events[1].pass_name, "GBuffer");

    EXPECT_EQ(events[2].type, engine::rendering::ResourceEvent::Type::Acquire);
    EXPECT_EQ(events[2].resource_name, "Color");
    EXPECT_EQ(events[2].pass_name, "GBuffer");

    EXPECT_EQ(events[3].type, engine::rendering::ResourceEvent::Type::Release);
    EXPECT_EQ(events[3].resource_name, "Color");
    EXPECT_EQ(events[3].pass_name, "Lighting");
}

TEST(FrameGraph, EmitsSchedulerSubmissionsWithOrderedBarriers)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource(make_depth_resource("Depth"));
    const auto color = graph.create_resource(make_color_resource("Color"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "DepthPrepass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(depth); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "GBuffer",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(depth);
            builder.write(color);
        },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Lighting",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.read(color); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.compile();

    engine::scene::Scene scene;
    engine::rendering::MaterialSystem materials;
    NullProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::tests::NullCommandEncoderProvider command_encoders;
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider, command_encoders};
    graph.execute(context);

    const auto& submissions = scheduler.submissions;
    ASSERT_EQ(submissions.size(), 3);  // NOLINT

    EXPECT_EQ(device_provider.frames_begun(), 1U);
    EXPECT_EQ(device_provider.frames_completed(), 1U);

    const auto& depth_submission = submissions[0];
    EXPECT_EQ(depth_submission.pass_name, "DepthPrepass");
    EXPECT_TRUE(depth_submission.waits.empty());
    ASSERT_EQ(depth_submission.begin_barriers.size(), 1);  // NOLINT
    const auto depth_info = graph.resource_info(depth_submission.begin_barriers.front().resource);
    EXPECT_EQ(depth_info.name, "Depth");
    EXPECT_EQ(depth_submission.begin_barriers.front().source_access,
              engine::rendering::resources::Access::Read);
    EXPECT_EQ(depth_submission.begin_barriers.front().destination_access,
              engine::rendering::resources::Access::Write);
    ASSERT_EQ(depth_submission.end_barriers.size(), 1);  // NOLINT
    EXPECT_EQ(depth_submission.end_barriers.front().source_access,
              engine::rendering::resources::Access::Write);
    EXPECT_EQ(depth_submission.end_barriers.front().destination_access,
              engine::rendering::resources::Access::Read);
    ASSERT_EQ(depth_submission.signals.size(), 1);  // NOLINT
    EXPECT_EQ(depth_submission.signals.front().value, 1U);
    EXPECT_EQ(depth_submission.fence_value, 1U);

    const auto& gbuffer_submission = submissions[1];
    EXPECT_EQ(gbuffer_submission.pass_name, "GBuffer");
    ASSERT_EQ(gbuffer_submission.waits.size(), 1);  // NOLINT
    EXPECT_EQ(gbuffer_submission.waits.front().value, 1U);
    ASSERT_EQ(gbuffer_submission.begin_barriers.size(), 2);  // NOLINT
    const auto& gbuffer_read = gbuffer_submission.begin_barriers[0];
    EXPECT_EQ(graph.resource_info(gbuffer_read.resource).name, "Depth");
    EXPECT_EQ(gbuffer_read.source_access, engine::rendering::resources::Access::Write);
    EXPECT_EQ(gbuffer_read.destination_access, engine::rendering::resources::Access::Read);
    const auto& gbuffer_write = gbuffer_submission.begin_barriers[1];
    EXPECT_EQ(graph.resource_info(gbuffer_write.resource).name, "Color");
    EXPECT_EQ(gbuffer_write.source_access, engine::rendering::resources::Access::Read);
    EXPECT_EQ(gbuffer_write.destination_access, engine::rendering::resources::Access::Write);
    ASSERT_EQ(gbuffer_submission.end_barriers.size(), 1);  // NOLINT
    EXPECT_EQ(graph.resource_info(gbuffer_submission.end_barriers.front().resource).name, "Color");
    EXPECT_EQ(gbuffer_submission.end_barriers.front().source_access,
              engine::rendering::resources::Access::Write);
    EXPECT_EQ(gbuffer_submission.end_barriers.front().destination_access,
              engine::rendering::resources::Access::Read);
    ASSERT_EQ(gbuffer_submission.signals.size(), 1);  // NOLINT
    EXPECT_EQ(gbuffer_submission.signals.front().value, 2U);
    EXPECT_EQ(gbuffer_submission.fence_value, 2U);

    const auto& lighting_submission = submissions[2];
    EXPECT_EQ(lighting_submission.pass_name, "Lighting");
    ASSERT_EQ(lighting_submission.waits.size(), 1);  // NOLINT
    EXPECT_EQ(lighting_submission.waits.front().value, 2U);
    ASSERT_EQ(lighting_submission.begin_barriers.size(), 1);  // NOLINT
    EXPECT_EQ(graph.resource_info(lighting_submission.begin_barriers.front().resource).name, "Color");
    EXPECT_EQ(lighting_submission.begin_barriers.front().source_access,
              engine::rendering::resources::Access::Write);
    EXPECT_EQ(lighting_submission.begin_barriers.front().destination_access,
              engine::rendering::resources::Access::Read);
    EXPECT_TRUE(lighting_submission.end_barriers.empty());
    ASSERT_EQ(lighting_submission.signals.size(), 1);  // NOLINT
    EXPECT_EQ(lighting_submission.signals.front().value, 3U);
    EXPECT_EQ(lighting_submission.fence_value, 3U);
}

TEST(FrameGraph, PassHonorsQueuePreference)
{
    engine::rendering::FrameGraph graph;
    const auto color = graph.create_resource(make_color_resource("ComputeColor"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "ComputePass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(color); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {},
        engine::rendering::QueueType::Compute));

    graph.compile();

    engine::scene::Scene scene;
    engine::rendering::MaterialSystem materials;
    NullProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::tests::NullCommandEncoderProvider command_encoders;
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider, command_encoders};
    graph.execute(context);

    ASSERT_EQ(scheduler.submissions.size(), 1);  // NOLINT
    EXPECT_EQ(scheduler.submissions.front().queue, engine::rendering::QueueType::Compute);
}

TEST(FrameGraph, BuilderRejectsInvalidHandles)
{
    engine::rendering::FrameGraph graph;

    auto make_pass = [](auto setup) {
        return std::make_unique<engine::rendering::CallbackRenderPass>(
            "Invalid", std::move(setup), [](engine::rendering::FrameGraphPassExecutionContext&) {});
    };

    EXPECT_THROW(
        graph.add_pass(make_pass([](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(engine::rendering::FrameGraphResourceHandle{});
        })),
        std::out_of_range);

    EXPECT_THROW(
        graph.add_pass(make_pass([](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.write(engine::rendering::FrameGraphResourceHandle{});
        })),
        std::out_of_range);
}

TEST(FrameGraph, RejectsMissingResourceMetadata)
{
    engine::rendering::FrameGraph graph;
    engine::rendering::FrameGraphResourceDescriptor descriptor{};
    descriptor.name = "Invalid";
    const auto handle = graph.create_resource(descriptor);

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Writer", [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(handle); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    EXPECT_THROW(graph.compile(), std::logic_error);
}

TEST(FrameGraph, PreventsMultipleWritersForResource)
{
    engine::rendering::FrameGraph graph;
    const auto handle = graph.create_resource(make_color_resource("Color"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "WriterA",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(handle); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    EXPECT_THROW(
        graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
            "WriterB",
            [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(handle); },
            [](engine::rendering::FrameGraphPassExecutionContext&) {})),
        std::logic_error);
}

TEST(FrameGraph, DetectsCyclesDuringCompile)
{
    engine::rendering::FrameGraph graph;
    const auto a = graph.create_resource(make_color_resource("A"));
    const auto b = graph.create_resource(make_color_resource("B"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "PassA",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.write(a);
            builder.read(b);
        },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "PassB",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.write(b);
            builder.read(a);
        },
        [](engine::rendering::FrameGraphPassExecutionContext&) {}));

    EXPECT_THROW(graph.compile(), std::logic_error);
}

TEST(FrameGraph, SerializesDeterministically)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource(make_depth_resource("Depth"));
    const auto color = graph.create_resource(make_color_resource("Color"));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "DepthPrepass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(depth); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {},
        engine::rendering::QueueType::Graphics,
        engine::rendering::PassPhase::Setup,
        engine::rendering::ValidationSeverity::Warning));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "GBuffer",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(depth);
            builder.write(color);
        },
        [](engine::rendering::FrameGraphPassExecutionContext&) {},
        engine::rendering::QueueType::Compute,
        engine::rendering::PassPhase::Geometry,
        engine::rendering::ValidationSeverity::Error));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Lighting",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.read(color); },
        [](engine::rendering::FrameGraphPassExecutionContext&) {},
        engine::rendering::QueueType::Graphics,
        engine::rendering::PassPhase::Lighting,
        engine::rendering::ValidationSeverity::Info));

    graph.compile();

    const auto first = graph.serialize();
    const auto second = graph.serialize();
    EXPECT_EQ(first, second);

    const std::string expected = R"({
  "resources": [
    {
      "name": "Depth",
      "lifetime": "Transient",
      "format": "Depth24Stencil8",
      "dimension": "Texture2D",
      "width": 1920,
      "height": 1080,
      "depth": 1,
      "array_layers": 1,
      "mip_levels": 1,
      "sample_count": 1,
      "size_bytes": 0,
      "usage": "DepthStencil",
      "initial_state": "DepthStencilAttachment",
      "final_state": "DepthStencilAttachment"
    },
    {
      "name": "Color",
      "lifetime": "Transient",
      "format": "Rgba8Unorm",
      "dimension": "Texture2D",
      "width": 1920,
      "height": 1080,
      "depth": 1,
      "array_layers": 1,
      "mip_levels": 1,
      "sample_count": 1,
      "size_bytes": 0,
      "usage": "ShaderRead|ColorAttachment",
      "initial_state": "ColorAttachment",
      "final_state": "ShaderRead"
    }
  ],
  "passes": [
    {
      "name": "DepthPrepass",
      "queue": "Graphics",
      "phase": "Setup",
      "validation": "Warning",
      "reads": [],
      "writes": ["Depth"]
    },
    {
      "name": "GBuffer",
      "queue": "Compute",
      "phase": "Geometry",
      "validation": "Error",
      "reads": ["Depth"],
      "writes": ["Color"]
    },
    {
      "name": "Lighting",
      "queue": "Graphics",
      "phase": "Lighting",
      "validation": "Info",
      "reads": ["Color"],
      "writes": []
    }
  ],
  "execution_order": ["DepthPrepass", "GBuffer", "Lighting"]
}
)";

    EXPECT_EQ(first, expected);
}

TEST(FrameGraph, ResourceInfoRejectsInvalidHandle)
{
    engine::rendering::FrameGraph graph;
    EXPECT_THROW(graph.resource_info(engine::rendering::FrameGraphResourceHandle{}), std::out_of_range);
}
