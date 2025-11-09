#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes/capsule.hpp"
#include "engine/geometry/shapes/ellipsoid.hpp"
#include "engine/core/log.hpp"
#include "engine/math/utils/utils_rotation.hpp"
#include <cmath>
#include <numbers>

namespace engine::geometry::surfaces
{
    // ============================================================================
    // Capsule Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Capsule& capsule, int radial_segments, int height_segments,
                                  int hemisphere_rings)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Capsule");

        SurfaceMesh mesh;

        const math::vec3& point_a = capsule.point_a;
        const math::vec3& point_b = capsule.point_b;
        const float radius = capsule.radius;

        const math::vec3 axis = AxisDirection(capsule);
        const float cylinder_height = Length(capsule);
        const math::vec3 center = Center(capsule);

        // Create orthonormal basis
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

        // Estimate vertex count
        const int cylinder_verts = (radial_segments + 1) * (height_segments + 1);
        const int hemisphere_verts = (radial_segments + 1) * hemisphere_rings * 2;
        mesh.rest_positions.reserve(cylinder_verts + hemisphere_verts + 2);
        mesh.normals.reserve(mesh.rest_positions.capacity());
        mesh.texture_coordinates.reserve(mesh.rest_positions.capacity());

        // Bottom hemisphere (at point_a)
        for (int ring = hemisphere_rings; ring > 0; --ring)
        {
            const float phi = static_cast<float>(ring) * (std::numbers::pi_v<float> / 2.0f) / static_cast<float>(
                hemisphere_rings);
            const float sin_phi = std::sin(phi);
            const float cos_phi = std::cos(phi);
            const float y = -cos_phi * radius;
            const float ring_radius = sin_phi * radius;

            const math::vec3 ring_center = point_a + axis * y;

            for (int seg = 0; seg <= radial_segments; ++seg)
            {
                const float theta = static_cast<float>(seg) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(
                    radial_segments);
                const float cos_theta = std::cos(theta);
                const float sin_theta = std::sin(theta);

                const math::vec3 radial = tangent * cos_theta + bitangent * sin_theta;
                const math::vec3 position = ring_center + radial * ring_radius;
                const math::vec3 normal = math::normalize(position - point_a);

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(normal);

                const float u = static_cast<float>(seg) / static_cast<float>(radial_segments);
                const float v = static_cast<float>(hemisphere_rings - ring) / static_cast<float>(hemisphere_rings * 2 +
                    height_segments);
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Cylindrical section
        const float half_cylinder = cylinder_height * 0.5f;
        for (int h = 0; h <= height_segments; ++h)
        {
            const float t = static_cast<float>(h) / static_cast<float>(height_segments);
            const float y = -half_cylinder + t * cylinder_height;
            const math::vec3 ring_center = center + axis * y;

            for (int seg = 0; seg <= radial_segments; ++seg)
            {
                const float theta = static_cast<float>(seg) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(
                    radial_segments);
                const float cos_theta = std::cos(theta);
                const float sin_theta = std::sin(theta);

                const math::vec3 radial = tangent * cos_theta + bitangent * sin_theta;
                const math::vec3 position = ring_center + radial * radius;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(radial);

                const float u = static_cast<float>(seg) / static_cast<float>(radial_segments);
                const float v = (static_cast<float>(hemisphere_rings + h) / static_cast<float>(hemisphere_rings * 2 +
                    height_segments));
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Top hemisphere (at point_b)
        for (int ring = 1; ring <= hemisphere_rings; ++ring)
        {
            const float phi = static_cast<float>(ring) * (std::numbers::pi_v<float> / 2.0f) / static_cast<float>(
                hemisphere_rings);
            const float sin_phi = std::sin(phi);
            const float cos_phi = std::cos(phi);
            const float y = cos_phi * radius;
            const float ring_radius = sin_phi * radius;

            const math::vec3 ring_center = point_b + axis * y;

            for (int seg = 0; seg <= radial_segments; ++seg)
            {
                const float theta = static_cast<float>(seg) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(
                    radial_segments);
                const float cos_theta = std::cos(theta);
                const float sin_theta = std::sin(theta);

                const math::vec3 radial = tangent * cos_theta + bitangent * sin_theta;
                const math::vec3 position = ring_center + radial * ring_radius;
                const math::vec3 normal = math::normalize(position - point_b);

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(normal);

                const float u = static_cast<float>(seg) / static_cast<float>(radial_segments);
                const float v = (static_cast<float>(hemisphere_rings + height_segments + ring) /
                    static_cast<float>(hemisphere_rings * 2 + height_segments));
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Generate indices
        const int total_rings = hemisphere_rings * 2 + height_segments;
        for (int ring = 0; ring < total_rings; ++ring)
        {
            for (int seg = 0; seg < radial_segments; ++seg)
            {
                const auto base = static_cast<std::uint32_t>(ring * (radial_segments + 1) + seg);
                const auto next_ring = base + radial_segments + 1;

                mesh.indices.push_back(base);
                mesh.indices.push_back(next_ring);
                mesh.indices.push_back(base + 1);

                mesh.indices.push_back(next_ring);
                mesh.indices.push_back(next_ring + 1);
                mesh.indices.push_back(base + 1);
            }
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created capsule mesh: {} vertices, {} triangles",
                          mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Capsule& capsule, mesh::HalfedgeMeshInterface& out_mesh,
                            int radial_segments, int height_segments, int hemisphere_rings)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Capsule");
        auto surface = surface_mesh_from(capsule, radial_segments, height_segments, hemisphere_rings);
        to_halfedge_mesh(surface, out_mesh);
    }

    // ============================================================================
    // Ellipsoid Mesh
    // ============================================================================

    SurfaceMesh surface_mesh_from(const Ellipsoid& ellipsoid, int lat_segments, int lon_segments)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Ellipsoid");

        // Create a unit sphere at origin
        Sphere unit_sphere{math::vec3{0.0f, 0.0f, 0.0f}, 1.0f};

        // Calculate subdivisions based on segments
        int subdivisions = 0;
        while ((16 * (1 << subdivisions)) < lat_segments) subdivisions++;

        // Generate sphere mesh
        SurfaceMesh mesh = surface_mesh_from(unit_sphere, subdivisions);

        // Convert quaternion to 4x4 rotation matrix
        const math::mat4 rot_matrix = math::utils::to_rotation_matrix(ellipsoid.orientation);

        // Apply affine transformation: scale by radii, rotate, then translate to center
        for (size_t i = 0; i < mesh.rest_positions.size(); ++i)
        {
            // Scale by radii (non-uniform scaling to create ellipsoid)
            math::vec3 scaled_pos{
                mesh.rest_positions[i][0] * ellipsoid.radii[0],
                mesh.rest_positions[i][1] * ellipsoid.radii[1],
                mesh.rest_positions[i][2] * ellipsoid.radii[2]
            };

            // Rotate using upper-left 3x3 of rotation matrix
            math::vec3 rotated_pos{
                rot_matrix[0][0] * scaled_pos[0] + rot_matrix[0][1] * scaled_pos[1] + rot_matrix[0][2] * scaled_pos[2],
                rot_matrix[1][0] * scaled_pos[0] + rot_matrix[1][1] * scaled_pos[1] + rot_matrix[1][2] * scaled_pos[2],
                rot_matrix[2][0] * scaled_pos[0] + rot_matrix[2][1] * scaled_pos[1] + rot_matrix[2][2] * scaled_pos[2]
            };

            // Translate to center
            mesh.rest_positions[i] = ellipsoid.center + rotated_pos;
            mesh.positions[i] = mesh.rest_positions[i];

            // Transform normal: for non-uniform scaling, use inverse transpose of scale matrix
            math::vec3 scaled_normal{
                mesh.normals[i][0] / (ellipsoid.radii[0] * ellipsoid.radii[0]),
                mesh.normals[i][1] / (ellipsoid.radii[1] * ellipsoid.radii[1]),
                mesh.normals[i][2] / (ellipsoid.radii[2] * ellipsoid.radii[2])
            };

            // Rotate normal
            math::vec3 rotated_normal{
                rot_matrix[0][0] * scaled_normal[0] + rot_matrix[0][1] * scaled_normal[1] + rot_matrix[0][2] * scaled_normal[2],
                rot_matrix[1][0] * scaled_normal[0] + rot_matrix[1][1] * scaled_normal[1] + rot_matrix[1][2] * scaled_normal[2],
                rot_matrix[2][0] * scaled_normal[0] + rot_matrix[2][1] * scaled_normal[1] + rot_matrix[2][2] * scaled_normal[2]
            };

            mesh.normals[i] = math::normalize(rotated_normal);
        }

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created ellipsoid mesh from sphere: {} vertices, {} triangles",
                          mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Ellipsoid& ellipsoid, mesh::HalfedgeMeshInterface& out_mesh,
                            int lat_segments, int lon_segments)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Ellipsoid");
        auto surface = surface_mesh_from(ellipsoid, lat_segments, lon_segments);
        to_halfedge_mesh(surface, out_mesh);
    }
} // namespace engine::geometry::surfaces
