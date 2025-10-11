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

- **Near-Term** – Finish clip/rig serialization and validation, then unlock blend-tree
  authoring so runtime clients can composite multiple motion sources. (See
  [docs/animation_roadmap.md](animation_roadmap.md).)
- **Mid-Term** – Integrate deformation paths with the geometry module and publish GPU
  pose buffers for rendering.
- **Long-Term** – Expand diagnostics, profiling, and tooling coverage to monitor large
  rigs and complex state machines.

### Physics

- **Near-Term** – Stabilise the rigid-body world (mass/force invariants, API factoring)
  and introduce instrumentation before widening scope. Detailed tasks live in
  [docs/physics/roadmap.md](physics/roadmap.md).
- **Mid-Term** – Stand up scalable collision management (broad-phase, contact manifolds,
  constraint solver) so physics can participate in richer runtime scenes.
- **Long-Term** – Advance dynamics fidelity with improved integrators, sleeping/activation
  heuristics, and an extensible collider set coordinated with geometry.

### Rendering

- **Near-Term** – Enrich frame-graph resource descriptors, propagate queue/command
  metadata, and prototype a reference GPU scheduler. Refer to
  [docs/rendering/ROADMAP.md](rendering/ROADMAP.md).
- **Mid-Term** – Ship Vulkan/DX12 resource providers and backend schedulers together with
  a library of canonical passes.
- **Long-Term** – Layer on validation, profiling, and extensive documentation/samples to
  support production use.

### Geometry

- **Near-Term** – Complete the foundational read/write and conversion paths for meshes,
  graphs, and point clouds so other subsystems can consume consistent data structures.
- **Mid-Term** – Add remeshing, parameterisation, and reconstruction algorithms that feed
  animation deformation and physics collision generation.
- **Long-Term** – Institutionalise benchmarking, profiling, and documentation to maintain
  data quality as advanced algorithms land.

### Runtime

- **Near-Term** – Replace the singleton bootstrap with an explicit `RuntimeHost`, expand
  dispatcher flexibility, and harden scene mirroring as described in
  [docs/design/runtime_plan.md](design/runtime_plan.md).
- **Mid-Term** – Integrate asynchronous asset streaming and render scheduling hooks so the
  runtime can orchestrate full-frame workloads.
- **Long-Term** – Move onto the global job system, support deterministic replay, and expose
  hot-reloadable configuration for live tuning.

### Tooling

- **Near-Term** – Establish shared tooling infrastructure (common utilities, Dear ImGui
  backends) and prototype asset pipeline automation.
- **Mid-Term** – Build profiling surfaces and editor shells that leverage runtime/rendering
  telemetry.
- **Long-Term** – Package the tooling ecosystem with distribution, documentation, and
  diagnostics suitable for production teams.

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
