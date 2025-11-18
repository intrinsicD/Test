/**
 * @file procedural_cube_example.cpp
 * @brief Example showing how to create procedural geometry assets
 *
 * This demonstrates:
 * 1. Creating a procedural mesh using engine::geometry::make_unit_cube()
 * 2. Storing it in the scene for rendering
 * 3. Accessing it through the rendering pipeline
 */

#include <iostream>
#include "engine/geometry/api.hpp"

int main()
{
    std::cout << "=== Procedural Geometry Creation Example ===\n\n";

    // Step 1: Create a procedural cube mesh
    std::cout << "Creating procedural cube...\n";
    auto cube_mesh = engine::geometry::make_unit_cube();

    std::cout << "  ✓ Vertices: " << cube_mesh.positions.size() << "\n";
    std::cout << "  ✓ Indices: " << cube_mesh.indices.size() << "\n";
    std::cout << "  ✓ Triangles: " << (cube_mesh.indices.size() / 3) << "\n";
    std::cout << "  ✓ Normals: " << cube_mesh.normals.size() << "\n";

    // Step 2: Inspect the mesh data
    std::cout << "\nMesh bounds:\n";
    std::cout << "  Min: ("
              << cube_mesh.bounds.min[0] << ", "
              << cube_mesh.bounds.min[1] << ", "
              << cube_mesh.bounds.min[2] << ")\n";
    std::cout << "  Max: ("
              << cube_mesh.bounds.max[0] << ", "
              << cube_mesh.bounds.max[1] << ", "
              << cube_mesh.bounds.max[2] << ")\n";

    // Step 3: You can also create meshes from custom AABBs
    std::cout << "\nCreating mesh from custom AABB...\n";
    engine::geometry::Aabb custom_box{
        engine::math::vec3{0.0f, 0.0f, 0.0f},   // min
        engine::math::vec3{2.0f, 1.0f, 0.5f}    // max (2x1x0.5 box)
    };
    auto custom_mesh = engine::geometry::surface_mesh_from(custom_box);
    std::cout << "  ✓ Custom box (2x1x0.5): " << custom_mesh.positions.size() << " vertices\n";
    
    // Step 4: You can also create other procedural shapes
    std::cout << "\nCreating procedural quad...\n";
    auto quad_mesh = engine::geometry::make_unit_quad();

    std::cout << "  ✓ Vertices: " << quad_mesh.positions.size() << "\n";
    std::cout << "  ✓ Indices: " << quad_mesh.indices.size() << "\n";
    
    // Step 5: Meshes can be transformed
    std::cout << "\nTransforming mesh...\n";
    engine::geometry::apply_uniform_translation(cube_mesh, engine::math::vec3{1.0f, 0.0f, 0.0f});
    std::cout << "  ✓ Translated by (1, 0, 0)\n";
    
    // Step 6: Calculate properties
    auto centroid = engine::geometry::centroid(cube_mesh);
    auto surface_area = engine::geometry::surface_area(cube_mesh);

    std::cout << "\nMesh properties:\n";
    std::cout << "  Centroid: ("
              << centroid[0] << ", "
              << centroid[1] << ", "
              << centroid[2] << ")\n";
    std::cout << "  Surface area: " << surface_area << "\n";
    
    // Step 7: Save to file (optional)
    // engine::geometry::save_surface_mesh(cube_mesh, "my_cube.mesh");

    std::cout << "\n=== Complete! ===\n";
    std::cout << "\nTo use this mesh in rendering:\n";
    std::cout << "1. Store the SurfaceMesh in a container accessible to your app\n";
    std::cout << "2. Create a MeshHandle that references it\n";
    std::cout << "3. Add RenderGeometry component with the handle\n";
    std::cout << "4. The rendering backend will resolve the handle and draw it\n";

    return 0;
}

