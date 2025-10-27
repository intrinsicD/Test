#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "engine/assets/handles.hpp"
#include "engine/core/memory/resource_pool.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/rendering/pipeline/research_baseline_telemetry.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/rendering/material_system.hpp"
#include "command_encoder_test_utils.hpp"
#include "scheduler_test_utils.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace engine::rendering::tests
{
    namespace
    {
        void compile(FrameGraph& graph)
        {
            ASSERT_NO_THROW(graph.compile());
        }

        class NullProvider final : public RenderResourceProvider
        {
        public:
            void require_mesh(const assets::MeshHandle&) override {}
            void require_graph(const assets::GraphHandle&) override {}
            void require_point_cloud(const assets::PointCloudHandle&) override {}
            void require_material(const assets::MaterialHandle&) override {}
            void require_shader(const assets::ShaderHandle&) override {}
        };
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

    TEST(ResearchBaselineTelemetry, RecordsPassAndShadingMetrics)
    {
        ResearchBaselineTelemetry::instance().reset_for_testing();

        FrameGraph graph;
        ResearchBaselineOptions options{};
        options.shading_mode = ResearchShadingMode::Deferred;
        options.enable_normals_overlay = true;
        options.enable_material_overlay = true;

        configure_research_baseline(graph, options);
        compile(graph);

        scene::Scene scene{};
        auto entity = scene.create_entity();
        entity.emplace<scene::components::WorldTransform>();

        core::memory::ResourcePool<int, assets::MeshHandleTag> mesh_pool;
        auto [mesh_handle, value] = mesh_pool.acquire(1);
        (void)value;
        assets::MeshHandle mesh_identifier{std::string{"telemetry://mesh"}};
        mesh_identifier.bind(mesh_handle);
        entity.emplace<components::RenderGeometry>(components::RenderGeometry::from_mesh(mesh_identifier));

        MaterialSystem materials{};
        NullProvider provider{};
        resources::RecordingGpuResourceProvider device_provider{};
        tests::RecordingScheduler scheduler{};
        tests::RecordingCommandEncoderProvider command_encoders{};
        RenderExecutionContext context{provider, materials, RenderView{scene},
                                       scheduler, device_provider, command_encoders};

        graph.execute(context);

        const auto snapshot = ResearchBaselineTelemetry::instance().snapshot();
        EXPECT_EQ(snapshot.active_mode, ResearchShadingMode::Deferred);
        EXPECT_GE(snapshot.mode_selection_counts[1], 1U);

        const auto overlay_index = [](ResearchOverlay overlay) {
            return static_cast<std::size_t>(overlay);
        };

        EXPECT_TRUE(snapshot.overlays_enabled[overlay_index(ResearchOverlay::Normals)]);
        EXPECT_FALSE(snapshot.overlays_enabled[overlay_index(ResearchOverlay::Uv)]);
        EXPECT_TRUE(snapshot.overlays_enabled[overlay_index(ResearchOverlay::Material)]);
        EXPECT_FALSE(snapshot.overlays_enabled[overlay_index(ResearchOverlay::LightVolume)]);

        EXPECT_GE(snapshot.overlay_selection_counts[overlay_index(ResearchOverlay::Normals)], 1U);
        EXPECT_EQ(snapshot.overlay_selection_counts[overlay_index(ResearchOverlay::Uv)], 0U);
        EXPECT_GE(snapshot.overlay_selection_counts[overlay_index(ResearchOverlay::Material)], 1U);

        const auto find_pass = [&](std::string_view name) {
            return std::find_if(snapshot.passes.begin(), snapshot.passes.end(),
                                [&](const ResearchBaselinePassTelemetry& telemetry) {
                                    return telemetry.name == name;
                                });
        };

        const auto gbuffer = find_pass("Research.GBuffer");
        ASSERT_NE(gbuffer, snapshot.passes.end());
        EXPECT_EQ(gbuffer->last_draw_calls, 1U);
        EXPECT_EQ(gbuffer->total_draw_calls, 1U);
        EXPECT_GE(gbuffer->last_gpu_time_ms, 0.0);

        const auto lighting = find_pass("Research.LightingComposite");
        ASSERT_NE(lighting, snapshot.passes.end());
        EXPECT_EQ(lighting->last_draw_calls, 0U);
        EXPECT_GE(lighting->last_gpu_time_ms, 0.0);

        const auto& encoders = command_encoders.completed_encoders;
        ASSERT_FALSE(encoders.empty());
        EXPECT_EQ(encoders.front()->draws.size(), 1U);
    }
}
