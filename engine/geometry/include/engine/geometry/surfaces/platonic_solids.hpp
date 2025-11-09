#pragma once

#include "engine/geometry/export.hpp"

namespace engine::geometry
{
    struct SurfaceMesh;

    /// Generate a regular tetrahedron (4 vertices, 4 triangular faces)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh tetrahedron();

    /// Generate a regular octahedron (6 vertices, 8 triangular faces)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh octahedron();

    /// Generate a regular dodecahedron (20 vertices, 12 pentagonal faces triangulated to 36 triangles)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh dodecahedron();

    /// Generate a regular icosahedron (12 vertices, 20 triangular faces)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh icosahedron();
} // namespace engine::geometry