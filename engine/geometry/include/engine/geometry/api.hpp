#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/geometry/export.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/math/math.hpp"

namespace engine::geometry {

namespace mesh
{
    struct IOFlags;
} // namespace mesh

struct SurfaceMesh {
    std::vector<math::vec3> rest_positions;
    std::vector<math::vec3> positions;
    std::vector<math::vec3> normals;
    std::vector<std::uint32_t> indices;
    Aabb bounds{};
};

[[nodiscard]] ENGINE_GEOMETRY_API std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh make_unit_quad();

ENGINE_GEOMETRY_API void recompute_vertex_normals(SurfaceMesh& mesh);

ENGINE_GEOMETRY_API void update_bounds(SurfaceMesh& mesh);

ENGINE_GEOMETRY_API void apply_uniform_translation(SurfaceMesh& mesh, const math::vec3& translation);

[[nodiscard]] ENGINE_GEOMETRY_API math::vec3 centroid(const SurfaceMesh& mesh);

[[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh load_surface_mesh(const std::filesystem::path& path);

ENGINE_GEOMETRY_API void save_surface_mesh(const SurfaceMesh& mesh, const std::filesystem::path& path);

ENGINE_GEOMETRY_API void save_surface_mesh(const SurfaceMesh& mesh,
                                           const std::filesystem::path& path,
                                           const mesh::IOFlags& flags);

}  // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept;
