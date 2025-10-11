#include "engine/geometry/api.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>

namespace engine::geometry {

namespace {

[[nodiscard]] math::vec3 triangle_normal(
    const math::vec3& a,
    const math::vec3& b,
    const math::vec3& c) {
    return math::normalize(math::cross(b - a, c - a));
}

}  // namespace

std::string_view module_name() noexcept {
    return "geometry";
}

SurfaceMesh make_unit_quad() {
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
    update_bounds(mesh);
    return mesh;
}

void recompute_vertex_normals(SurfaceMesh& mesh) {
    mesh.normals.assign(mesh.positions.size(), math::vec3{0.0F, 0.0F, 0.0F});
    for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const auto ia = mesh.indices[i];
        const auto ib = mesh.indices[i + 1];
        const auto ic = mesh.indices[i + 2];
        if (ia >= mesh.positions.size() || ib >= mesh.positions.size() || ic >= mesh.positions.size()) {
            continue;
        }
        const auto normal = triangle_normal(mesh.positions[ia], mesh.positions[ib], mesh.positions[ic]);
        mesh.normals[ia] += normal;
        mesh.normals[ib] += normal;
        mesh.normals[ic] += normal;
    }

    for (auto& normal : mesh.normals) {
        const float length_sq = math::dot(normal, normal);
        if (length_sq > 0.0F) {
            normal = math::normalize(normal);
        } else {
            normal = math::vec3{0.0F, 1.0F, 0.0F};
        }
    }
}

void update_bounds(SurfaceMesh& mesh) {
    if (mesh.positions.empty()) {
        // Degenerate meshes have no extents; normalize their bounds to the origin so downstream
        // consumers never observe infinities.
        mesh.bounds = MeshBounds{math::vec3{0.0F, 0.0F, 0.0F}, math::vec3{0.0F, 0.0F, 0.0F}};
        return;
    }

    math::vec3 min_bounds{std::numeric_limits<float>::max()};
    math::vec3 max_bounds{std::numeric_limits<float>::lowest()};
    for (const auto& position : mesh.positions) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            min_bounds[axis] = std::min(min_bounds[axis], position[axis]);
            max_bounds[axis] = std::max(max_bounds[axis], position[axis]);
        }
    }
    mesh.bounds = MeshBounds{min_bounds, max_bounds};
}

void apply_uniform_translation(SurfaceMesh& mesh, const math::vec3& translation) {
    mesh.positions.resize(mesh.rest_positions.size());
    for (std::size_t index = 0; index < mesh.rest_positions.size(); ++index) {
        mesh.positions[index] = mesh.rest_positions[index] + translation;
    }
    update_bounds(mesh);
}

math::vec3 centroid(const SurfaceMesh& mesh) {
    if (mesh.positions.empty()) {
        return math::vec3{0.0F, 0.0F, 0.0F};
    }
    math::vec3 sum{0.0F, 0.0F, 0.0F};
    for (const auto& position : mesh.positions) {
        sum += position;
    }
    return sum / static_cast<float>(mesh.positions.size());
}

SurfaceMesh load_surface_mesh(const std::filesystem::path& path)
{
    mesh::Mesh container{};
    mesh::read(container.interface, path);
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
    mesh::Mesh container{};
    mesh::build_halfedge_from_surface_mesh(surface, container.interface);
    mesh::write(container.interface, path, flags);
}

}  // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept {
    return engine::geometry::module_name().data();
}
