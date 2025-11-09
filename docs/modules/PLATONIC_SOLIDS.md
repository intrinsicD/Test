# Platonic Solids Mesh Generation

## Overview

The engine provides functions to generate meshes for all five Platonic solids - the regular convex polyhedra. These are perfect for procedural geometry, collision meshes, and testing.

## Header

```cpp
#include "engine/geometry/surfaces/platonic_solids.hpp"
```

## The Five Platonic Solids

### 1. Tetrahedron
**4 vertices, 4 triangular faces**

```cpp
SurfaceMesh mesh = engine::geometry::tetrahedron();
```

Properties:
- Regular tetrahedron (all faces are equilateral triangles)
- 4 vertices at `(±a, ±a, ±a)` where `a = 1/√3`
- 4 faces
- Centered at origin
- Each face is an equilateral triangle

### 2. Octahedron
**6 vertices, 8 triangular faces**

```cpp
SurfaceMesh mesh = engine::geometry::octahedron();
```

Properties:
- Regular octahedron (8 equilateral triangular faces)
- 6 vertices at `(±1, 0, 0)`, `(0, ±1, 0)`, `(0, 0, ±1)`
- 8 faces (4 on top pyramid, 4 on bottom pyramid)
- Centered at origin
- Dual of the cube

### 3. Dodecahedron
**20 vertices, 12 pentagonal faces (36 triangles)**

```cpp
SurfaceMesh mesh = engine::geometry::dodecahedron();
```

Properties:
- Regular dodecahedron (12 regular pentagonal faces)
- 20 vertices using golden ratio φ = (1 + √5) / 2
- 12 pentagonal faces triangulated into 36 triangles
- Vertices include:
  - 8 cube vertices at `(±1, ±1, ±1)`
  - 12 vertices on rectangles using φ and 1/φ
- Centered at origin
- Dual of the icosahedron

### 4. Icosahedron
**12 vertices, 20 triangular faces**

```cpp
SurfaceMesh mesh = engine::geometry::icosahedron();
```

Properties:
- Regular icosahedron (20 equilateral triangular faces)
- 12 vertices using golden ratio φ = (1 + √5) / 2
- Vertices form 3 golden rectangles
- 20 faces
- Centered at origin
- Dual of the dodecahedron
- Often used as base for subdivided spheres

### 5. Cube
**Use AABB mesh generation**

```cpp
Aabb box{math::vec3{-0.5f, -0.5f, -0.5f}, math::vec3{0.5f, 0.5f, 0.5f}};
SurfaceMesh mesh = surfaces::surface_mesh_from(box);
```

Note: The cube (hexahedron) is already available via AABB mesh generation.

## Complete Example

```cpp
#include "engine/geometry/surfaces/platonic_solids.hpp"
#include "engine/geometry/surfaces/mesh_generation.hpp"
#include "engine/core/log.hpp"

int main()
{
    using namespace engine::geometry;
    
    engine::core::Log::init();
    
    // Generate all five Platonic solids
    
    // 1. Tetrahedron (4 faces)
    auto tetra = tetrahedron();
    ENGINE_INFO("Tetrahedron: {} vertices, {} triangles",
        tetra.positions.size(), tetra.indices.size() / 3);
    
    // 2. Cube (6 faces) - via AABB
    Aabb unit_box{
        math::vec3{-0.5f, -0.5f, -0.5f},
        math::vec3{0.5f, 0.5f, 0.5f}
    };
    auto cube = surfaces::surface_mesh_from(unit_box);
    ENGINE_INFO("Cube: {} vertices, {} triangles",
        cube.positions.size(), cube.indices.size() / 3);
    
    // 3. Octahedron (8 faces)
    auto octa = octahedron();
    ENGINE_INFO("Octahedron: {} vertices, {} triangles",
        octa.positions.size(), octa.indices.size() / 3);
    
    // 4. Dodecahedron (12 pentagonal faces = 36 triangles)
    auto dodeca = dodecahedron();
    ENGINE_INFO("Dodecahedron: {} vertices, {} triangles",
        dodeca.positions.size(), dodeca.indices.size() / 3);
    
    // 5. Icosahedron (20 faces)
    auto icosa = icosahedron();
    ENGINE_INFO("Icosahedron: {} vertices, {} triangles",
        icosa.positions.size(), icosa.indices.size() / 3);
    
    engine::core::Log::shutdown();
    return 0;
}
```

Output:
```
[INFO] APP: Tetrahedron: 4 vertices, 4 triangles
[INFO] APP: Cube: 24 vertices, 12 triangles
[INFO] APP: Octahedron: 6 vertices, 8 triangles
[INFO] APP: Dodecahedron: 20 vertices, 36 triangles
[INFO] APP: Icosahedron: 12 vertices, 20 triangles
```

## Properties Table

| Solid | Vertices | Edges | Faces | Face Type | Triangles |
|-------|----------|-------|-------|-----------|-----------|
| **Tetrahedron** | 4 | 6 | 4 | Triangle | 4 |
| **Cube** | 8 | 12 | 6 | Square | 12 |
| **Octahedron** | 6 | 12 | 8 | Triangle | 8 |
| **Dodecahedron** | 20 | 30 | 12 | Pentagon | 36 |
| **Icosahedron** | 12 | 30 | 20 | Triangle | 20 |

## Use Cases

### Collision Meshes
Platonic solids make excellent simple collision shapes:

```cpp
// Use icosahedron as sphere approximation for collision
auto collision_mesh = engine::geometry::icosahedron();
// Scale to desired radius
for (auto& pos : collision_mesh.positions) {
    pos = pos * desired_radius;
}
```

### Level of Detail (LOD)
Use different platonic solids for different LOD levels:

```cpp
// Far: Tetrahedron (4 triangles)
auto lod_far = tetrahedron();

// Medium: Octahedron (8 triangles)
auto lod_medium = octahedron();

// Near: Icosahedron (20 triangles)
auto lod_near = icosahedron();
```

### Procedural Generation
Use as base shapes for subdivision or extrusion:

```cpp
// Start with icosahedron for sphere subdivision
auto base = icosahedron();
// Apply subdivision algorithm to get smoother sphere
// (subdivision not shown)
```

### Testing and Visualization
Perfect for testing rendering pipelines:

```cpp
// Test face culling
auto test_mesh = octahedron();

// Test normal visualization
auto dodeca = dodecahedron();
// Normals are precomputed and available in dodeca.normals
```

## Mathematical Background

### Golden Ratio
The dodecahedron and icosahedron use the golden ratio:
```
φ = (1 + √5) / 2 ≈ 1.618
```

This appears in their vertex coordinates and gives them their perfect symmetry.

### Euler's Formula
All Platonic solids satisfy Euler's polyhedron formula:
```
V - E + F = 2
```
Where V = vertices, E = edges, F = faces

Examples:
- Tetrahedron: 4 - 6 + 4 = 2 ✓
- Cube: 8 - 12 + 6 = 2 ✓
- Octahedron: 6 - 12 + 8 = 2 ✓
- Dodecahedron: 20 - 30 + 12 = 2 ✓
- Icosahedron: 12 - 30 + 20 = 2 ✓

### Duality
Platonic solids come in dual pairs:
- **Tetrahedron** is self-dual
- **Cube ↔ Octahedron** are duals
- **Dodecahedron ↔ Icosahedron** are duals

## Mesh Properties

All generated meshes include:
- ✅ **Positions**: Vertex positions (rest and current)
- ✅ **Normals**: Per-vertex normals (averaged from face normals)
- ✅ **UVs**: Texture coordinates (spherical projection)
- ✅ **Indices**: Triangle indices (counter-clockwise winding)
- ✅ **Bounds**: AABB bounding box

## Conversion to Halfedge

Convert any Platonic solid to halfedge representation:

```cpp
auto icosa_surface = engine::geometry::icosahedron();

mesh::HalfedgeMeshInterface icosa_halfedge;
surfaces::to_halfedge_mesh(icosa_surface, icosa_halfedge);

ENGINE_INFO("Halfedge icosahedron: {} vertices, {} edges, {} faces",
    icosa_halfedge.vertex_count(),
    icosa_halfedge.edge_count(),
    icosa_halfedge.face_count());
```

## Performance Notes

- All Platonic solids are generated at runtime
- Generation is very fast (microseconds)
- Consider caching if used frequently
- Vertex/index counts are small (suitable for embedding)

## Implementation Details

- **Vertex positions**: Computed using exact mathematical coordinates
- **Normals**: Averaged from adjacent face normals for smooth shading
- **UVs**: Spherical projection for most solids
- **Winding**: Counter-clockwise (front-facing)
- **Centering**: All solids centered at origin
- **Scale**: Approximately unit-sized (radius ≈ 1)

## See Also

- Surface mesh generation: `engine/geometry/surfaces/mesh_generation.hpp`
- Shape primitives: `engine/geometry/shapes.hpp`
- Mesh subdivision: `engine/geometry/remesh/`

## References

- Wikipedia: [Platonic Solid](https://en.wikipedia.org/wiki/Platonic_solid)
- Source: `engine/geometry/src/surfaces/platonic_solids.cpp`
- Header: `engine/geometry/include/engine/geometry/surfaces/platonic_solids.hpp`

