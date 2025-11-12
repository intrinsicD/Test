#include "engine/geometry/api.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include "engine/geometry/shapes/aabb.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <cmath>

namespace engine::geometry
{
    std::string_view module_name() noexcept
    {
        return "geometry";
    }

    SurfaceMesh make_unit_quad()
    {
        SurfaceMesh mesh;
        mesh.rest_positions = {
            math::vec3{-0.5F, 0.0F, -0.5F},
            math::vec3{0.5F, 0.0F, -0.5F},
            math::vec3{0.5F, 0.0F, 0.5F},
            math::vec3{-0.5F, 0.0F, 0.5F},
        };
        mesh.positions = mesh.rest_positions;
        mesh.indices = {0, 1, 2, 0, 2, 3};
        mesh.normals.assign(mesh.positions.size(), math::vec3{0.0F, 1.0F, 0.0F});
        mesh.texture_coordinates = {
            math::vec2{0.0F, 0.0F},
            math::vec2{1.0F, 0.0F},
            math::vec2{1.0F, 1.0F},
            math::vec2{0.0F, 1.0F},
        };
        update_bounds(mesh);
        return mesh;
    }

    SurfaceMesh mesh_from_aabb(const Aabb& box)
    {
        // Get corner vertices and face quads from AABB utilities
        auto corners = GetCorners(box);
        auto face_quads = GetFaceQuads(box);

        SurfaceMesh mesh;

        // We need to duplicate vertices for proper per-face normals
        // Each face gets 4 vertices
        mesh.rest_positions.reserve(24);
        mesh.normals.reserve(24);
        mesh.indices.reserve(36);

        // Face normals for a box
        const std::array<math::vec3, 6> face_normals = {{
            math::vec3{ 0.0F,  0.0F,  1.0F},  // Front (+Z)
            math::vec3{ 0.0F,  0.0F, -1.0F},  // Back (-Z)
            math::vec3{ 1.0F,  0.0F,  0.0F},  // Right (+X)
            math::vec3{-1.0F,  0.0F,  0.0F},  // Left (-X)
            math::vec3{ 0.0F,  1.0F,  0.0F},  // Top (+Y)
            math::vec3{ 0.0F, -1.0F,  0.0F},  // Bottom (-Y)
        }};

        // Build mesh from AABB face quads
        for (std::size_t face_idx = 0; face_idx < face_quads.size(); ++face_idx)
        {
            const auto& quad = face_quads[face_idx];
            const auto& normal = face_normals[face_idx];

            const std::uint32_t base_vertex = static_cast<std::uint32_t>(mesh.rest_positions.size());

            // Add 4 vertices for this face
            for (int i = 0; i < 4; ++i)
            {
                mesh.rest_positions.push_back(corners[quad[i]]);
                mesh.normals.push_back(normal);
            }

            // Add 2 triangles (quad = 2 tris)
            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 1);
            mesh.indices.push_back(base_vertex + 2);

            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 2);
            mesh.indices.push_back(base_vertex + 3);
        }

        mesh.positions = mesh.rest_positions;

        // Simple UV coordinates
        mesh.texture_coordinates.resize(mesh.positions.size());
        for (std::size_t i = 0; i < mesh.texture_coordinates.size(); ++i)
        {
            mesh.texture_coordinates[i] = math::vec2{0.5F, 0.5F};
        }

        update_bounds(mesh);
        return mesh;
    }

    SurfaceMesh make_unit_cube()
    {
        // Create unit AABB centered at origin and reuse mesh_from_aabb()
        Aabb unit_box{
            math::vec3{-0.5F, -0.5F, -0.5F},
            math::vec3{ 0.5F,  0.5F,  0.5F}
        };

        return mesh_from_aabb(unit_box);
    }



} // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept
{
    return engine::geometry::module_name().data();
}