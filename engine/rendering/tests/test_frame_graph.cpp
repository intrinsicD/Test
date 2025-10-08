#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/scene/scene.hpp"

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
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene}};
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
    engine::rendering::RenderExecutionContext context{provider, materials, engine::rendering::RenderView{scene}};
    graph.execute(context);

    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4);  // NOLINT

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
