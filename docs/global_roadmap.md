# Global Roadmap Overview

This document aligns the major engine subsystems around a shared sequencing plan. The
per-module roadmaps linked below remain authoritative for local details; the goal here
is to surface cross-cutting priorities and dependencies so work streams progress in
concert.

## Milestone Bands

- **Near-Term (1–2 milestones)** – Close the most painful feature gaps to unblock
  multi-module integration and smoke-test scenarios.
- **Mid-Term (3–5 milestones)** – Broaden functionality and harden subsystems for
  sustained iteration, including cross-module data flows and tooling.
- **Long-Term (5+ milestones)** – Deliver robustness, authoring experiences, and
  extensibility required for production workloads.

## Subsystem Alignment

### Animation

- **Near-Term** – Clip validation, JSON serialization, deterministic clip evaluation, and
  linear blend-tree nodes ship today. Next steps focus on deformation bindings and
  parameter-driven blending per [animation_roadmap.md](animation_roadmap.md).
- **Mid-Term** – Share pose buffers with geometry and rendering while introducing richer
  blend-node types and controller authoring utilities.
- **Long-Term** – Expand diagnostics, profiling, and tooling coverage to monitor large rigs
  and complex state machines.

### Assets

- **Near-Term** – Formalise asset descriptors, improve hot-reload diagnostics, and align
  cache metadata with IO importers so pipelines can reason about provenance.
- **Mid-Term** – Stand up staged import graphs that feed runtime caches and persist cooked
  artefacts for reuse across runs.
- **Long-Term** – Automate validation with representative sample sets and integrate cache
  residency policies into tooling workflows.

### Compute

- **Near-Term** – Unify dispatch descriptions so CPU and GPU executors can share the kernel
  scheduler and timing instrumentation.
- **Mid-Term** – Introduce the CUDA-backed executor, surface device/stream management hooks,
  and expose mixed CPU/GPU regression scenarios.
- **Long-Term** – Integrate with the runtime job system and extend profiling/telemetry so
  heterogeneous workloads remain observable.

### Core

- **Near-Term** – Define the application lifecycle, configuration, and diagnostics services
  that the runtime will host, leveraging the existing EnTT façade.
- **Mid-Term** – Implement plugin discovery/loading with deterministic teardown and expand
  configuration layering (defaults, file overrides, command line).
- **Long-Term** – Harden coverage and author examples that demonstrate how downstream modules
  consume the shared services.

### Geometry & IO

- **Near-Term** – Complete the foundational read/write paths for meshes, point clouds, and
  graphs while keeping property registries and spatial structures aligned with runtime
  needs.
- **Mid-Term** – Add remeshing, parameterisation, and reconstruction algorithms that feed
  deformation and collision pipelines, alongside richer detection diagnostics.
- **Long-Term** – Institutionalise benchmarking, profiling, and documentation to maintain
  data quality as advanced algorithms land.

### Math

- **Near-Term** – Catalogue the current primitives, document invariants, and broaden unit
  tests around existing helpers (reflection, inversion, decomposition).
- **Mid-Term** – Introduce fixed-size factorisation routines (QR/SVD/Cholesky) and SIMD
  specialisations required by animation/physics.
- **Long-Term** – Expand interoperability layers with geometry/physics and expose conversion
  utilities to the Python bindings.

### Physics

- **Near-Term** – The rigid-body world exposes damping, substepping, sweep-and-prune
  broad-phase pruning, and capsule/sphere/AABB colliders. The priority now is contact
  manifold generation, constraint solving, and instrumentation as captured in
  [physics/roadmap.md](physics/roadmap.md).
- **Mid-Term** – Scale collision management with persistent manifolds and constraint
  solvers so physics can drive richer runtime scenes.
- **Long-Term** – Advance dynamics fidelity with improved integrators, sleeping/activation
  heuristics, and an extensible collider set coordinated with geometry.

### Rendering

- **Near-Term** – Extend frame-graph resource descriptors, propagate queue/command metadata,
  and prototype a reference GPU scheduler according to
  [rendering/ROADMAP.md](rendering/ROADMAP.md).
- **Mid-Term** – Provide backend resource providers (Vulkan/DX12) and schedulers along with a
  baseline library of passes.
- **Long-Term** – Layer on validation, profiling, and extensive documentation/samples to
  support production use.

### Runtime

- **Near-Term** – `RuntimeHost` already orchestrates animation → physics → geometry via the
  compute dispatcher. Harden lifecycle diagnostics, scene mirroring, and streaming hooks as
  outlined in [design/runtime_plan.md](design/runtime_plan.md).
- **Mid-Term** – Integrate asynchronous asset streaming and render submission so the runtime
  can coordinate full-frame workloads.
- **Long-Term** – Move onto the global job system, support deterministic replay, and expose
  hot-reloadable configuration for live tuning.

### Scene

- **Near-Term** – Define schemas for first-class runtime components (lights, cameras,
  visibility volumes) and extend traversal helpers beyond raw registry views.
- **Mid-Term** – Broaden serialization to cover the new component families, add versioning,
  and ensure forward/backward compatibility tests land alongside loaders.
- **Long-Term** – Maintain profiling scenarios and authoring tooling that stress lifecycle,
  hierarchy manipulation, and serialization throughput.

### Platform

- **Near-Term** – Replace mock window/input providers with real GLFW/SDL integrations and
  extend the virtual filesystem with write/watch utilities to unblock hot reload.
- **Mid-Term** – Harden backend selection, expose advanced window features (high DPI,
  fullscreen, swapchain surfaces), and wire real input sampling into the runtime.
- **Long-Term** – Deliver robust diagnostics, configuration toggles, and automated coverage
  for cross-platform deployments.

### Tools

- **Near-Term** – Stand up shared tooling infrastructure (common utilities, Dear ImGui
  backends) and prototype the asset pipeline CLI.
- **Mid-Term** – Integrate profiling data capture and live viewers, then iterate on editor
  shells with dockable panels and hot-reload hooks.
- **Long-Term** – Package distribution-ready tooling with documentation, telemetry options,
  and reproducible sample projects.

## Cross-Cutting Dependencies

1. **Data Contracts** – Animation, physics, and geometry must converge on shared mesh,
   rig, and collider representations before runtime orchestration expands. Geometry's
   foundational work is a prerequisite for advanced animation deformation and physics
   manifolds.
2. **Scheduling Infrastructure** – Rendering's scheduler prototype and compute's unified
   dispatch abstractions inform the runtime's migration to a job-graph-driven frame loop.
3. **Asset Pipelines** – The IO and asset modules must deliver reliable import/cache flows
   before tooling and runtime streaming can stabilise.
4. **Diagnostics** – Profiling hooks introduced in physics, rendering, and runtime feed the
   tooling roadmap and should share consistent telemetry schemas.

Keep the aggregated backlog in the root `README.md` synchronised with this alignment table
whenever module roadmaps evolve.
