#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/export.hpp"

#include <cstddef>

namespace engine::geometry
{
    struct ENGINE_GEOMETRY_API SurfaceDeviationMetrics
    {
        float max_distance{0.0f};
        float mean_distance{0.0f};
        float rms_distance{0.0f};
        float reference_to_candidate_max{0.0f};
        float candidate_to_reference_max{0.0f};
        std::size_t sample_count{0U};
    };

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceDeviationMetrics ComputeSurfaceDeviationMetrics(
        const SurfaceMesh& reference,
        const SurfaceMesh& candidate) noexcept;
}