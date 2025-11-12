#pragma once

#include "engine/geometry/export.hpp"
#include "engine/math/math.hpp"
#include "engine/geometry/shapes/aabb.hpp"

#include <filesystem>

namespace engine::geometry
{

    namespace mesh
    {
        struct IOFlags;
    } // namespace mesh

    struct SurfaceMesh
    {
        std::vector<math::vec3> rest_positions;
        std::vector<math::vec3> positions;
        std::vector<math::vec3> normals;
        std::vector<std::uint32_t> indices;
        std::vector<math::vec2> texture_coordinates;
        Aabb bounds{};
    };

    ENGINE_GEOMETRY_API void recompute_vertex_normals(SurfaceMesh& mesh);

    ENGINE_GEOMETRY_API void update_bounds(SurfaceMesh& mesh);

    //TODO Task: refactor this to be apply_transform(SurfaceMesh &mesh, const math::mat4 &transform);
    ENGINE_GEOMETRY_API void apply_uniform_translation(SurfaceMesh& mesh, const math::vec3& translation);

    [[nodiscard]] ENGINE_GEOMETRY_API math::vec3 centroid(const SurfaceMesh& mesh);

    [[nodiscard]] ENGINE_GEOMETRY_API float surface_area(const SurfaceMesh& mesh) noexcept;

    [[nodiscard]] ENGINE_GEOMETRY_API SurfaceMesh load_surface_mesh(const std::filesystem::path& path);

    ENGINE_GEOMETRY_API void save_surface_mesh(const SurfaceMesh& mesh, const std::filesystem::path& path);

    ENGINE_GEOMETRY_API void save_surface_mesh(const SurfaceMesh& mesh,
                                           const std::filesystem::path& path,
                                           const mesh::IOFlags& flags);
}