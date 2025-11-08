#include "engine/geometry/api.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"
#include "engine/geometry/shapes/aabb.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <cmath>

namespace engine::geometry
{
    namespace
    {
        [[nodiscard]] math::vec3 triangle_normal(
            const math::vec3& a,
            const math::vec3& b,
            const math::vec3& c)
        {
            return math::normalize(math::cross(c - a, b - a));
        }
    } // namespace

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

    void recompute_vertex_normals(SurfaceMesh& mesh)
    {
        mesh.normals.assign(mesh.positions.size(), math::vec3{0.0F, 0.0F, 0.0F});
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const auto ia = mesh.indices[i];
            const auto ib = mesh.indices[i + 1];
            const auto ic = mesh.indices[i + 2];
            if (ia >= mesh.positions.size() || ib >= mesh.positions.size() || ic >= mesh.positions.size())
            {
                continue;
            }
            const auto normal = triangle_normal(mesh.positions[ia], mesh.positions[ib], mesh.positions[ic]);
            mesh.normals[ia] += normal;
            mesh.normals[ib] += normal;
            mesh.normals[ic] += normal;
        }

        for (auto& normal : mesh.normals)
        {
            const float length_sq = math::dot(normal, normal);
            if (length_sq > 0.0F)
            {
                normal = math::normalize(normal);
            }
            else
            {
                normal = math::vec3{0.0F, 1.0F, 0.0F};
            }
        }
    }

    void update_bounds(SurfaceMesh& mesh)
    {
        if (mesh.positions.empty())
        {
            // Degenerate meshes have no extents; normalize their bounds to the origin so downstream
            // consumers never observe infinities.
            mesh.bounds = Aabb{math::vec3{0.0F, 0.0F, 0.0F}, math::vec3{0.0F, 0.0F, 0.0F}};
            return;
        }

        math::vec3 min_bounds{std::numeric_limits<float>::max()};
        math::vec3 max_bounds{std::numeric_limits<float>::lowest()};
        for (const auto& position : mesh.positions)
        {
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                min_bounds[axis] = std::min(min_bounds[axis], position[axis]);
                max_bounds[axis] = std::max(max_bounds[axis], position[axis]);
            }
        }
        mesh.bounds = Aabb{min_bounds, max_bounds};
    }

    void apply_uniform_translation(SurfaceMesh& mesh, const math::vec3& translation)
    {
        mesh.positions.resize(mesh.rest_positions.size());
        for (std::size_t index = 0; index < mesh.rest_positions.size(); ++index)
        {
            mesh.positions[index] = mesh.rest_positions[index] + translation;
        }
        update_bounds(mesh);
    }

    math::vec3 centroid(const SurfaceMesh& mesh)
    {
        if (mesh.positions.empty())
        {
            return math::vec3{0.0F, 0.0F, 0.0F};
        }
        math::vec3 sum{0.0F, 0.0F, 0.0F};
        for (const auto& position : mesh.positions)
        {
            sum += position;
        }
        return sum / static_cast<float>(mesh.positions.size());
    }

    float surface_area(const SurfaceMesh& mesh) noexcept
    {
        if (mesh.positions.empty() || mesh.indices.size() < 3U)
        {
            return 0.0F;
        }

        float area = 0.0F;
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3U)
        {
            const std::uint32_t ia = mesh.indices[i];
            const std::uint32_t ib = mesh.indices[i + 1U];
            const std::uint32_t ic = mesh.indices[i + 2U];
            if (ia >= mesh.positions.size() || ib >= mesh.positions.size() || ic >= mesh.positions.size())
            {
                continue;
            }

            const math::vec3& a = mesh.positions[ia];
            const math::vec3& b = mesh.positions[ib];
            const math::vec3& c = mesh.positions[ic];
            const math::vec3 cross = math::cross(b - a, c - a);
            const float triangle_area = 0.5F * math::length(cross);
            if (std::isfinite(triangle_area))
            {
                area += triangle_area;
            }
        }

        return area;
    }

    SurfaceMesh load_surface_mesh(const std::filesystem::path& path)
    {
        Mesh container{};
        read(container.interface, path);
        return mesh::build_surface_mesh_from_halfedge(container.interface);
    }

    void save_surface_mesh(const SurfaceMesh& surface, const std::filesystem::path& path)
    {
        save_surface_mesh(surface, path, mesh::IOFlags{});
    }

    void save_surface_mesh(const SurfaceMesh& surface,
                           const std::filesystem::path& path,
                           const mesh::IOFlags& flags)
    {
        Mesh container{};
        build_halfedge_from_surface_mesh(surface, container.interface);
        write(container.interface, path, flags);
    }
} // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept
{
    return engine::geometry::module_name().data();
}