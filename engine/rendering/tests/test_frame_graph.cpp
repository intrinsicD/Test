#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/scene/scene.hpp"
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
}

TEST(FrameGraph, SchedulesPassesBasedOnDependencies)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource("Depth");
    const auto color = graph.create_resource("Color");

    std::vector<std::string> order;

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "DepthPrepass",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.write(depth); },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
        }));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "GBuffer",
        [=](engine::rendering::FrameGraphPassBuilder& builder) {
            builder.read(depth);
            builder.write(color);
        },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
        }));

    graph.add_pass(std::make_unique<engine::rendering::CallbackRenderPass>(
        "Lighting",
        [=](engine::rendering::FrameGraphPassBuilder& builder) { builder.read(color); },
        [&](engine::rendering::FrameGraphPassExecutionContext& context) {
            order.emplace_back(context.pass_name());
        }));

    graph.compile();

    engine::scene::Scene scene;
    engine::rendering::MaterialSystem materials;
    NullProvider provider;
    engine::rendering::resources::RecordingGpuResourceProvider device_provider;
    engine::rendering::tests::RecordingScheduler scheduler;
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider};
    graph.execute(context);

    ASSERT_EQ(order.size(), 3);  // NOLINT
    EXPECT_EQ(order[0], "DepthPrepass");
    EXPECT_EQ(order[1], "GBuffer");
    EXPECT_EQ(order[2], "Lighting");
}

TEST(FrameGraph, TracksResourceLifetimes)
{
    engine::rendering::FrameGraph graph;
    const auto depth = graph.create_resource("Depth");
    const auto color = graph.create_resource("Color");

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
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider};
    graph.execute(context);

    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4);  // NOLINT

    EXPECT_EQ(device_provider.frames_begun(), 1U);
    EXPECT_EQ(device_provider.frames_completed(), 1U);
    ASSERT_EQ(device_provider.acquired().size(), 2);  // NOLINT
    ASSERT_EQ(device_provider.released().size(), 2);  // NOLINT

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
    const auto depth = graph.create_resource("Depth");
    const auto color = graph.create_resource("Color");

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
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene},
                                                     scheduler, device_provider};
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

TEST(FrameGraph, PreventsMultipleWritersForResource)
{
    engine::rendering::FrameGraph graph;
    const auto handle = graph.create_resource("Color");

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
    const auto a = graph.create_resource("A");
    const auto b = graph.create_resource("B");

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

TEST(FrameGraph, ResourceInfoRejectsInvalidHandle)
{
    engine::rendering::FrameGraph graph;
    EXPECT_THROW(graph.resource_info(engine::rendering::FrameGraphResourceHandle{}), std::out_of_range);
}
