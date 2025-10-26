#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"

namespace engine::rendering::tests
{
    namespace
    {
        void compile(FrameGraph& graph)
        {
            ASSERT_NO_THROW(graph.compile());
        }
    }

    TEST(ResearchBaselinePreset, DeferredCreatesGBufferAndLightingPass)
    {
        FrameGraph graph;
        ResearchBaselineOptions options{};
        options.shading_mode = ResearchShadingMode::Deferred;
        options.width = 1280;
        options.height = 720;

        const auto resources = configure_research_baseline(graph, options);
        compile(graph);

        ASSERT_TRUE(resources.gbuffer_albedo.has_value());
        ASSERT_TRUE(resources.gbuffer_normals.has_value());
        ASSERT_TRUE(resources.gbuffer_material.has_value());

        const auto& albedo_info = graph.resource_info(*resources.gbuffer_albedo);
        EXPECT_EQ(albedo_info.name, "Research.GBuffer.Albedo");
        EXPECT_EQ(albedo_info.width, options.width);
        EXPECT_EQ(albedo_info.height, options.height);
        EXPECT_EQ(albedo_info.format, ResourceFormat::Rgba16f);

        const auto order = graph.execution_order();
        ASSERT_EQ(order.size(), 2);
        EXPECT_EQ(graph.pass_name(order[0]), "Research.GBuffer");
        EXPECT_EQ(graph.pass_name(order[1]), "Research.LightingComposite");
    }

    TEST(ResearchBaselinePreset, ForwardCreatesSingleGeometryPass)
    {
        FrameGraph graph;
        ResearchBaselineOptions options{};
        options.shading_mode = ResearchShadingMode::Forward;

        const auto resources = configure_research_baseline(graph, options);
        compile(graph);

        EXPECT_FALSE(resources.gbuffer_albedo.has_value());
        EXPECT_FALSE(resources.gbuffer_normals.has_value());
        EXPECT_FALSE(resources.gbuffer_material.has_value());

        const auto color_info = graph.resource_info(resources.lighting_output);
        EXPECT_EQ(color_info.name, "Research.FinalColor");
        EXPECT_EQ(color_info.format, ResourceFormat::Rgba16f);

        const auto order = graph.execution_order();
        ASSERT_EQ(order.size(), 1);
        EXPECT_EQ(graph.pass_name(order[0]), "Research.ForwardGeometry");
    }

    TEST(ResearchBaselinePreset, AddsOverlayPassesWhenEnabled)
    {
        FrameGraph graph;
        ResearchBaselineOptions options{};
        options.shading_mode = ResearchShadingMode::Forward;
        options.enable_normals_overlay = true;
        options.enable_uv_overlay = true;
        options.enable_material_overlay = true;
        options.enable_light_volume_overlay = true;

        const auto resources = configure_research_baseline(graph, options);
        compile(graph);

        ASSERT_TRUE(resources.debug_normals_overlay.has_value());
        ASSERT_TRUE(resources.debug_uv_overlay.has_value());
        ASSERT_TRUE(resources.debug_material_overlay.has_value());
        ASSERT_TRUE(resources.debug_light_volume_overlay.has_value());

        const auto order = graph.execution_order();
        ASSERT_EQ(order.size(), 5);

        std::vector<std::string_view> pass_names;
        pass_names.reserve(order.size());
        for (const auto index : order)
        {
            pass_names.push_back(graph.pass_name(index));
        }

        const std::array<std::string_view, 5> expected{
            "Research.ForwardGeometry",
            "Research.Debug.Normals",
            "Research.Debug.UV",
            "Research.Debug.Material",
            "Research.Debug.LightVolume",
        };

        EXPECT_EQ(pass_names, std::vector<std::string_view>(expected.begin(), expected.end()));
    }
}
