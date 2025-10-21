# Geometry Module

## Current State
- Offers `SurfaceMesh` utilities (normals, bounds, centroid), halfedge
  conversions, procedural primitives, ASCII import/export, and CPU linear blend
  skinning deformers that consume animation rig bindings.
- Provides kd-tree and octree spatial acceleration structures for geometry and
  point-cloud queries.
- Exposes helpers for mesh/point-cloud interchange with other subsystems.

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

- Track `GE-205`, `GE-212`, `GE-220` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — supports `TI-002` benchmarking.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `GE-205` | Benchmark accelerated normal recomputation (`TI-002`). | Publish benchmark results and integrate into CI perf harness. | 🔄 In Progress |
| `GE-212` | Draft remeshing/parameterisation RFP. | Produce design note outlining requirements and dependencies. | 🟢 Todo |
| `GE-220` | Align geometry telemetry with diagnostics (`CC-001`). | Add instrumentation for spatial queries and document metrics. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for full context.

### Staffing Notes

- `GE-212` is a planning/RFP effort focused on defining scope and dependencies.
- `GE-220` instruments telemetry pathways on top of the established diagnostics schema.
- Assign separate agents to each task and coordinate asynchronously on schema updates to keep workstreams decoupled.
