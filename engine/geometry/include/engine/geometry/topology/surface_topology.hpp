#pragma once

#include <cstdint>
#include <vector>

#include "engine/geometry/export.hpp"
#include "engine/math/common.hpp"

namespace engine::geometry
{
    struct SurfaceMesh;

    struct SurfaceEdgeTag
    {
        std::uint32_t v0{0};
        std::uint32_t v1{0};
        float dihedral_angle{0.0F};
        bool is_boundary{false};
        bool is_crease{false};
        bool is_non_manifold{false};
    };

    struct SurfaceVertexTag
    {
        bool is_boundary{false};
        bool is_feature{false};
    };

    struct SurfaceTopologySummary
    {
        std::vector<SurfaceEdgeTag> edges;
        std::vector<SurfaceVertexTag> vertices;
    };

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceTopologySummary AnalyzeSurfaceTopology(
        const SurfaceMesh& mesh,
        float crease_angle_radians = math::radians(45.0F)) noexcept;
} // namespace engine::geometry
