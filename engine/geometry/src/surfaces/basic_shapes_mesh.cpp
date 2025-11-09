#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes/triangle.hpp"
#include "engine/geometry/shapes/plane.hpp"
#include "engine/geometry/shapes/obb.hpp"
#include "engine/geometry/shapes/frustum.hpp"
#include "engine/math/utils/utils_rotation.hpp"
#include "engine/core/log.hpp"

namespace engine::geometry::surfaces
{
    // ============================================================================
    // Triangle Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Triangle& triangle)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Triangle");

        SurfaceMesh mesh;

        mesh.rest_positions = {triangle.a, triangle.b, triangle.c};
        mesh.positions = mesh.rest_positions;
        mesh.indices = {0, 1, 2};

        // Compute normal
        const math::vec3 edge1 = triangle.b - triangle.a;
        const math::vec3 edge2 = triangle.c - triangle.a;
        const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

        mesh.normals = {normal, normal, normal};

        // Simple UV coordinates
        mesh.texture_coordinates = {
            math::vec2{0.0f, 0.0f},
            math::vec2{1.0f, 0.0f},
            math::vec2{0.5f, 1.0f}
        };

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created triangle mesh: 3 vertices, 1 triangle");

        return mesh;
    }

    void halfedge_mesh_from(const Triangle& triangle, mesh::HalfedgeMeshInterface& out_mesh)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Triangle");
        auto surface = surface_mesh_from(triangle);
        to_halfedge_mesh(surface, out_mesh);
    }

    // ============================================================================
    // Plane Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Plane& plane, float width, float height,
                                 int width_segments, int height_segments)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Plane: {}x{} segments", width_segments, height_segments);

        SurfaceMesh mesh;

        const math::vec3& normal = plane.normal;
        const math::vec3 point_on_plane = normal * plane.distance;

        // Create orthonormal basis
        math::vec3 tangent, bitangent;
        if (std::abs(normal[1]) < 0.999f)
        {
            tangent = math::normalize(math::cross(normal, math::vec3{0.0f, 1.0f, 0.0f}));
        }
        else
        {
            tangent = math::normalize(math::cross(normal, math::vec3{1.0f, 0.0f, 0.0f}));
        }
        bitangent = math::cross(normal, tangent);

        // Generate grid vertices
        const int vertex_count = (width_segments + 1) * (height_segments + 1);
        mesh.rest_positions.reserve(vertex_count);
        mesh.normals.reserve(vertex_count);
        mesh.texture_coordinates.reserve(vertex_count);

        for (int h = 0; h <= height_segments; ++h)
        {
            const float v = static_cast<float>(h) / static_cast<float>(height_segments);
            const float y_offset = (v - 0.5f) * height;

            for (int w = 0; w <= width_segments; ++w)
            {
                const float u = static_cast<float>(w) / static_cast<float>(width_segments);
                const float x_offset = (u - 0.5f) * width;

                const math::vec3 position = point_on_plane + tangent * x_offset + bitangent * y_offset;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(normal);
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Generate indices
        mesh.indices.reserve(width_segments * height_segments * 6);

        for (int h = 0; h < height_segments; ++h)
        {
            for (int w = 0; w < width_segments; ++w)
            {
                const auto base = static_cast<std::uint32_t>(h * (width_segments + 1) + w);
                const auto next_row = base + width_segments + 1;

                mesh.indices.push_back(base);
                mesh.indices.push_back(next_row);
                mesh.indices.push_back(base + 1);

                mesh.indices.push_back(next_row);
                mesh.indices.push_back(next_row + 1);
                mesh.indices.push_back(base + 1);
            }
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created plane mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Plane& plane, mesh::HalfedgeMeshInterface& out_mesh,
                           float width, float height, int width_segments, int height_segments)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Plane");
        auto surface = surface_mesh_from(plane, width, height, width_segments, height_segments);
        to_halfedge_mesh(surface, out_mesh);
    }

    // ============================================================================
    // OBB Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Obb& box)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from OBB");

        // Convert OBB to AABB in local space, then transform
        const math::vec3& center = box.center;
        const math::vec3& extent = box.half_sizes;
        const math::mat3 axes = math::utils::to_rotation_matrix(box.orientation);

        SurfaceMesh mesh;

        // 8 corners of the box in local space
        const std::array<math::vec3, 8> local_corners = {{
            math::vec3{-extent[0], -extent[1], -extent[2]},
            math::vec3{ extent[0], -extent[1], -extent[2]},
            math::vec3{ extent[0],  extent[1], -extent[2]},
            math::vec3{-extent[0],  extent[1], -extent[2]},
            math::vec3{-extent[0], -extent[1],  extent[2]},
            math::vec3{ extent[0], -extent[1],  extent[2]},
            math::vec3{ extent[0],  extent[1],  extent[2]},
            math::vec3{-extent[0],  extent[1],  extent[2]}
        }};

        // Transform corners to world space
        std::array<math::vec3, 8> corners;
        for (size_t i = 0; i < 8; ++i)
        {
            corners[i] = center + axes * local_corners[i];
        }

        // Face normals (in local space, then transformed)
        const std::array<math::vec3, 6> local_normals = {{
            math::vec3{ 0.0f,  0.0f,  1.0f},  // Front
            math::vec3{ 0.0f,  0.0f, -1.0f},  // Back
            math::vec3{ 1.0f,  0.0f,  0.0f},  // Right
            math::vec3{-1.0f,  0.0f,  0.0f},  // Left
            math::vec3{ 0.0f,  1.0f,  0.0f},  // Top
            math::vec3{ 0.0f, -1.0f,  0.0f},  // Bottom
        }};

        std::array<math::vec3, 6> face_normals;
        for (size_t i = 0; i < 6; ++i)
        {
            face_normals[i] = math::normalize(axes * local_normals[i]);
        }

        // Face quads (indices into corners array)
        const std::array<std::array<int, 4>, 6> face_quads = {{
            {4, 5, 6, 7},  // Front
            {1, 0, 3, 2},  // Back
            {5, 1, 2, 6},  // Right
            {0, 4, 7, 3},  // Left
            {7, 6, 2, 3},  // Top
            {0, 1, 5, 4}   // Bottom
        }};

        mesh.rest_positions.reserve(24);
        mesh.normals.reserve(24);
        mesh.indices.reserve(36);
        mesh.texture_coordinates.reserve(24);

        for (size_t face_idx = 0; face_idx < 6; ++face_idx)
        {
            const auto& quad = face_quads[face_idx];
            const auto& normal = face_normals[face_idx];

            const auto base_vertex = static_cast<std::uint32_t>(mesh.rest_positions.size());

            for (int i = 0; i < 4; ++i)
            {
                mesh.rest_positions.push_back(corners[quad[i]]);
                mesh.normals.push_back(normal);
            }

            // UVs
            mesh.texture_coordinates.push_back(math::vec2{0.0f, 0.0f});
            mesh.texture_coordinates.push_back(math::vec2{1.0f, 0.0f});
            mesh.texture_coordinates.push_back(math::vec2{1.0f, 1.0f});
            mesh.texture_coordinates.push_back(math::vec2{0.0f, 1.0f});

            // Triangles
            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 1);
            mesh.indices.push_back(base_vertex + 2);

            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 2);
            mesh.indices.push_back(base_vertex + 3);
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created OBB mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Obb& box, mesh::HalfedgeMeshInterface& out_mesh)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from OBB");
        auto surface = surface_mesh_from(box);
        to_halfedge_mesh(surface, out_mesh);
    }

    // ============================================================================
    // Frustum Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Frustum& frustum)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Frustum");

        SurfaceMesh mesh;

        // Get the 8 corners of the frustum
        const auto corners = GetCorners(frustum);

        // Near plane: 0, 1, 2, 3
        // Far plane: 4, 5, 6, 7

        // Face quads
        const std::array<std::array<int, 4>, 6> face_quads = {{
            {0, 1, 2, 3},  // Near
            {5, 4, 7, 6},  // Far
            {4, 5, 1, 0},  // Bottom
            {3, 2, 6, 7},  // Top
            {4, 0, 3, 7},  // Left
            {1, 5, 6, 2}   // Right
        }};

        mesh.rest_positions.reserve(24);
        mesh.normals.reserve(24);
        mesh.indices.reserve(36);
        mesh.texture_coordinates.reserve(24);

        for (size_t face_idx = 0; face_idx < 6; ++face_idx)
        {
            const auto& quad = face_quads[face_idx];

            const auto base_vertex = static_cast<std::uint32_t>(mesh.rest_positions.size());

            // Add vertices
            for (int i = 0; i < 4; ++i)
            {
                mesh.rest_positions.push_back(corners[quad[i]]);
            }

            // Compute face normal
            const math::vec3 edge1 = corners[quad[1]] - corners[quad[0]];
            const math::vec3 edge2 = corners[quad[2]] - corners[quad[0]];
            const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

            for (int i = 0; i < 4; ++i)
            {
                mesh.normals.push_back(normal);
            }

            // UVs
            mesh.texture_coordinates.push_back(math::vec2{0.0f, 0.0f});
            mesh.texture_coordinates.push_back(math::vec2{1.0f, 0.0f});
            mesh.texture_coordinates.push_back(math::vec2{1.0f, 1.0f});
            mesh.texture_coordinates.push_back(math::vec2{0.0f, 1.0f});

            // Triangles
            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 1);
            mesh.indices.push_back(base_vertex + 2);

            mesh.indices.push_back(base_vertex + 0);
            mesh.indices.push_back(base_vertex + 2);
            mesh.indices.push_back(base_vertex + 3);
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created frustum mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Frustum& frustum, mesh::HalfedgeMeshInterface& out_mesh)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Frustum");
        auto surface = surface_mesh_from(frustum);
        to_halfedge_mesh(surface, out_mesh);
    }

} // namespace engine::geometry::surfaces

