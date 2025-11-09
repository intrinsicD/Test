#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes/cylinder.hpp"
#include "engine/core/log.hpp"
#include <cmath>
#include <numbers>

namespace engine::geometry::surfaces
{
    SurfaceMesh surface_mesh_from(const Cylinder& cylinder, int radial_segments, int height_segments, bool with_caps)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Cylinder: radial={}, height={}, caps={}",
            radial_segments, height_segments, with_caps);

        SurfaceMesh mesh;

        const math::vec3& center = cylinder.center;
        const math::vec3 axis = AxisDirection(cylinder);
        const float radius = cylinder.radius;
        const float half_height = cylinder.half_height;

        // Create orthonormal basis for cylinder
        math::vec3 tangent, bitangent;
        if (std::abs(axis[1]) < 0.999f)
        {
            tangent = math::normalize(math::cross(axis, math::vec3{0.0f, 1.0f, 0.0f}));
        }
        else
        {
            tangent = math::normalize(math::cross(axis, math::vec3{1.0f, 0.0f, 0.0f}));
        }
        bitangent = math::cross(axis, tangent);

        // Generate lateral surface vertices
        const int lateral_vertex_count = (radial_segments + 1) * (height_segments + 1);
        mesh.rest_positions.reserve(lateral_vertex_count + (with_caps ? radial_segments * 2 + 2 : 0));
        mesh.normals.reserve(mesh.rest_positions.capacity());
        mesh.texture_coordinates.reserve(mesh.rest_positions.capacity());

        // Lateral surface
        for (int h = 0; h <= height_segments; ++h)
        {
            const float t = static_cast<float>(h) / static_cast<float>(height_segments);
            const float y = -half_height + t * 2.0f * half_height;
            const math::vec3 ring_center = center + axis * y;

            for (int r = 0; r <= radial_segments; ++r)
            {
                const float angle = static_cast<float>(r) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(radial_segments);
                const float cos_angle = std::cos(angle);
                const float sin_angle = std::sin(angle);

                const math::vec3 radial_dir = tangent * cos_angle + bitangent * sin_angle;
                const math::vec3 position = ring_center + radial_dir * radius;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(radial_dir); // Normal points outward radially

                const float u = static_cast<float>(r) / static_cast<float>(radial_segments);
                const float v = t;
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Generate lateral surface indices
        for (int h = 0; h < height_segments; ++h)
        {
            for (int r = 0; r < radial_segments; ++r)
            {
                const auto base = static_cast<std::uint32_t>(h * (radial_segments + 1) + r);
                const auto next_row = static_cast<std::uint32_t>(base + radial_segments + 1);

                mesh.indices.push_back(base);
                mesh.indices.push_back(next_row);
                mesh.indices.push_back(base + 1);

                mesh.indices.push_back(next_row);
                mesh.indices.push_back(next_row + 1);
                mesh.indices.push_back(base + 1);
            }
        }

        // Add caps if requested
        if (with_caps)
        {
            const auto lateral_vertex_count_u32 = static_cast<std::uint32_t>(mesh.rest_positions.size());

            // Top cap
            const math::vec3 top_center = center + axis * half_height;
            const auto top_center_idx = static_cast<std::uint32_t>(mesh.rest_positions.size());
            mesh.rest_positions.push_back(top_center);
            mesh.normals.push_back(axis);
            mesh.texture_coordinates.push_back(math::vec2{0.5f, 0.5f});

            for (int r = 0; r < radial_segments; ++r)
            {
                const float angle = static_cast<float>(r) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(radial_segments);
                const float cos_angle = std::cos(angle);
                const float sin_angle = std::sin(angle);

                const math::vec3 radial_dir = tangent * cos_angle + bitangent * sin_angle;
                const math::vec3 position = top_center + radial_dir * radius;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(axis);

                const float u = 0.5f + 0.5f * cos_angle;
                const float v = 0.5f + 0.5f * sin_angle;
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }

            // Top cap triangles
            for (int r = 0; r < radial_segments; ++r)
            {
                const auto next = (r + 1) % radial_segments;
                mesh.indices.push_back(top_center_idx);
                mesh.indices.push_back(top_center_idx + 1 + r);
                mesh.indices.push_back(top_center_idx + 1 + next);
            }

            // Bottom cap
            const math::vec3 bottom_center = center - axis * half_height;
            const auto bottom_center_idx = static_cast<std::uint32_t>(mesh.rest_positions.size());
            mesh.rest_positions.push_back(bottom_center);
            mesh.normals.push_back(-axis);
            mesh.texture_coordinates.push_back(math::vec2{0.5f, 0.5f});

            for (int r = 0; r < radial_segments; ++r)
            {
                const float angle = static_cast<float>(r) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(radial_segments);
                const float cos_angle = std::cos(angle);
                const float sin_angle = std::sin(angle);

                const math::vec3 radial_dir = tangent * cos_angle + bitangent * sin_angle;
                const math::vec3 position = bottom_center + radial_dir * radius;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(-axis);

                const float u = 0.5f + 0.5f * cos_angle;
                const float v = 0.5f + 0.5f * sin_angle;
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }

            // Bottom cap triangles (reversed winding for downward normal)
            for (int r = 0; r < radial_segments; ++r)
            {
                const auto next = (r + 1) % radial_segments;
                mesh.indices.push_back(bottom_center_idx);
                mesh.indices.push_back(bottom_center_idx + 1 + next);
                mesh.indices.push_back(bottom_center_idx + 1 + r);
            }
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created cylinder mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Cylinder& cylinder, mesh::HalfedgeMeshInterface& out_mesh,
                           int radial_segments, int height_segments, bool with_caps)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Cylinder");
        auto surface = surface_mesh_from(cylinder, radial_segments, height_segments, with_caps);
        to_halfedge_mesh(surface, out_mesh);
    }

} // namespace engine::geometry::surfaces

