#include "engine/geometry/api.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include "engine/geometry/shapes/aabb.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <cmath>
#include <vector>

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
            math::vec3{-1.0F,  0.0F,  0.0F},  // -X
            math::vec3{ 1.0F,  0.0F,  0.0F},  // +X
            math::vec3{ 0.0F, -1.0F,  0.0F},  // -Y
            math::vec3{ 0.0F,  1.0F,  0.0F},  // +Y
            math::vec3{ 0.0F,  0.0F, -1.0F},  // -Z
            math::vec3{ 0.0F,  0.0F,  1.0F},  // +Z
        }};

        // Build mesh from AABB face quads
        std::vector<math::vec2> texture_coordinates{};
        texture_coordinates.reserve(face_quads.size() * 4U);

        const math::vec3 extent = box.max - box.min;
        const math::vec3 safe_extent{
            std::max(extent[0], std::numeric_limits<float>::epsilon()),
            std::max(extent[1], std::numeric_limits<float>::epsilon()),
            std::max(extent[2], std::numeric_limits<float>::epsilon())
        };

        const std::array<std::pair<int, int>, 6> face_axes{{
            {2, 1}, // -X: u = z, v = y
            {2, 1}, // +X
            {0, 2}, // -Y: u = x, v = z
            {0, 2}, // +Y
            {0, 1}, // -Z: u = x, v = y
            {0, 1}  // +Z
        }};

        for (std::size_t face_idx = 0; face_idx < face_quads.size(); ++face_idx)
        {
            const auto& quad = face_quads[face_idx];
            const auto& normal = face_normals[face_idx];
            const auto [u_axis, v_axis] = face_axes[face_idx];

            const std::uint32_t base_vertex = static_cast<std::uint32_t>(mesh.rest_positions.size());

            // Add 4 vertices for this face
            for (int i = 0; i < 4; ++i)
            {
                const auto corner_index = static_cast<std::size_t>(quad[i]);
                const auto position = corners[corner_index];
                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(normal);

                const float u = (position[u_axis] - box.min[u_axis]) / safe_extent[u_axis];
                const float v = (position[v_axis] - box.min[v_axis]) / safe_extent[v_axis];
                texture_coordinates.emplace_back(u, v);
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
        mesh.texture_coordinates = std::move(texture_coordinates);

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