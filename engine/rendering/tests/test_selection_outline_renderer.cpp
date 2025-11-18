#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "engine/assets/handles.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/rendering/selection_outline_renderer.hpp"
#include "command_encoder_test_utils.hpp"
#include "scheduler_test_utils.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/selection/selection_engine.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering::tests
{
    namespace
    {
        class NullProvider final : public RenderResourceProvider
        {
        public:
            void require_mesh(const assets::MeshHandle&) override {}
            void require_graph(const assets::GraphHandle&) override {}
            void require_point_cloud(const assets::PointCloudHandle&) override {}
            void require_material(const assets::MaterialHandle&) override {}
            void require_shader(const assets::ShaderHandle&) override {}
        };

        [[nodiscard]] FrameGraphResourceHandle create_color(FrameGraph& graph)
        {
            FrameGraphResourceDescriptor descriptor{};
            descriptor.name = "SelectionOutline.Color";
            descriptor.format = ResourceFormat::Rgba16f;
            descriptor.dimension = ResourceDimension::Texture2D;
            descriptor.usage = ResourceUsage::ColorAttachment | ResourceUsage::ShaderRead;
            descriptor.initial_state = ResourceState::ColorAttachment;
            descriptor.final_state = ResourceState::ShaderRead;
            descriptor.width = 256;
            descriptor.height = 256;
            descriptor.sample_count = ResourceSampleCount::Count1;
            return graph.create_resource(std::move(descriptor));
        }

        [[nodiscard]] FrameGraphResourceHandle create_depth(FrameGraph& graph)
        {
            FrameGraphResourceDescriptor descriptor{};
            descriptor.name = "SelectionOutline.Depth";
            descriptor.format = ResourceFormat::Depth24Stencil8;
            descriptor.dimension = ResourceDimension::Texture2D;
            descriptor.usage = ResourceUsage::DepthStencilAttachment;
            descriptor.initial_state = ResourceState::DepthStencilAttachment;
            descriptor.final_state = ResourceState::DepthStencilAttachment;
            descriptor.width = 256;
            descriptor.height = 256;
            descriptor.sample_count = ResourceSampleCount::Count1;
            return graph.create_resource(std::move(descriptor));
        }

        struct ExecutionHarness
        {
            NullProvider provider{};
            MaterialSystem materials{};
            resources::RecordingGpuResourceProvider device_resources{};
            tests::RecordingScheduler scheduler{};
            tests::RecordingCommandEncoderProvider encoders{};
        };

        [[nodiscard]] RenderExecutionContext make_context(ExecutionHarness& harness, scene::Scene& scene)
        {
            return RenderExecutionContext{
                harness.provider,
                harness.materials,
                RenderView{scene},
                harness.scheduler,
                harness.device_resources,
                harness.encoders,
            };
        }
    } // namespace

    TEST(SelectionOutlineRenderer, AddsPassToFrameGraph)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        renderer.add_pass(graph, color, depth);

        ASSERT_NO_THROW(graph.compile());
        const auto order = graph.execution_order();
        ASSERT_EQ(order.size(), 1U);
        EXPECT_EQ(graph.pass_name(order[0]), "Selection.Outline");
        EXPECT_FALSE(renderer.strategy_name().empty());
    }

    TEST(SelectionOutlineRenderer, SelectsJumpFloodForHighQuality)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        renderer.config().quality = scene::selection::visualization::OutlineQuality::High;
        renderer.add_pass(graph, color, depth);
        graph.compile();

        EXPECT_EQ(renderer.strategy_name(), "JumpFlood");
    }

    TEST(SelectionOutlineRenderer, SelectsEdgeDetectionForFastQuality)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        renderer.config().quality = scene::selection::visualization::OutlineQuality::Fast;
        renderer.add_pass(graph, color, depth);
        graph.compile();

        EXPECT_EQ(renderer.strategy_name(), "EdgeDetection");
    }

    TEST(SelectionOutlineRenderer, StrategyOverrideWins)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        renderer.set_strategy_override("JumpFlood");
        renderer.config().quality = scene::selection::visualization::OutlineQuality::Fast;
        renderer.add_pass(graph, color, depth);
        graph.compile();

        EXPECT_EQ(renderer.strategy_name(), "JumpFlood");
    }

    TEST(SelectionOutlineRenderer, ListsAvailableStrategies)
    {
        const auto strategies = SelectionOutlineRenderer::available_strategies();
        EXPECT_FALSE(strategies.empty());
        EXPECT_NE(std::find(strategies.begin(), strategies.end(), "JumpFlood"), strategies.end());
    }

    TEST(SelectionOutlineRenderer, EmitsOutlineDrawsForSelections)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        scene::selection::SelectionEngine engine;
        renderer.set_selection_engine(&engine);
        renderer.config().style.thickness = 6.0F;
        renderer.config().style.color = engine::math::vec3{0.9F, 0.1F, 0.2F};
        renderer.add_pass(graph, color, depth);
        graph.compile();

        scene::Scene scene;
        auto entity = scene.create_entity();
        scene.registry().emplace<scene::components::WorldTransform>(entity.id());
        assets::MeshHandle mesh_handle{std::string{"selection_outline.mesh"}};
        scene.registry().emplace<components::RenderGeometry>(
            entity.id(), components::RenderGeometry::from_mesh(mesh_handle));

        scene::selection::SelectionEvent event{};
        event.hit.entity = entity.id();
        event.hit.distance = 2.0F;
        engine.push_selection(event);

        ExecutionHarness harness{};
        auto context = make_context(harness, scene);
        graph.execute(context);

        ASSERT_EQ(harness.encoders.completed_encoders.size(), 1U);
        const auto& draws = harness.encoders.completed_encoders.front()->geometry_draws();
        ASSERT_FALSE(draws.empty());
        EXPECT_TRUE(draws.front().has_color_override);
        const float expected_alpha = std::min(renderer.config().style.alpha,
                                              renderer.config().occluded_style.alpha);
        EXPECT_NEAR(draws.front().alpha_override, expected_alpha, 1e-3F);
        EXPECT_GT(draws.front().transform.scale[0], 1.0F);
    }

    TEST(SelectionOutlineRenderer, SkipsEncodingWhenDisabled)
    {
        FrameGraph graph;
        auto color = create_color(graph);
        auto depth = create_depth(graph);

        SelectionOutlineRenderer renderer;
        scene::selection::SelectionEngine engine;
        renderer.set_selection_engine(&engine);
        renderer.set_enabled(false);
        renderer.add_pass(graph, color, depth);
        graph.compile();

        scene::Scene scene;
        auto entity = scene.create_entity();
        scene.registry().emplace<scene::components::WorldTransform>(entity.id());
        assets::MeshHandle mesh_handle{std::string{"selection_outline.mesh"}};
        scene.registry().emplace<components::RenderGeometry>(
            entity.id(), components::RenderGeometry::from_mesh(mesh_handle));

        scene::selection::SelectionEvent event{};
        event.hit.entity = entity.id();
        engine.push_selection(event);

        ExecutionHarness harness{};
        auto context = make_context(harness, scene);
        graph.execute(context);

        if (!harness.encoders.completed_encoders.empty())
        {
            const auto& draws = harness.encoders.completed_encoders.front()->geometry_draws();
            EXPECT_TRUE(draws.empty());
        }
    }
} // namespace engine::rendering::tests
