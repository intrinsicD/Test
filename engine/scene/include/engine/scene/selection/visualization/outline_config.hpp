#pragma once

#include <cstdint>

#include "engine/math/vector.hpp"

namespace engine::scene::selection::visualization
{
    enum class ThicknessMode
    {
        ScreenSpaceUniform,
        WorldSpaceUniform,
        DepthWeighted,
        HybridAdaptive,
    };

    enum class OcclusionMode
    {
        Disabled,
        Hidden,
        Faded,
        XRay,
        XRayPulsing,
    };

    enum class MultiSelectionMode
    {
        Merged,
        Distinct,
        ColorCoded,
        PriorityOrdered,
        Grouped,
    };

    enum class TransparencyHandling
    {
        Ignore,
        Opaque,
        Blended,
        Separate,
    };

    enum class OutlineQuality
    {
        Fast,
        Balanced,
        High,
        Ultra,
    };

    struct OutlineStyle
    {
        engine::math::vec3 color{1.0F, 0.5F, 0.0F};
        float thickness{3.0F};
        float alpha{1.0F};

        bool animated{false};
        float pulse_speed{2.0F};
        float pulse_amplitude{0.4F};

        bool dashed{false};
        float dash_length{10.0F};
        float dash_speed{5.0F};

        bool glow{false};
        float glow_intensity{0.5F};
        float glow_falloff{2.0F};

        bool use_entity_color{false};
        bool use_selection_order_gradient{false};
        engine::math::vec3 gradient_end_color{0.0F, 0.5F, 1.0F};
    };

    namespace detail
    {
        inline OutlineStyle make_default_occluded_style()
        {
            OutlineStyle style{};
            style.color = engine::math::vec3{0.3F, 0.3F, 0.5F};
            style.alpha = 0.3F;
            style.animated = false;
            style.dashed = false;
            style.glow = false;
            return style;
        }
    } // namespace detail

    struct OutlineConfig
    {
        ThicknessMode thickness_mode{ThicknessMode::ScreenSpaceUniform};
        OcclusionMode occlusion_mode{OcclusionMode::Faded};
        MultiSelectionMode multi_selection_mode{MultiSelectionMode::Distinct};
        TransparencyHandling transparency_mode{TransparencyHandling::Opaque};
        OutlineQuality quality{OutlineQuality::Balanced};

        OutlineStyle style{};
        OutlineStyle occluded_style{detail::make_default_occluded_style()};

        float world_space_thickness{0.05F};
        float depth_weight_factor{0.01F};
        float min_screen_thickness{1.0F};
        float max_screen_thickness{10.0F};

        float near_distance{5.0F};
        float far_distance{100.0F};

        float jfa_resolution_scale{1.0F};
        bool async_compute{false};
        bool cache_when_static{true};
        int max_jfa_iterations{12};

        bool visualize_distance_field{false};
        bool show_seed_points{false};
    };
} // namespace engine::scene::selection::visualization
