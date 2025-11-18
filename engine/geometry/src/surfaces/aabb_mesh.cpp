#include "engine/geometry/api.hpp"
#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/core/log.hpp"
#include <array>

namespace engine::geometry::surfaces
{
    SurfaceMesh surface_mesh_from(const Aabb& box)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from AABB");

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

        // Simple UV coordinates (box mapping)
        mesh.texture_coordinates.resize(mesh.positions.size());
        for (std::size_t i = 0; i < mesh.texture_coordinates.size(); i += 4)
        {
            mesh.texture_coordinates[i + 0] = math::vec2{0.0F, 0.0F};
            mesh.texture_coordinates[i + 1] = math::vec2{1.0F, 0.0F};
            mesh.texture_coordinates[i + 2] = math::vec2{1.0F, 1.0F};
            mesh.texture_coordinates[i + 3] = math::vec2{0.0F, 1.0F};
        }

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created AABB mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Aabb& box, mesh::HalfedgeMeshInterface& out_mesh)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from AABB");
        auto surface = surface_mesh_from(box);
        to_halfedge_mesh(surface, out_mesh);
    }

} // namespace engine::geometry::surfaces

