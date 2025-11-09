# Surface Mesh Generation System

## Overview

The engine provides a comprehensive mesh generation system in `engine/geometry/surfaces` that creates triangle meshes (`SurfaceMesh`) and halfedge meshes (`HalfedgeMesh`) from geometric primitives.

## Header

```cpp
#include "engine/geometry/surfaces/mesh_generation.hpp"
```

## Available Shape→Mesh Functions

### SurfaceMesh Generation

All functions return a `SurfaceMesh` with positions, normals, UVs, and indices.

#### AABB (Axis-Aligned Bounding Box)
```cpp
Aabb box{math::vec3{-1, -1, -1}, math::vec3{1, 1, 1}};
SurfaceMesh mesh = surfaces::surface_mesh_from(box);
```

#### Sphere
```cpp
Sphere sphere{math::vec3{0, 0, 0}, 1.0f};
SurfaceMesh mesh = surfaces::surface_mesh_from(sphere, 2);  // subdivisions = 2
```

#### Cylinder
```cpp
Cylinder cylinder{center, axis, radius, half_height};
SurfaceMesh mesh = surfaces::surface_mesh_from(
    cylinder, 
    32,    // radial_segments
    1,     // height_segments
    true   // with_caps
);
```

#### Capsule
```cpp
Capsule capsule{point_a, point_b, radius};
SurfaceMesh mesh = surfaces::surface_mesh_from(
    capsule,
    32,  // radial_segments
    1,   // height_segments  
    8    // hemisphere_rings
);
```

#### Ellipsoid
```cpp
Ellipsoid ellipsoid{center, radii, orientation};
SurfaceMesh mesh = surfaces::surface_mesh_from(
    ellipsoid,
    16,  // lat_segments
    32   // lon_segments
);
```

#### OBB (Oriented Bounding Box)
```cpp
Obb box{center, extent, axes};
SurfaceMesh mesh = surfaces::surface_mesh_from(box);
```

#### Plane
```cpp
Plane plane{normal, distance};
SurfaceMesh mesh = surfaces::surface_mesh_from(
    plane,
    10.0f,  // width
    10.0f,  // height
    10,     // width_segments
    10      // height_segments
);
```

#### Triangle
```cpp
Triangle triangle{point_a, point_b, point_c};
SurfaceMesh mesh = surfaces::surface_mesh_from(triangle);
```

#### Frustum
```cpp
Frustum frustum{/* 8 corner points */};
SurfaceMesh mesh = surfaces::surface_mesh_from(frustum);
```

### HalfedgeMesh Generation

All shapes also support direct halfedge mesh generation:

```cpp
mesh::HalfedgeMeshInterface halfedge_mesh;

// AABB example
Aabb box{...};
surfaces::halfedge_mesh_from(box, halfedge_mesh);

// Sphere example  
Sphere sphere{...};
surfaces::halfedge_mesh_from(sphere, halfedge_mesh, 2);

// Cylinder example
Cylinder cylinder{...};
surfaces::halfedge_mesh_from(cylinder, halfedge_mesh, 32, 1, true);
```

## Mesh Conversion

Convert between mesh representations:

```cpp
// SurfaceMesh → HalfedgeMesh
SurfaceMesh surface = ...;
mesh::HalfedgeMeshInterface halfedge;
surfaces::to_halfedge_mesh(surface, halfedge);

// HalfedgeMesh → SurfaceMesh
SurfaceMesh surface = surfaces::to_surface_mesh(halfedge);
```

## Complete Example

```cpp
#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/geometry/shapes.hpp"
#include "engine/core/log.hpp"

int main()
{
    using namespace engine::geometry;
    
    // Initialize logging
    engine::core::Log::init();
    
    // Create various meshes from shapes
    
    // 1. Cube from AABB
    Aabb unit_box{
        math::vec3{-0.5f, -0.5f, -0.5f},
        math::vec3{0.5f, 0.5f, 0.5f}
    };
    auto cube_mesh = surfaces::surface_mesh_from(unit_box);
    ENGINE_INFO("Cube: {} vertices, {} triangles", 
        cube_mesh.positions.size(), cube_mesh.indices.size() / 3);
    
    // 2. Sphere
    Sphere unit_sphere{math::vec3{0, 0, 0}, 1.0f};
    auto sphere_mesh = surfaces::surface_mesh_from(unit_sphere, 3);
    ENGINE_INFO("Sphere: {} vertices, {} triangles",
        sphere_mesh.positions.size(), sphere_mesh.indices.size() / 3);
    
    // 3. Cylinder
    Cylinder cylinder{
        math::vec3{0, 0, 0},      // center
        math::vec3{0, 1, 0},      // axis
        0.5f,                     // radius
        1.0f                      // half_height
    };
    auto cylinder_mesh = surfaces::surface_mesh_from(cylinder, 24, 1, true);
    ENGINE_INFO("Cylinder: {} vertices, {} triangles",
        cylinder_mesh.positions.size(), cylinder_mesh.indices.size() / 3);
    
    // 4. Capsule
    Capsule capsule{
        math::vec3{0, -1, 0},     // point_a
        math::vec3{0, 1, 0},      // point_b
        0.5f                      // radius
    };
    auto capsule_mesh = surfaces::surface_mesh_from(capsule, 24, 2, 8);
    ENGINE_INFO("Capsule: {} vertices, {} triangles",
        capsule_mesh.positions.size(), capsule_mesh.indices.size() / 3);
    
    // 5. Ellipsoid (scaled and rotated sphere)
    Ellipsoid ellipsoid{
        math::vec3{0, 0, 0},                          // center
        math::vec3{1.0f, 0.5f, 0.8f},                 // radii (x, y, z)
        math::quat{1.0f, 0.0f, 0.0f, 0.0f}            // orientation
    };
    auto ellipsoid_mesh = surfaces::surface_mesh_from(ellipsoid, 16, 32);
    ENGINE_INFO("Ellipsoid: {} vertices, {} triangles",
        ellipsoid_mesh.positions.size(), ellipsoid_mesh.indices.size() / 3);
    
    // 6. Convert to halfedge mesh
    mesh::HalfedgeMeshInterface halfedge_cube;
    surfaces::to_halfedge_mesh(cube_mesh, halfedge_cube);
    ENGINE_INFO("Halfedge cube: {} vertices, {} edges, {} faces",
        halfedge_cube.vertex_count(), 
        halfedge_cube.edge_count(),
        halfedge_cube.face_count());
    
    // 7. Convert back to surface mesh
    auto cube_mesh2 = surfaces::to_surface_mesh(halfedge_cube);
    ENGINE_INFO("Converted back: {} vertices, {} triangles",
        cube_mesh2.positions.size(), cube_mesh2.indices.size() / 3);
    
    engine::core::Log::shutdown();
    return 0;
}
```

## Implementation Details

### Ellipsoid Generation

Ellipsoids are created by:
1. Generating a unit sphere at the origin
2. Applying non-uniform scaling by the radii
3. Rotating by the orientation quaternion
4. Translating to the center position

This is an affine transformation that correctly handles both positions and normals.

### Mesh Quality

- **Normals**: All meshes include per-vertex normals
- **UVs**: Texture coordinates are generated for all shapes
- **Topology**: All meshes are manifold and suitable for halfedge conversion
- **Bounds**: AABB bounds are automatically computed

### Performance Notes

- Sphere subdivision: Each level doubles vertex count (16→64→256→1024...)
- Cylinder/Capsule: Vertex count = (radial_segments + 1) × (height_segments + 1) + caps
- Caching: Consider caching frequently used meshes

## Source Files

- Header: `engine/geometry/include/engine/geometry/surfaces/mesh_generation.hpp`
- Implementations:
  - `engine/geometry/src/surfaces/mesh_generation.cpp` (conversions)
  - `engine/geometry/src/surfaces/aabb_mesh.cpp`
  - `engine/geometry/src/surfaces/sphere_mesh.cpp`
  - `engine/geometry/src/surfaces/cylinder_mesh.cpp`
  - `engine/geometry/src/surfaces/basic_shapes_mesh.cpp`
  - `engine/geometry/src/surfaces/capsule_ellipsoid_mesh.cpp`

## Usage in Geometry Viewer

```cpp
// Create a shape
Sphere sphere{math::vec3{0, 0, 0}, 1.0f};

// Generate mesh
auto mesh = engine::geometry::surfaces::surface_mesh_from(sphere, 2);

// Store and use in rendering
mesh_storage->store("my_sphere", std::move(mesh));
```

## See Also

- Existing API functions: `make_unit_cube()`, `make_unit_quad()`, `mesh_from_aabb()`
- These are implemented using the new surface generation system
- Halfedge mesh documentation in `engine/geometry/mesh/`

