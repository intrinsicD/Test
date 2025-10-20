# Geometry Module

## Current State
- Offers `SurfaceMesh` utilities (normals, bounds, centroid), halfedge
  conversions, procedural primitives, and ASCII import/export.
- Provides kd-tree and octree spatial acceleration structures for geometry and
  point-cloud queries.
- Exposes helpers for mesh/point-cloud interchange with other subsystems.

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
