#include "engine/geometry/surfaces/platonic_solids.hpp"
#include "engine/geometry/api.hpp"
#include "engine/core/log.hpp"
#include <cmath>
#include <numbers>

namespace engine::geometry
{
    SurfaceMesh tetrahedron()
    {
        ENGINE_CORE_TRACE("Creating tetrahedron mesh");

        SurfaceMesh mesh;

        // Tetrahedron vertices (regular tetrahedron centered at origin)
        const float a = 1.0f / std::sqrt(3.0f);

        mesh.rest_positions = {
            math::vec3{ a,  a,  a},
            math::vec3{-a, -a,  a},
            math::vec3{-a,  a, -a},
            math::vec3{ a, -a, -a}
        };

        mesh.positions = mesh.rest_positions;

        // 4 triangular faces
        mesh.indices = {
            0, 2, 1,  // Front
            0, 1, 3,  // Right
            0, 3, 2,  // Back
            1, 2, 3   // Bottom
        };

        // Compute face normals and assign to vertices
        mesh.normals.resize(4);
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const auto& v0 = mesh.positions[mesh.indices[i]];
            const auto& v1 = mesh.positions[mesh.indices[i + 1]];
            const auto& v2 = mesh.positions[mesh.indices[i + 2]];

            const math::vec3 edge1 = v1 - v0;
            const math::vec3 edge2 = v2 - v0;
            const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

            mesh.normals[mesh.indices[i]] = normal;
            mesh.normals[mesh.indices[i + 1]] = normal;
            mesh.normals[mesh.indices[i + 2]] = normal;
        }

        // Simple UV coordinates (planar projection)
        mesh.texture_coordinates = {
            math::vec2{0.5f, 1.0f},
            math::vec2{0.0f, 0.0f},
            math::vec2{1.0f, 0.0f},
            math::vec2{0.5f, 0.5f}
        };

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created tetrahedron: 4 vertices, 4 faces");

        return mesh;
    }

    SurfaceMesh octahedron()
    {
        ENGINE_CORE_TRACE("Creating octahedron mesh");

        SurfaceMesh mesh;

        // Octahedron vertices (regular octahedron centered at origin)
        mesh.rest_positions = {
            math::vec3{ 1.0f,  0.0f,  0.0f},  // +X
            math::vec3{-1.0f,  0.0f,  0.0f},  // -X
            math::vec3{ 0.0f,  1.0f,  0.0f},  // +Y
            math::vec3{ 0.0f, -1.0f,  0.0f},  // -Y
            math::vec3{ 0.0f,  0.0f,  1.0f},  // +Z
            math::vec3{ 0.0f,  0.0f, -1.0f}   // -Z
        };

        mesh.positions = mesh.rest_positions;

        // 8 triangular faces
        mesh.indices = {
            // Top pyramid
            0, 2, 4,  // +X, +Y, +Z
            4, 2, 1,  // +Z, +Y, -X
            1, 2, 5,  // -X, +Y, -Z
            5, 2, 0,  // -Z, +Y, +X

            // Bottom pyramid
            0, 4, 3,  // +X, +Z, -Y
            4, 1, 3,  // +Z, -X, -Y
            1, 5, 3,  // -X, -Z, -Y
            5, 0, 3   // -Z, +X, -Y
        };

        // Compute face normals
        mesh.normals.resize(6, math::vec3{0.0f, 0.0f, 0.0f});

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const auto& v0 = mesh.positions[mesh.indices[i]];
            const auto& v1 = mesh.positions[mesh.indices[i + 1]];
            const auto& v2 = mesh.positions[mesh.indices[i + 2]];

            const math::vec3 edge1 = v1 - v0;
            const math::vec3 edge2 = v2 - v0;
            const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

            // Accumulate normals for shared vertices
            mesh.normals[mesh.indices[i]] += normal;
            mesh.normals[mesh.indices[i + 1]] += normal;
            mesh.normals[mesh.indices[i + 2]] += normal;
        }

        // Normalize accumulated normals
        for (auto& normal : mesh.normals)
        {
            normal = math::normalize(normal);
        }

        // UV coordinates (spherical projection)
        mesh.texture_coordinates = {
            math::vec2{0.75f, 0.5f},
            math::vec2{0.25f, 0.5f},
            math::vec2{0.5f, 1.0f},
            math::vec2{0.5f, 0.0f},
            math::vec2{0.5f, 0.75f},
            math::vec2{0.5f, 0.25f}
        };

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created octahedron: 6 vertices, 8 faces");

        return mesh;
    }

    SurfaceMesh dodecahedron()
    {
        ENGINE_CORE_TRACE("Creating dodecahedron mesh");

        SurfaceMesh mesh;

        // Golden ratio
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
        const float inv_phi = 1.0f / phi;

        // Dodecahedron has 20 vertices
        mesh.rest_positions = {
            // Cube vertices (±1, ±1, ±1)
            math::vec3{ 1.0f,  1.0f,  1.0f},
            math::vec3{ 1.0f,  1.0f, -1.0f},
            math::vec3{ 1.0f, -1.0f,  1.0f},
            math::vec3{ 1.0f, -1.0f, -1.0f},
            math::vec3{-1.0f,  1.0f,  1.0f},
            math::vec3{-1.0f,  1.0f, -1.0f},
            math::vec3{-1.0f, -1.0f,  1.0f},
            math::vec3{-1.0f, -1.0f, -1.0f},

            // Rectangle in YZ plane (0, ±φ, ±1/φ)
            math::vec3{0.0f,  phi,  inv_phi},
            math::vec3{0.0f,  phi, -inv_phi},
            math::vec3{0.0f, -phi,  inv_phi},
            math::vec3{0.0f, -phi, -inv_phi},

            // Rectangle in XZ plane (±φ, 0, ±1/φ)
            math::vec3{ phi, 0.0f,  inv_phi},
            math::vec3{ phi, 0.0f, -inv_phi},
            math::vec3{-phi, 0.0f,  inv_phi},
            math::vec3{-phi, 0.0f, -inv_phi},

            // Rectangle in XY plane (±φ, ±1/φ, 0)
            math::vec3{ phi,  inv_phi, 0.0f},
            math::vec3{ phi, -inv_phi, 0.0f},
            math::vec3{-phi,  inv_phi, 0.0f},
            math::vec3{-phi, -inv_phi, 0.0f}
        };

        mesh.positions = mesh.rest_positions;

        // 12 pentagonal faces - triangulated (each pentagon = 3 triangles)
        mesh.indices = {
            // Face 1
            0, 8, 9,    0, 9, 1,    0, 1, 16,
            // Face 2
            0, 12, 2,   0, 16, 12,  12, 16, 17,
            // Face 3
            12, 17, 2,  17, 3, 2,   17, 13, 3,
            // Face 4
            1, 9, 5,    1, 5, 13,   1, 13, 16,
            // Face 5
            13, 16, 17, 13, 17, 3,  3, 17, 2,
            // Face 6
            4, 8, 0,    4, 0, 12,   4, 12, 14,
            // Face 7
            8, 4, 18,   8, 18, 9,   9, 18, 5,
            // Face 8
            14, 6, 4,   6, 18, 4,   6, 10, 18,
            // Face 9
            6, 14, 19,  6, 19, 10,  10, 19, 11,
            // Face 10
            2, 10, 6,   2, 6, 12,   12, 6, 14,
            // Face 11
            3, 11, 7,   3, 7, 2,    2, 7, 10,
            // Face 12
            5, 15, 7,   5, 7, 13,   13, 7, 3
        };

        // Compute vertex normals
        mesh.normals.resize(20, math::vec3{0.0f, 0.0f, 0.0f});

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const auto& v0 = mesh.positions[mesh.indices[i]];
            const auto& v1 = mesh.positions[mesh.indices[i + 1]];
            const auto& v2 = mesh.positions[mesh.indices[i + 2]];

            const math::vec3 edge1 = v1 - v0;
            const math::vec3 edge2 = v2 - v0;
            const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

            mesh.normals[mesh.indices[i]] += normal;
            mesh.normals[mesh.indices[i + 1]] += normal;
            mesh.normals[mesh.indices[i + 2]] += normal;
        }

        for (auto& normal : mesh.normals)
        {
            normal = math::normalize(normal);
        }

        // UV coordinates (simple spherical projection)
        mesh.texture_coordinates.resize(20);
        for (size_t i = 0; i < 20; ++i)
        {
            const auto& pos = mesh.positions[i];
            const float u = 0.5f + std::atan2(pos[2], pos[0]) / (2.0f * std::numbers::pi_v<float>);
            const float v = 0.5f + std::asin(pos[1] / math::length(pos)) / std::numbers::pi_v<float>;
            mesh.texture_coordinates[i] = math::vec2{u, v};
        }

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created dodecahedron: 20 vertices, 36 triangles (12 pentagonal faces)");

        return mesh;
    }

    SurfaceMesh icosahedron()
    {
        ENGINE_CORE_TRACE("Creating icosahedron mesh");

        SurfaceMesh mesh;

        // Golden ratio
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

        // Icosahedron has 12 vertices
        // 3 rectangles of vertices
        mesh.rest_positions = {
            // Rectangle in XY plane
            math::vec3{-1.0f,  phi, 0.0f},
            math::vec3{ 1.0f,  phi, 0.0f},
            math::vec3{-1.0f, -phi, 0.0f},
            math::vec3{ 1.0f, -phi, 0.0f},

            // Rectangle in YZ plane
            math::vec3{0.0f, -1.0f,  phi},
            math::vec3{0.0f,  1.0f,  phi},
            math::vec3{0.0f, -1.0f, -phi},
            math::vec3{0.0f,  1.0f, -phi},

            // Rectangle in XZ plane
            math::vec3{ phi, 0.0f, -1.0f},
            math::vec3{ phi, 0.0f,  1.0f},
            math::vec3{-phi, 0.0f, -1.0f},
            math::vec3{-phi, 0.0f,  1.0f}
        };

        mesh.positions = mesh.rest_positions;

        // 20 triangular faces
        mesh.indices = {
            // 5 faces around point 0
            0, 11, 5,
            0, 5, 1,
            0, 1, 7,
            0, 7, 10,
            0, 10, 11,

            // 5 adjacent faces
            1, 5, 9,
            5, 11, 4,
            11, 10, 2,
            10, 7, 6,
            7, 1, 8,

            // 5 faces around point 3
            3, 9, 4,
            3, 4, 2,
            3, 2, 6,
            3, 6, 8,
            3, 8, 9,

            // 5 adjacent faces
            4, 9, 5,
            2, 4, 11,
            6, 2, 10,
            8, 6, 7,
            9, 8, 1
        };

        // Compute vertex normals
        mesh.normals.resize(12, math::vec3{0.0f, 0.0f, 0.0f});

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const auto& v0 = mesh.positions[mesh.indices[i]];
            const auto& v1 = mesh.positions[mesh.indices[i + 1]];
            const auto& v2 = mesh.positions[mesh.indices[i + 2]];

            const math::vec3 edge1 = v1 - v0;
            const math::vec3 edge2 = v2 - v0;
            const math::vec3 normal = math::normalize(math::cross(edge1, edge2));

            mesh.normals[mesh.indices[i]] += normal;
            mesh.normals[mesh.indices[i + 1]] += normal;
            mesh.normals[mesh.indices[i + 2]] += normal;
        }

        for (auto& normal : mesh.normals)
        {
            normal = math::normalize(normal);
        }

        // UV coordinates (spherical projection)
        mesh.texture_coordinates.resize(12);
        for (size_t i = 0; i < 12; ++i)
        {
            const auto& pos = mesh.positions[i];
            const float len = math::length(pos);
            const float u = 0.5f + std::atan2(pos[2], pos[0]) / (2.0f * std::numbers::pi_v<float>);
            const float v = 0.5f + std::asin(pos[1] / len) / std::numbers::pi_v<float>;
            mesh.texture_coordinates[i] = math::vec2{u, v};
        }

        update_bounds(mesh);

        ENGINE_CORE_DEBUG("Created icosahedron: 12 vertices, 20 faces");

        return mesh;
    }

} // namespace engine::geometry

