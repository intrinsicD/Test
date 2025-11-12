#pragma once

#include <string_view>

#include "engine/geometry/export.hpp"
#include "engine/geometry/mesh/surface_mesh.hpp"

namespace engine::geometry
{
    [[nodiscard]] ENGINE_GEOMETRY_API std::string_view module_name() noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh make_unit_quad();

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh make_unit_cube();

    /// Create a SurfaceMesh from an AABB shape (reuses existing shape utilities)
    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh mesh_from_aabb(const Aabb& box);
} // namespace engine::geometry


extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept;
