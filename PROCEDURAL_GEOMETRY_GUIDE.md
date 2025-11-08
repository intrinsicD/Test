# 📐 Procedural Geometry Creation Guide

**Date:** November 9, 2025  
**Status:** ✅ Complete with working examples

---

## Quick Start

### Creating a Procedural Cube

```cpp
#include "engine/geometry/api.hpp"

// Create a unit cube (1x1x1 centered at origin)
auto cube = engine::geometry::make_unit_cube();

// Cube has:
// - 24 vertices (duplicated for proper per-face normals)
// - 36 indices (12 triangles, 2 per face, 6 faces)
// - Per-vertex normals
// - Texture coordinates
// - Bounding box
```

### Creating a Procedural Quad

```cpp
// Create a unit quad (1x1 on XZ plane)
auto quad = engine::geometry::make_unit_quad();

// Quad has:
// - 4 vertices
// - 6 indices (2 triangles)
// - Per-vertex normals
// - UV coordinates
```

---

## Available Procedural Shapes

### ✅ Implemented

| Function | Description | Vertices | Triangles |
|----------|-------------|----------|-----------|
| `make_unit_cube()` | 1x1x1 cube centered at origin | 24 | 12 |
| `make_unit_quad()` | 1x1 quad on XZ plane | 4 | 2 |

### 🚧 To Implement (Future)

- `make_sphere(subdivisions)` - UV sphere
- `make_icosphere(subdivisions)` - Icosahedral sphere
- `make_cylinder(height, radius, segments)` - Cylinder
- `make_cone(height, radius, segments)` - Cone
- `make_torus(major_radius, minor_radius, segments)` - Torus
- `make_plane(width, height, subdivisions)` - Subdivided plane

---

## SurfaceMesh Structure

```cpp
struct SurfaceMesh {
    std::vector<math::vec3> rest_positions;      // Original vertex positions
    std::vector<math::vec3> positions;           // Current vertex positions
    std::vector<math::vec3> normals;             // Per-vertex normals
    std::vector<std::uint32_t> indices;          // Triangle indices
    std::vector<math::vec2> texture_coordinates; // UV coordinates
    Aabb bounds;                                  // Bounding box
};
```

---

## Mesh Operations

### Transform

```cpp
auto mesh = engine::geometry::make_unit_cube();

// Translate
engine::geometry::apply_uniform_translation(mesh, 
    engine::math::vec3{1.0f, 2.0f, 3.0f});
```

### Recalculate Normals

```cpp
// After modifying vertex positions
engine::geometry::recompute_vertex_normals(mesh);
```

### Update Bounds

```cpp
// After modifying vertices
engine::geometry::update_bounds(mesh);
```

### Calculate Properties

```cpp
// Get centroid
auto center = engine::geometry::centroid(mesh);

// Get surface area
float area = engine::geometry::surface_area(mesh);
```

### Save/Load

```cpp
// Save to file
engine::geometry::save_surface_mesh(mesh, "my_mesh.mesh");

// Load from file
auto loaded = engine::geometry::load_surface_mesh("my_mesh.mesh");
```

---

## Complete Example

See `engine/tools/examples/procedural_cube_example.cpp`:

```cpp
#include <iostream>
#include "engine/geometry/api.hpp"

int main()
{
    // Create procedural cube
    auto cube = engine::geometry::make_unit_cube();
    
    std::cout << "Vertices: " << cube.positions.size() << "\n";
    std::cout << "Triangles: " << (cube.indices.size() / 3) << "\n";
    
    // Transform
    engine::geometry::apply_uniform_translation(cube, 
        engine::math::vec3{1.0f, 0.0f, 0.0f});
    
    // Calculate properties
    auto center = engine::geometry::centroid(cube);
    auto area = engine::geometry::surface_area(cube);
    
    std::cout << "Centroid: (" << center[0] << ", " 
              << center[1] << ", " << center[2] << ")\n";
    std::cout << "Surface area: " << area << "\n";
    
    return 0;
}
```

### Build & Run

```bash
# Build
cmake --build cmake-build-debug --target procedural_cube_example

# Run
cmake-build-debug/engine/tools/examples/procedural_cube_example
```

**Output:**
```
=== Procedural Geometry Creation Example ===

Creating procedural cube...
  ✓ Vertices: 24
  ✓ Indices: 36
  ✓ Triangles: 12
  ✓ Normals: 24

Mesh bounds:
  Min: (-0.5, -0.5, -0.5)
  Max: (0.5, 0.5, 0.5)

Creating procedural quad...
  ✓ Vertices: 4
  ✓ Indices: 6

Transforming mesh...
  ✓ Translated by (1, 0, 0)

Mesh properties:
  Centroid: (1, 0, 0)
  Surface area: 6

=== Complete! ===
```

---

## Using in Rendering Pipeline

### Current Status

**✅ What Works:**
- Creating procedural meshes
- Manipulating mesh data
- Saving/loading meshes
- Window clearing with OpenGL backend

**🚧 What's Missing:**
- Integration between SurfaceMesh and rendering pipeline
- MeshHandle resolution system
- Asset storage accessible to MeshResolver

### The Challenge

The rendering pipeline uses this flow:

```
Entity with RenderGeometry
    ↓ (contains MeshHandle)
OpenGL Backend needs mesh data
    ↓ (calls MeshResolver lambda)
Lambda returns SurfaceMesh
    ↓
Backend renders it
```

**Problem:** The MeshResolver lambda currently expects meshes to be stored in RuntimeHost, which is designed for file-loaded assets, not procedural geometry.

### Solution Options

#### Option 1: Quick Workaround (Current State)
**Don't add RenderGeometry component**
- Window clears properly showing pipeline works
- No geometry rendered (expected)
- No crashes ✅

#### Option 2: Extend RuntimeHost
**Store procedural meshes in RuntimeHost**
```cpp
// Add to RuntimeHost:
void store_procedural_mesh(const std::string& name, SurfaceMesh mesh);
std::optional<SurfaceMesh> get_procedural_mesh(const std::string& name);

// In app:
runtime_host.store_procedural_mesh("my_cube", make_unit_cube());
auto handle = MeshHandle{"my_cube"};
registry.emplace<RenderGeometry>(entity, 
    RenderGeometry::from_mesh(handle, material));
```

#### Option 3: Custom MeshResolver
**Override the default resolver**
```cpp
// In Application initialization:
struct MyMeshStorage {
    std::unordered_map<std::string, SurfaceMesh> meshes;
};

MyMeshStorage storage;
storage.meshes["cube"] = make_unit_cube();

auto backend = std::make_shared<OpenGLPresentationBackend>(
    [&storage](const MeshHandle& handle) -> std::optional<SurfaceMesh> {
        auto it = storage.meshes.find(handle.path());
        if (it != storage.meshes.end()) {
            return it->second;
        }
        return std::nullopt;
    }
);
```

#### Option 4: Asset System (Proper Solution)
**Implement full asset management**
- Register procedural meshes in asset registry
- Generate proper asset IDs
- Use asset system for all geometry
- Requires significant architecture work

---

## Current Workaround for Rendering

Until the integration is complete, use this pattern:

```cpp
// In geometry_viewer.cpp setup_scene():

auto& registry = scene().registry();
auto cube = registry.create();

// Add transform (works)
auto& transform = registry.emplace<WorldTransform>(cube);
transform.value = Transform<float>::Identity();

// DON'T add RenderGeometry yet (causes crash without proper resolver)
// auto handle = MeshHandle{"my_cube"};
// registry.emplace<RenderGeometry>(cube, 
//     RenderGeometry::from_mesh(handle, material));

// Instead: Window clears properly, showing rendering pipeline works
```

**Result:** 
- ✅ Window opens and clears to blue-grey
- ✅ 60 FPS rendering loop
- ✅ Camera controls work
- ✅ Frame graph executes
- ❌ No visible geometry (expected)

---

## Next Steps

To get visible geometry rendering:

### 1. Implement Option 2 or 3
Choose a mesh storage strategy and implement it.

### 2. Create Material
```cpp
// Currently materials are also asset-based
// Need similar solution for procedural materials
```

### 3. Update geometry_viewer
```cpp
void setup_scene() {
    // Create and store mesh
    auto cube_mesh = engine::geometry::make_unit_cube();
    store_mesh("procedural_cube", cube_mesh);
    
    // Create entity with geometry
    auto entity = registry.create();
    auto& transform = registry.emplace<WorldTransform>(entity);
    transform.value = Transform<float>::Identity();
    
    auto handle = MeshHandle{"procedural_cube"};
    auto material = MaterialHandle{"default"};
    registry.emplace<RenderGeometry>(entity,
        RenderGeometry::from_mesh(handle, material));
}
```

### 4. Implement Shader/Material System
The geometry needs shaders to render. Currently the material system expects file-based assets.

---

## Summary

**✅ Procedural Geometry Creation:** WORKING
```bash
cmake-build-debug/engine/tools/examples/procedural_cube_example
```

**✅ Rendering Pipeline:** WORKING (window clears)
```bash
cmake-build-debug/engine/tools/examples/geometry_viewer
```

**🚧 Geometry Rendering:** BLOCKED
- Need mesh storage accessible to MeshResolver
- Need material/shader system for procedural content
- Options documented above

**Files:**
- API: `engine/geometry/include/engine/geometry/api.hpp`
- Implementation: `engine/geometry/src/api.cpp`
- Example: `engine/tools/examples/procedural_cube_example.cpp`
- Viewer: `engine/tools/examples/geometry_viewer.cpp`

---

## Quick Reference

```cpp
// Create shapes
auto cube = make_unit_cube();      // 24 verts, 12 tris
auto quad = make_unit_quad();      // 4 verts, 2 tris

// Transform
apply_uniform_translation(mesh, offset);

// Properties
auto center = centroid(mesh);
auto area = surface_area(mesh);

// Maintenance
recompute_vertex_normals(mesh);
update_bounds(mesh);

// I/O
save_surface_mesh(mesh, "file.mesh");
auto loaded = load_surface_mesh("file.mesh");
```

---

**The geometry creation API is complete and working. Integration with the rendering pipeline requires architectural decisions about asset/mesh storage.**

