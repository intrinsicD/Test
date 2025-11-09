#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/core/log.hpp"
#include <cmath>
#include <numbers>

namespace engine::geometry::surfaces
{
    SurfaceMesh surface_mesh_from(const Sphere& sphere, int subdivisions)
    {
        ENGINE_CORE_TRACE("Creating SurfaceMesh from Sphere with {} subdivisions", subdivisions);

        // Use UV sphere (latitude-longitude) parameterization
        const int lat_segments = 16 * (1 << subdivisions);  // Double each subdivision
        const int lon_segments = 32 * (1 << subdivisions);

        SurfaceMesh mesh;

        const float radius = sphere.radius;
        const math::vec3& center = sphere.center;

        // Generate vertices
        mesh.rest_positions.reserve((lat_segments + 1) * (lon_segments + 1));
        mesh.normals.reserve((lat_segments + 1) * (lon_segments + 1));
        mesh.texture_coordinates.reserve((lat_segments + 1) * (lon_segments + 1));

        for (int lat = 0; lat <= lat_segments; ++lat)
        {
            const float theta = static_cast<float>(lat) * std::numbers::pi_v<float> / static_cast<float>(lat_segments);
            const float sin_theta = std::sin(theta);
            const float cos_theta = std::cos(theta);

            for (int lon = 0; lon <= lon_segments; ++lon)
            {
                const float phi = static_cast<float>(lon) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(lon_segments);
                const float sin_phi = std::sin(phi);
                const float cos_phi = std::cos(phi);

                // Position on unit sphere
                const math::vec3 normal{
                    sin_theta * cos_phi,
                    cos_theta,
                    sin_theta * sin_phi
                };

                const math::vec3 position = center + normal * radius;

                mesh.rest_positions.push_back(position);
                mesh.normals.push_back(normal);

                // UV coordinates
                const float u = static_cast<float>(lon) / static_cast<float>(lon_segments);
                const float v = static_cast<float>(lat) / static_cast<float>(lat_segments);
                mesh.texture_coordinates.push_back(math::vec2{u, v});
            }
        }

        // Generate indices
        mesh.indices.reserve(lat_segments * lon_segments * 6);

        for (int lat = 0; lat < lat_segments; ++lat)
        {
            for (int lon = 0; lon < lon_segments; ++lon)
            {
                const auto first = static_cast<std::uint32_t>(lat * (lon_segments + 1) + lon);
                const auto second = static_cast<std::uint32_t>(first + lon_segments + 1);

                // First triangle
                mesh.indices.push_back(first);
                mesh.indices.push_back(second);
                mesh.indices.push_back(first + 1);

                // Second triangle
                mesh.indices.push_back(second);
                mesh.indices.push_back(second + 1);
                mesh.indices.push_back(first + 1);
            }
        }

        mesh.positions = mesh.rest_positions;
        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created sphere mesh: {} vertices, {} triangles",
            mesh.positions.size(), mesh.indices.size() / 3);

        return mesh;
    }

    void halfedge_mesh_from(const Sphere& sphere, mesh::HalfedgeMeshInterface& out_mesh, int subdivisions)
    {
        ENGINE_CORE_TRACE("Creating HalfedgeMesh from Sphere");
        auto surface = surface_mesh_from(sphere, subdivisions);
        to_halfedge_mesh(surface, out_mesh);
    }

} // namespace engine::geometry::surfaces

