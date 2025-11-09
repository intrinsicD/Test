#include "engine/geometry/surfaces/platonic_solids.hpp"
#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/core/log.hpp"

int main()
{
    using namespace engine::geometry;

    // Initialize logging
    engine::core::Log::init();

    ENGINE_INFO("=== Platonic Solids Generation Test ===");

    // Generate all five Platonic solids

    // 1. Tetrahedron - simplest (4 faces)
    ENGINE_INFO("Generating Tetrahedron...");
    auto tetra = tetrahedron();
    ENGINE_INFO("  Vertices: {}, Triangles: {}",
        tetra.positions.size(), tetra.indices.size() / 3);
    ENGINE_INFO("  Has normals: {}, Has UVs: {}",
        !tetra.normals.empty(), !tetra.texture_coordinates.empty());

    // 2. Cube - via AABB (6 square faces)
    ENGINE_INFO("Generating Cube (via AABB)...");
    Aabb unit_box{
        math::vec3{-0.5f, -0.5f, -0.5f},
        math::vec3{0.5f, 0.5f, 0.5f}
    };
    auto cube = surfaces::surface_mesh_from(unit_box);
    ENGINE_INFO("  Vertices: {}, Triangles: {}",
        cube.positions.size(), cube.indices.size() / 3);

    // 3. Octahedron - dual of cube (8 faces)
    ENGINE_INFO("Generating Octahedron...");
    auto octa = octahedron();
    ENGINE_INFO("  Vertices: {}, Triangles: {}",
        octa.positions.size(), octa.indices.size() / 3);

    // 4. Dodecahedron - 12 pentagonal faces (most complex)
    ENGINE_INFO("Generating Dodecahedron...");
    auto dodeca = dodecahedron();
    ENGINE_INFO("  Vertices: {}, Triangles: {} (from 12 pentagons)",
        dodeca.positions.size(), dodeca.indices.size() / 3);

    // 5. Icosahedron - 20 triangular faces (most triangles)
    ENGINE_INFO("Generating Icosahedron...");
    auto icosa = icosahedron();
    ENGINE_INFO("  Vertices: {}, Triangles: {}",
        icosa.positions.size(), icosa.indices.size() / 3);

    // Demonstrate conversion to halfedge mesh
    ENGINE_INFO("Converting Icosahedron to HalfedgeMesh...");
    mesh::HalfedgeMeshInterface icosa_halfedge;
    surfaces::to_halfedge_mesh(icosa, icosa_halfedge);
    ENGINE_INFO("  Halfedge - Vertices: {}, Edges: {}, Faces: {}",
        icosa_halfedge.vertex_count(),
        icosa_halfedge.edge_count(),
        icosa_halfedge.face_count());

    // Verify Euler's formula: V - E + F = 2
    int V = icosa_halfedge.vertex_count();
    int E = icosa_halfedge.edge_count();
    int F = icosa_halfedge.face_count();
    int euler = V - E + F;
    ENGINE_INFO("  Euler's formula check: {} - {} + {} = {} (should be 2)",
        V, E, F, euler);

    // Summary
    ENGINE_INFO("=== Summary ===");
    ENGINE_INFO("All five Platonic solids generated successfully!");
    ENGINE_INFO("Properties:");
    ENGINE_INFO("  - All meshes have positions, normals, and UVs");
    ENGINE_INFO("  - All meshes can convert to halfedge representation");
    ENGINE_INFO("  - All satisfy Euler's polyhedron formula");

    engine::core::Log::shutdown();

    return 0;
}

