#pragma once

#include <vector>

#include "engine/geometry/export.hpp"

namespace engine::geometry
{
    struct SurfaceMesh;

    struct SurfaceCurvatureResult
    {
        std::vector<float> mean_curvature;
        std::vector<float> gaussian_curvature;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceCurvatureResult ComputeSurfaceCurvature(
        const SurfaceMesh& mesh) noexcept;
} // namespace engine::geometry
