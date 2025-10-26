#pragma once

#include <cstdint>
#include <optional>

#include "engine/rendering/frame_graph.hpp"

namespace engine::rendering
{
    enum class ResearchShadingMode
    {
        Forward,
        Deferred,
    };

    struct ResearchBaselineOptions
    {
        ResearchShadingMode shading_mode{ResearchShadingMode::Deferred};
        std::uint32_t width{1920};
        std::uint32_t height{1080};
        bool enable_normals_overlay{false};
        bool enable_uv_overlay{false};
        bool enable_material_overlay{false};
        bool enable_light_volume_overlay{false};
    };

    struct ResearchBaselineResources
    {
        FrameGraphResourceHandle lighting_output{};
        FrameGraphResourceHandle depth{};
        std::optional<FrameGraphResourceHandle> gbuffer_albedo;
        std::optional<FrameGraphResourceHandle> gbuffer_normals;
        std::optional<FrameGraphResourceHandle> gbuffer_material;
        std::optional<FrameGraphResourceHandle> debug_normals_overlay;
        std::optional<FrameGraphResourceHandle> debug_uv_overlay;
        std::optional<FrameGraphResourceHandle> debug_material_overlay;
        std::optional<FrameGraphResourceHandle> debug_light_volume_overlay;
    };

    ResearchBaselineResources configure_research_baseline(FrameGraph& graph,
                                                           const ResearchBaselineOptions& options);
}
