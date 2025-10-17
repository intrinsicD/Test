# Architecture Overview

This document captures the stable truths about the engine. Treat it as the authoritative map of module responsibilities, control flow, and invariants.

## Subsystem Map

- **Core & Runtime:** `engine/core` wraps the EnTT registry and discovery utilities. `engine/runtime` orchestrates animation, physics, geometry updates, and rendering submission through `RuntimeHost`.
- **Animation:** Clip sampling, validation, and controller/blend-tree evaluation feed pose data into the runtime. Outputs drive deformation systems downstream.
- **Physics:** A rigid-body world with sweep-and-prune broad phase, collider primitives (sphere, capsule, AABB), and configurable substepping. Physics publishes transforms and contact events back into the runtime.
- **Geometry & IO:** Geometry owns mesh/point-cloud/graph types plus spatial indices (kd-tree, octree). IO handles import/export pipelines, with ASCII formats as the current baseline. Geometry services expose bounds, centroid, and procedural primitive helpers.
- **Rendering:** Frame-graph compilation and execution, command encoder hooks, and GPU resource lifetime tracking. Backends (Vulkan, DirectX, OpenGL) plug into a scheduler abstraction.
- **Compute:** Dispatch infrastructure for CPU/GPU kernels, CUDA interop, and math helpers. The runtime consumes these services for simulation and deformation workloads.
- **Assets, Math, Scene, Tools:** Asset caches with hot reload, math primitives, scene graph serialization, and tooling scaffolding (Dear ImGui) round out the workspace.

## Data & Control Flow

1. **Asset ingestion:** IO imports resources into asset caches; geometry validates topology and generates bounds.
2. **Simulation:** Runtime advances animation controllers, publishes pose data, and drives physics simulation via compute kernels.
3. **Geometry deformation:** Animation deforms meshes and updates spatial acceleration structures maintained by geometry.
4. **Rendering submission:** Runtime packages frame-graph jobs and hands them to the rendering scheduler, which selects the backend.
5. **Diagnostics & tooling:** Tools module layers Dear ImGui and profiling utilities on top of the runtime loop.

## Invariants

- **Deterministic Scheduler:** Frame-graph compilation must be deterministic for identical inputs. Backends may add validation, but they cannot reorder resource transitions.
- **Resource Ownership:** Assets expose handles via `engine::headers`. Lifetime is reference-counted; releasing a handle must free GPU/CPU resources deterministically.
- **Geometry Fidelity:** Spatial structures (kd-tree, octree) must stay in sync with mesh/point-cloud mutations. All geometry changes update bounds and centroid data before publishing to other systems.
- **Physics Integration:** The physics world clamps mass/damping and maintains monotonic substep progression. Sweeps respect branchless hot loops to avoid perf regressions.
- **Documentation Discipline:** Module READMEs are canonical for local behaviour. Architectural shifts must also update this document and the relevant ADR in [`docs/specs/`](specs/).

## Related Documents

- [`../README.md`](../README.md) – module snapshot, architecture improvement plan summary, and build/test workflow.
- [`ROADMAP.md`](ROADMAP.md) – authoritative backlog with `DC-`, `AI-`, and `RT-` identifiers referenced throughout this file.
- [`docs/specs/ADR-0003-runtime-frame-graph.md`](specs/ADR-0003-runtime-frame-graph.md) – scheduler contracts and metadata expectations.
- [`docs/specs/ADR-0005-geometry-io-roundtrip.md`](specs/ADR-0005-geometry-io-roundtrip.md) – geometry and IO ownership, file formats, and validation.
- [`docs/tasks/2025-02-17-sprint-06.md`](tasks/2025-02-17-sprint-06.md) – current sprint focus tying runtime and rendering milestones together.

Keep this file short, precise, and free of tentative language. If a claim here stops being true, fix it immediately and reference the change in the affected ADR or task record.
