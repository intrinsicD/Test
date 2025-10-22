# Geometry Module

## Current State
- Offers `SurfaceMesh` utilities (normals, bounds, centroid), halfedge
  conversions, procedural primitives, ASCII import/export, and CPU linear blend
  skinning deformers that consume animation rig bindings.
- Provides kd-tree and octree spatial acceleration structures for geometry and
  point-cloud queries, instrumented with spatial query telemetry that reports
  invocation counts and result distributions for diagnostics consumers.【F:engine/geometry/include/engine/geometry/telemetry.hpp†L1-L78】【F:engine/geometry/include/engine/geometry/octree/octree.hpp†L137-L420】
- Exposes helpers for mesh/point-cloud interchange with other subsystems.
- Ships `geometry_normals_benchmark` +
  `geometry_normals_benchmark_report.py` to capture
  `geometry::recompute_vertex_normals` throughput for roadmap item
  `GE-205`/`TI-002`, enabling deterministic CI tracking of future optimisation
  work.【F:engine/geometry/benchmarks/normal_recompute_benchmark.cpp†L18-L205】【F:scripts/diagnostics/geometry_normals_benchmark_report.py†L1-L137】

### Deformation Helpers

- `geometry::deform::apply_linear_blend_skinning` updates mesh positions and
  normals in-place using precomputed skinning transforms, falling back to rest
  positions when no valid binding data is available.【F:engine/geometry/src/deform/linear_blend_skinning.cpp†L16-L73】
- The helper expects the supplied `RigBinding` vertex count to match
  `SurfaceMesh::rest_positions`; runtime validation will reject inconsistent
  inputs before deformation to protect downstream systems.【F:engine/runtime/src/api.cpp†L85-L134】【F:engine/runtime/src/api.cpp†L1149-L1206】
- Recompute bounds and normals are handled internally, keeping geometry in sync
  for rendering and physics sampling without additional callers.
- Pair the helper with animation skinning utilities described in the animation
  module README to guarantee that inverse bind matrices and weight normalisation
  constraints are satisfied before deformation.【F:engine/animation/src/deformation/linear_blend_skinning.cpp†L9-L96】

## Usage
- Build via `cmake --build --preset <preset> --target engine_geometry`.
- Include `<engine/geometry/surface_mesh.hpp>` and related headers for mesh
  operations.
- Execute `ctest --preset <preset> --tests-regex engine_geometry`.

## TODO / Next Steps

- Track telemetry viewer documentation follow-up in the [central roadmap](../../ROADMAP.md) so diagnostics consumers understand the new metrics — supports `CC-001` alignment.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `GE-205` | Benchmark accelerated normal recomputation (`TI-002`). | Publish benchmark results and integrate into CI perf harness. | ✅ Done |
| `GE-212` | Draft remeshing/parameterisation RFP. | Produce design note outlining requirements and dependencies. | ✅ Done |
| `GE-220` | Align geometry telemetry with diagnostics (`CC-001`). | Add instrumentation for spatial queries and document metrics. | ✅ Done |

The remeshing/parameterisation RFP produced for `GE-212` is available under
[`docs/design/ge-212-remeshing_parameterization_rfp.md`](../../design/ge-212-remeshing_parameterization_rfp.md)
for stakeholders planning follow-on implementation work.

See [ROADMAP.md](ROADMAP.md) for full context.

### Staffing Notes

- `GE-212` is a planning/RFP effort focused on defining scope and dependencies.
- `GE-220` instrumentation is complete; coordinate with CC-001 stakeholders on viewer documentation for the new metrics.
- Assign separate agents to each task and coordinate asynchronously on schema updates to keep workstreams decoupled.

### Benchmarks

- Default debug preset (`resolution=256`, `iterations=128`) records
  `duration_seconds=7.673`, `iterations_per_second=16.68`,
  `vertices_per_second≈1.10e6`, and `triangles_per_second≈2.19e6`, with a
  checksum of `-66049` to guard determinism for regression tracking.【F:engine/geometry/benchmarks/normal_recompute_benchmark.cpp†L133-L205】
