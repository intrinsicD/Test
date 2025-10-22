# Geometry Module

## Overview

The geometry module provides core geometric data structures (meshes, point clouds, graphs), spatial acceleration structures (kd-trees, octrees), deformation algorithms (linear blend skinning), procedural primitive generation, and IO integration for import/export workflows.

## Core Data Structures

### Surface Meshes

`SurfaceMesh` represents indexed triangle meshes with optional vertex attributes:

```cpp
#include "engine/geometry/api.hpp"

geometry::SurfaceMesh mesh;
mesh.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
mesh.indices = {0, 1, 2};
mesh.normals = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};

// Optional attributes
mesh.uvs = {{0, 0}, {1, 0}, {0, 1}};
mesh.colors = {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}};
```

Meshes automatically maintain:
- **Bounds (AABB)**: `mesh.bounds` updated on vertex modification
- **Centroid**: `mesh.centroid` computed from vertex positions
- **Connectivity**: Optional halfedge data structure for topology queries

### Point Clouds

`PointCloud` stores unconnected point data with attributes:

```cpp
geometry::PointCloud cloud;
cloud.positions = {{0, 0, 0}, {1, 1, 1}, {2, 2, 2}};
cloud.colors = {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}};
cloud.normals = {{0, 0, 1}, {0, 0, 1}, {0, 0, 1}};
```

### Graphs

`Graph` represents connectivity data:

```cpp
geometry::Graph graph;
graph.add_node(0, {.position = {0, 0, 0}});
graph.add_node(1, {.position = {1, 0, 0}});
graph.add_edge(0, 1, {.weight = 1.0});

// Query neighbors
auto neighbors = graph.neighbors(0);
```

## Procedural Primitives

Generate common geometric shapes:

```cpp
// Quad (2 triangles)
auto quad = geometry::make_unit_quad();

// Cube with optional subdivision
auto cube = geometry::make_unit_cube(/*subdivisions=*/1);

// Sphere (UV sphere or icosphere)
auto sphere = geometry::make_uv_sphere(/*radius=*/1.0f, /*segments=*/32, /*rings=*/16);
auto ico_sphere = geometry::make_icosphere(/*radius=*/1.0f, /*subdivisions=*/2);

// Cylinder
auto cylinder = geometry::make_cylinder(/*radius=*/1.0f, /*height=*/2.0f, /*segments=*/32);

// Torus
auto torus = geometry::make_torus(/*major_radius=*/1.0f, /*minor_radius=*/0.3f);
```

All primitives include proper normals, UVs, and tangents where applicable.

## Spatial Acceleration

### KD-Tree

Fast nearest-neighbor and range queries:

```cpp
#include "engine/geometry/kdtree/kdtree.hpp"

geometry::KdTree tree(mesh.positions);

// Nearest neighbor
auto nearest = tree.nearest({0, 0, 0});
fmt::print("Nearest point index: {}\n", nearest.index);

// K-nearest neighbors
auto k_nearest = tree.k_nearest({0, 0, 0}, /*k=*/10);

// Range query
auto in_radius = tree.radius_search({0, 0, 0}, /*radius=*/5.0f);
```

### Octree

Hierarchical spatial partitioning:

```cpp
#include "engine/geometry/octree/octree.hpp"

geometry::Octree octree(mesh.positions, /*max_depth=*/8, /*max_points_per_leaf=*/32);

// Query points in AABB
geometry::Aabb query_box{{-1, -1, -1}, {1, 1, 1}};
auto points_in_box = octree.query(query_box);

// Frustum culling
geometry::Frustum frustum = compute_frustum_from_camera(camera);
auto visible_points = octree.query(frustum);
```

Both structures automatically rebuild when mesh topology changes.

## Deformation

### Linear Blend Skinning (LBS)

Apply skeletal animation to meshes:

```cpp
#include "engine/geometry/deform/linear_blend_skinning.hpp"

// Binding prepared by animation module
animation::RigBinding binding = /*...*/;
std::vector<math::mat4> joint_transforms = /*...*/;

// Apply deformation
geometry::apply_linear_blend_skinning(mesh, joint_transforms, binding.joint_weights);

// Mesh positions, normals updated in-place
// Bounds and centroid recomputed automatically
```

### Other Deformers

```cpp
// Lattice-based free-form deformation
geometry::apply_ffd(mesh, lattice);

// Laplacian smoothing
geometry::smooth_laplacian(mesh, /*iterations=*/5, /*lambda=*/0.5f);
```

## Topology Operations

### Halfedge Mesh Conversion

Convert to/from halfedge representation for advanced queries:

```cpp
auto halfedge_mesh = geometry::to_halfedge(mesh);

// Traverse around vertex
for (auto he : halfedge_mesh.outgoing_halfedges(vertex_id)) {
    auto target = halfedge_mesh.target(he);
    // Process edge
}

// Convert back
auto indexed_mesh = geometry::from_halfedge(halfedge_mesh);
```

### Topology Queries

```cpp
// Compute vertex valence (number of adjacent edges)
auto valence = geometry::vertex_valence(mesh, vertex_id);

// Find boundary edges
auto boundary = geometry::extract_boundary(mesh);

// Compute face adjacency
auto adjacent_faces = geometry::face_neighbors(mesh, face_id);
```

## Geometric Computations

### Normals & Tangents

```cpp
// Compute per-vertex normals (area-weighted)
geometry::compute_normals(mesh);

// Flat shading normals (per-face)
geometry::compute_flat_normals(mesh);

// Tangent space for normal mapping
geometry::compute_tangents(mesh);
```

### Bounds & Centroid

```cpp
// AABB
geometry::Aabb bounds = geometry::compute_bounds(mesh);
fmt::print("Min: ({}, {}, {})\n", bounds.min.x, bounds.min.y, bounds.min.z);
fmt::print("Max: ({}, {}, {})\n", bounds.max.x, bounds.max.y, bounds.max.z);

// Centroid
math::vec3 centroid = geometry::compute_centroid(mesh);

// Bounding sphere
geometry::Sphere bounding_sphere = geometry::compute_bounding_sphere(mesh);
```

### Intersections

```cpp
// Ray-mesh intersection
geometry::Ray ray{{0, 0, -10}, {0, 0, 1}};
auto hit = geometry::intersect_ray_mesh(ray, mesh);
if (hit) {
    fmt::print("Hit at t={}, triangle={}\n", hit->t, hit->triangle_index);
}

// AABB-AABB intersection
bool intersects = geometry::intersect_aabb_aabb(box1, box2);

// Sphere-sphere intersection
bool spheres_overlap = geometry::intersect_sphere_sphere(sphere1, sphere2);
```

## IO Integration

Import and export via the IO module:

```cpp
#include "engine/io/api.hpp"

// Import
auto result = io::import_mesh("model.obj");
if (result) {
    geometry::SurfaceMesh mesh = std::move(*result);
}

// Export
io::export_mesh(mesh, "output.obj", io::MeshFormat::OBJ);
```

See [`../io/README.md`](../io/README.md) for format support and error handling.

## Validation

Validate mesh topology and attributes:

```cpp
auto validation = geometry::validate_mesh(mesh);
if (!validation.is_valid) {
    for (const auto& error : validation.errors) {
        fmt::print("Validation error: {}\n", error.message);
    }
}
```

Validation checks:
- Index buffer bounds (no out-of-range vertex references)
- Consistent attribute counts (positions, normals, UVs match)
- Non-degenerate triangles (area > epsilon)
- Manifold topology (no non-manifold edges)
- Orientation consistency (winding order)

## Telemetry

Geometry operations emit telemetry for diagnostics:

```cpp
auto telemetry = io::geometry_io_telemetry();
for (const auto& [format, metrics] : telemetry.formats) {
    fmt::print("{}: {} imports, {} failures\n",
        format, metrics.import_count, metrics.import_failures);
}
```

## Performance Benchmarks

From `GE-205` benchmarking task:
- Normal recomputation: ~1.2ms for 50k vertices
- Bounds computation: ~0.3ms for 50k vertices
- KD-tree construction: ~15ms for 100k points
- Octree construction: ~8ms for 100k points
- LBS deformation: ~0.8ms for 1000 vertices with 20 joints

## Testing

Tests cover:
- Mesh construction and validation (`test_mesh.cpp`)
- Spatial structure queries (`test_kdtree.cpp`, `test_octree.cpp`)
- Deformation correctness (`test_deformation.cpp`)
- Procedural primitive generation (`test_shapes.cpp`)
- Topology operations (`test_topology.cpp`)

Run tests:
```bash
ctest --preset clang-debug -R geometry
```

## Dependencies

- **Math**: Vector, matrix, and quaternion types
- **IO** (integration): Format handlers for import/export
- **Animation** (integration): Rig binding for deformation
- **Runtime** (integration): Telemetry and diagnostics

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones and planned features
- [`../../design/GE-212-REMESHING_PARAMETERIZATION_RFP.md`](../../design/GE-212-REMESHING_PARAMETERIZATION_RFP.md): Remeshing proposal
- [`../../specs/ADR-0005-geometry-io-roundtrip.md`](../../specs/ADR-0005-geometry-io-roundtrip.md): IO architecture decisions
- [`../../specs/ADR-0006-animation-deformation.md`](../../specs/ADR-0006-animation-deformation.md): Deformation pipeline design
- [`../../tasks/T-0112-geometry-io-roundtrip-hardening.md`](../../tasks/T-0112-geometry-io-roundtrip-hardening.md): IO hardening milestone


