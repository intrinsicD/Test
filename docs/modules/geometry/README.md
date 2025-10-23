# Geometry Module

## Overview

The geometry module provides geometric data structures (meshes, point clouds, graphs), spatial acceleration structures (kd-trees, octrees), deformation algorithms, procedural primitives, and IO helpers.

## Core Data Structures

### Surface Meshes

`SurfaceMesh` represents an indexed triangle mesh with positions, optional normals, and a cached AABB:

```cpp
#include "engine/geometry/api.hpp"

engine::geometry::SurfaceMesh mesh;
mesh.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
mesh.indices = {0, 1, 2};

// Compute derived data explicitly
engine::geometry::recompute_vertex_normals(mesh);
engine::geometry::update_bounds(mesh);
const auto c = engine::geometry::centroid(mesh);
```

Notes:
- Call `recompute_vertex_normals(mesh)` after you change positions.
- Call `update_bounds(mesh)` after you change positions or topology.

### Point Clouds

See `engine/geometry/point_cloud/` for point cloud primitives and IO.

`PointCloudIOFlags::Format::kAuto` infers the writer format from the destination
file extension (case-insensitive) and falls back to PLY when the extension is
missing or unknown, honouring the binary flag when set. This keeps round-trip
helpers resilient in scripting pipelines that generate temporary files without
extensions.

### Graphs

Connectivity utilities live under `engine/geometry/graph/`.

## Procedural Primitives

Basic shapes and utilities live under `engine/geometry/shapes/` with pairwise intersection queries under `engine/geometry/utils/shape_interactions.hpp`.

## Spatial Acceleration

### KD-Tree

```cpp
#include "engine/geometry/kdtree/kdtree.hpp"

engine::geometry::KdTree tree(mesh.positions);
auto nearest = tree.nearest({0, 0, 0});
```

### Octree

```cpp
#include "engine/geometry/octree/octree.hpp"

engine::geometry::Octree oct(mesh.positions, /*max_depth=*/8, /*max_points_per_leaf=*/32);
auto points_in_box = oct.query(engine::geometry::MakeAabbFromCenterExtent({0,0,0}, {1,1,1}));
```

Rebuild these structures when you modify source data.

## Deformation

LBS and other deformers live under `engine/geometry/deform/` and integrate with the animation module.

## Topology Operations

Halfedge conversion and utilities live under `engine/geometry/mesh/` and `engine/geometry/topology/`.

## Geometric Computations

### Normals, Bounds, Centroid

```cpp
engine::geometry::recompute_vertex_normals(mesh);
engine::geometry::update_bounds(mesh);
auto center = engine::geometry::centroid(mesh);
```

## IO Integration

Load and save meshes via the module API:

```cpp
#include "engine/geometry/api.hpp"

engine::geometry::SurfaceMesh mesh = engine::geometry::load_surface_mesh("triangle.obj");
engine::geometry::save_surface_mesh(mesh, "out.obj");
```

## Telemetry

See diagnostics and IO modules for telemetry surfaces.

## Performance Benchmarks

See `engine/geometry/benchmarks/` for current microbenchmarks (e.g., normal recomputation).

## Testing

Run tests:
```bash
ctest --preset linux-gcc-debug -R geometry
```

## Dependencies

- Math: vector/matrix primitives
- IO (integration): import/export
- Animation (integration): deformers
- Runtime (integration): telemetry

## Related Documentation

- See `docs/design/` and `docs/specs/` for ADRs and RFPs.

## Current State

Surface mesh utilities, shapes and intersections, kd-tree and octree accelerators, IO helpers, and deformation hooks covered by unit tests.

## Usage

- Build and run geometry tests:
  - `ctest --preset linux-gcc-debug -R geometry`
- Run the normals microbenchmark:
  - `ctest --preset linux-gcc-debug -R geometry_normals_benchmark`
- Explore examples under `engine/geometry/tests/` and benchmarks under `engine/geometry/benchmarks/`.

## TODO / Next Steps

- Track remeshing and parameterization work per `GE-221+` planning (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Extend decimation and UV parameterization utilities aligned to the published RFPs (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Expand telemetry coverage for spatial queries and IO round-trips.
