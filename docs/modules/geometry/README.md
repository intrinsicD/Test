# Geometry Module

## Overview

The geometry module provides geometric data structures (meshes, point clouds, graphs), spatial acceleration structures (kd-trees, octrees), deformation algorithms, procedural primitives, and IO helpers.

## Core Data Structures

### Surface Meshes

`SurfaceMesh` represents an indexed triangle mesh with positions, optional normals, optional texture
coordinates, and a cached AABB:

```cpp
#include "engine/geometry/api.hpp"

engine::geometry::SurfaceMesh mesh;
mesh.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
mesh.indices = {0, 1, 2};
mesh.texture_coordinates = {{0, 0}, {1, 0}, {0, 1}};

// Compute derived data explicitly
engine::geometry::recompute_vertex_normals(mesh);
engine::geometry::update_bounds(mesh);
const auto c = engine::geometry::centroid(mesh);
```

- Notes:
  - Call `recompute_vertex_normals(mesh)` after you change positions.
  - Call `update_bounds(mesh)` after you change positions or topology.
  - Texture coordinates (if present) are preserved when converting between `SurfaceMesh` and the
    halfedge representation; mismatched counts are rejected early.

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
The intersection library now carries symmetry regression tests across all AABB pairings (cylinder, ellipsoid, OBB, plane, sphere,
triangle) plus the existing interval parity checks for line/ray/segment queries, guaranteeing that overload order never changes
classification results (`T-0129`).

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

### Surface Topology Summary

Use `AnalyzeSurfaceTopology` to extract boundary and crease annotations ahead of remeshing or atlas generation workflows:

```cpp
#include "engine/geometry/topology/surface_topology.hpp"

engine::geometry::SurfaceMesh mesh = engine::geometry::load_surface_mesh("open_quad.obj");
engine::geometry::SurfaceTopologySummary summary =
    engine::geometry::AnalyzeSurfaceTopology(mesh, engine::math::radians(30.0F));

for (const auto& edge : summary.edges)
{
    if (edge.is_boundary || edge.is_crease)
    {
        // Preserve boundary edges or sharp features during remeshing
    }
}

for (std::size_t vertex = 0; vertex < summary.vertices.size(); ++vertex)
{
    if (summary.vertices[vertex].is_boundary)
    {
        // Mark boundary vertices to keep UV seams stable
    }
}
```

`SurfaceTopologySummary` reports per-edge dihedral angles, boundary classification, and non-manifold flags while also tagging
vertices that sit on boundaries or detected features. These annotations form the foundation for the `GE-221+` remeshing
milestones by supplying deterministic feature classification without requiring the halfedge representation at call sites.

## Geometric Computations

### Normals, Bounds, Centroid

```cpp
engine::geometry::recompute_vertex_normals(mesh);
engine::geometry::update_bounds(mesh);
auto center = engine::geometry::centroid(mesh);
```

### Frustum Culling

Extract view frustums from projection matrices and perform efficient intersection tests:

```cpp
#include "engine/geometry/shapes/frustum.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"

// Extract frustum from view-projection matrix
engine::geometry::Frustum frustum = engine::geometry::ExtractFrustum(view_projection_matrix);

// Test intersection with bounding volumes
bool visible = engine::geometry::Intersects(frustum, object_aabb);
bool sphere_visible = engine::geometry::Intersects(frustum, bounding_sphere);

// Get frustum corner points
auto corners = engine::geometry::GetCorners(frustum);
```

The frustum is defined by 6 planes (left, right, bottom, top, near, far) with normals pointing inward. Intersection tests use optimized algorithms:
- **Frustum-AABB**: p-vertex/n-vertex test for early rejection
- **Frustum-Sphere**: signed distance to all planes vs. radius
- **Frustum-Point**: containment test against all planes

### Remeshing Requests (GE-221+)

The Phase 0 scaffolding for the remeshing roadmap introduces validated request
and policy structures so upcoming kernels can share consistent configuration
parsing and error handling:

```cpp
#include "engine/geometry/remesh/remesh.hpp"

SurfaceMesh mesh = /* populated elsewhere */;

engine::geometry::RemeshRequest request{};
request.input_mesh = &mesh;
request.mode = engine::geometry::RemeshingMode::kFeaturePreserving;
request.targets.target_edge_length = 0.05f;
request.parameterization.mode = engine::geometry::ParameterizationMode::kGenerateLscm;
request.parameterization.target_texel_density = 512.0f;
request.feature_preservation.lock_boundary_edges = true;
request.feature_preservation.lock_feature_edges = true;
request.feature_preservation.minimum_feature_angle_degrees = 30.0f;

const engine::geometry::RemeshValidationResult validation =
    engine::geometry::ValidateRemeshRequest(request);
if (!validation.has_value())
{
    const auto& error = validation.error();
    // Surface useful diagnostics before executing expensive kernels.
    // log_warning("Remesh request rejected: {}", error.message());
}

const auto resolved_targets = engine::geometry::ResolveRemeshingTargets(request);
if (resolved_targets.has_value())
{
    const auto& targets = resolved_targets.value();
    const float target_length = targets.target_edge_length.value();
    const float mean_length = targets.edge_statistics.mean_edge_length();
    // Feed derived targets into remeshing kernels and telemetry reporting.
}

const auto remesh_result = engine::geometry::Remesh(request);
if (remesh_result.has_value())
{
    const engine::geometry::RemeshOutput& output = remesh_result.value();
    // Feature-preserving remeshing refines the mesh while keeping crease and
    // boundary vertices locked. Tangential smoothing keeps unlocked vertices on
    // the surface without eroding sharp features.
    const auto iterations = output.statistics.iteration_count;
    const auto max_edge_length = output.statistics.max_edge_length;
    // Consume the statistics for telemetry or diagnostics reporting.
    const auto texel_density = output.parameterization.texel_density;
}
```

`RemeshRequest` captures edge-length targets, feature preservation options,
attribute transfer policies, and parameterisation preferences while
`ValidateRemeshRequest` enforces the invariants published in the
[`GE-212` remeshing RFP](../../design/GE-212-REMESHING_PARAMETERIZATION_RFP.md).
Uniform and feature-preserving remeshing kernels consume these structures
directly, returning a new `SurfaceMesh` alongside iteration counts and edge
statistics in `RemeshOutput`. The adaptive remeshing mode extends this pipeline
with curvature- and surface-error budgets: it derives absolute edge-length
thresholds when callers only supply Hausdorff/normal tolerances and executes the
same split/collapse loops while respecting feature locks and tangential
smoothing.

`RemeshStatistics::max_error` records the maximum absolute deviation between the
resolved target edge length (or the derived adaptive baseline when no explicit
target is provided) and the shortest/longest edges observed in the output mesh,
giving remeshing telemetry a single scalar that quantifies how far the result
strays from the requested budget.

When parameterisation reuse is requested, remeshing now interpolates texture
coordinates for generated vertices, averages UVs across collapses, and scales
them to honour the `target_texel_density` budget. `ParameterizationSummary`
tracks the resulting texel density and stretch so telemetry surfaces reflect
changes to the UV footprint alongside geometric error metrics.

`ParameterizationSummary::chart_count` reports the number of UV islands detected
in the generated atlas by counting connected components in the parameterisation
graph, ensuring telemetry and diagnostics reflect multi-chart assets instead of
assuming a single chart per mesh.

`ParameterizationMode::kGenerateLscm` builds a least-squares conformal map for the
output mesh. The implementation automatically selects a pair of anchor vertices
(preferring boundary loops), solves the complex-valued system with partial
pivoting, and normalises the result to the requested texel density.
Degenerate meshes fall back to a deterministic planar projection so tools always
receive a usable UV atlas, and the `ParameterizationSummary` mirrors the
generated coordinates for telemetry and diagnostics consumers.

`ParameterizationMode::kGenerateAbfpp` executes an angle-based flattening (ABF++)
solver. It constructs a constrained least-squares system over per-corner angles,
enforces triangle and vertex angle sums, and then reconstructs texture
coordinates by traversing the mesh, intersecting the induced edge-length
cylinders for each adjacent face. The solver emits a single chart, honours the
requested texel density, and deterministically falls back to the planar
projection when inputs are degenerate so downstream tooling retains a usable
atlas.

Use `ComputeMeshEdgeStatistics` when you need aggregate edge metrics for telemetry
or adaptive error budgets. `ResolveRemeshingTargets` consumes a validated request
and produces consistent absolute edge-length targets even when callers specify only
a relative scale. Uniform remeshing currently supports triangle meshes, and the
feature-preserving mode extends it with crease-aware edge protection and
tangential smoothing so downstream steps can assume landmarks remain stable.

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

See `engine/geometry/benchmarks/` for current microbenchmarks (e.g., normal recomputation and frustum culling tests).

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
- Run the frustum culling benchmark:
  - `ctest --preset linux-gcc-debug -R geometry_frustum_benchmark`
- Explore examples under `engine/geometry/tests/` and benchmarks under `engine/geometry/benchmarks/`.

## TODO / Next Steps

- Track remeshing and parameterization work per `GE-221+` planning (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Extend decimation and UV parameterization utilities aligned to the published RFPs (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Expand telemetry coverage for spatial queries and IO round-trips.
