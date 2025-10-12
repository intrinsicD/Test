# Central Roadmap

This roadmap aggregates the actionable plans defined by each engine module and highlights cross-cutting initiatives. Consult the per-module documents for detailed task descriptions.

## Module Roadmaps

- [Animation](modules/animation/ROADMAP.md)
- [Assets](modules/assets/ROADMAP.md)
- [Compute](modules/compute/ROADMAP.md)
- [Core](modules/core/ROADMAP.md)
- [Geometry](modules/geometry/ROADMAP.md)
- [IO](modules/io/ROADMAP.md)
- [Math](modules/math/ROADMAP.md)
- [Physics](modules/physics/ROADMAP.md)
- [Platform](modules/platform/ROADMAP.md)
- [Rendering](modules/rendering/ROADMAP.md)
- [Runtime](modules/runtime/ROADMAP.md)
- [Scene](modules/scene/ROADMAP.md)
- [Tools](modules/tools/ROADMAP.md)

## Milestone Bands

- **Near-Term (1–2 milestones)** – Close the most painful feature gaps to unblock multi-module integration and smoke-test scenarios.
- **Mid-Term (3–5 milestones)** – Broaden functionality and harden subsystems for sustained iteration, including cross-module data flows and tooling.
- **Long-Term (5+ milestones)** – Deliver robustness, authoring experiences, and extensibility required for production workloads.

## Milestone Spotlight – First Rendering Frame

### Objective

Deliver the first on-screen frame produced by the engine by aligning the rendering and runtime roadmaps
around a tightly scoped integration slice. The milestone emphasises the minimum vertical path capable of
submitting a renderable scene graph from the runtime into the rendering frame graph and replaying it through
a backend-neutral scheduler.

### Selected TODOs

1. **Enrich Frame-Graph Resource Descriptors**
   - **Source backlog:** `engine/rendering/README.md` short-term roadmap.
   - **Description:** Extend `FrameGraphResourceInfo` and related creation APIs with explicit format,
     dimension, and usage metadata so backend resource providers can allocate deterministically and passes
     publish the constraints required for validation.
   - **Deliverables:** Schema updates across the rendering resources, a migration guide documenting the new
     fields and defaults, and unit coverage asserting the metadata appears in compiled graphs.
   - **Dependencies:** Coordinate schema alignment with the assets module so material and shader descriptors
     carry matching requirements.

2. **Prototype the Reference GPU Scheduler**
   - **Source backlog:** Rendering roadmap short-term goals.
   - **Description:** Implement a backend-neutral scheduler that converts compiled frame-graph passes into a
     linear submission stream targeting an abstract command encoder. Validate dependency resolution,
     transient lifetime management, and queue metadata propagation before platform backends exist.
   - **Deliverables:** Reference scheduler implementation with stub encoder hooks and logging, integration
     tests covering multi-pass graphs, and diagnostics surfaced through the logging infrastructure to
     visualise submission order.
   - **Dependencies:** Builds directly on the enriched resource descriptors and exposes the data needed by
     runtime scheduling hooks.

3. **Introduce Runtime Render Submission Hooks**
   - **Source backlog:** Runtime roadmap mid-term goals.
   - **Description:** Extend `RuntimeHost` with a render submission interface (e.g., `RenderGraphBuilder`)
     that packages the current scene snapshot, camera parameters, and resource requirements into a form the
     rendering scheduler consumes. The runtime owns orchestration of visibility queries and pass registration
     for the initial slice.
   - **Deliverables:** Runtime façade or adapter that marshals scene data into renderable structures, a smoke
     test exercising animation → physics → runtime submission → rendering scheduler, and documentation that
     illustrates the lifecycle from runtime tick to render submission.
   - **Dependencies:** Requires TODOs 1 and 2 to define the data contracts accepted by the rendering
     scheduler.

### Acceptance Criteria

- A single example application can tick the runtime, build a frame graph, and submit it through the
  reference GPU scheduler without backend-specific code.
- CI includes regression coverage for resource descriptor enrichment, scheduler submission ordering, and
  runtime-to-rendering handoff.
- Documentation across runtime and rendering READMEs references this milestone to orient future
  contributors.

## Subsystem Alignment

### Animation

- **Completed (M1–M2):**
  - ✅ Clip validation, JSON serialization
  - ✅ Linear blend trees with parameter binding
  - ✅ Float, bool, and event parameters
- **Near-Term (M3):**
  - Additive blend nodes for pose composition
  - Deformation binding data structures
- **Mid-Term (M4–M5):**
  - Linear and dual quaternion skinning
  - State machine nodes
  - Editor authoring tools

### Assets

- **Near-Term** – Formalise asset descriptors, improve hot-reload diagnostics, and align cache metadata with IO importers so pipelines can reason about provenance.
- **Mid-Term** – Stand up staged import graphs that feed runtime caches and persist cooked artefacts for reuse across runs.
- **Long-Term** – Automate validation with representative sample sets and integrate cache residency policies into tooling workflows.

### Compute

- **Near-Term** – Unify dispatch descriptions so CPU and GPU executors can share the kernel scheduler and timing instrumentation.
- **Mid-Term** – Introduce the CUDA-backed executor, surface device/stream management hooks, and expose mixed CPU/GPU regression scenarios.
- **Long-Term** – Integrate with the runtime job system and extend profiling/telemetry so heterogeneous workloads remain observable.

### Core

- **Near-Term** – Define the application lifecycle, configuration, and diagnostics services that the runtime will host, leveraging the existing EnTT façade.
- **Mid-Term** – Implement plugin discovery/loading with deterministic teardown and expand configuration layering (defaults, file overrides, command line).
- **Long-Term** – Harden coverage and author examples that demonstrate how downstream modules consume the shared services.

### Geometry & IO

- **Near-Term** – Complete the foundational read/write paths for meshes, point clouds, and graphs while keeping property registries and spatial structures aligned with runtime needs.
- **Mid-Term** – Add remeshing, parameterisation, and reconstruction algorithms that feed deformation and collision pipelines, alongside richer detection diagnostics.
- **Long-Term** – Institutionalise benchmarking, profiling, and documentation to maintain data quality as advanced algorithms land.

### Math

- **Near-Term** – Catalogue the current primitives, document invariants, and broaden unit tests around existing helpers (reflection, inversion, decomposition).
- **Mid-Term** – Introduce fixed-size factorisation routines (QR/SVD/Cholesky) and SIMD specialisations required by animation/physics.
- **Long-Term** – Expand interoperability layers with geometry/physics and expose conversion utilities to the Python bindings.

### Physics

- **Near-Term** – The rigid-body world exposes damping, substepping, sweep-and-prune broad-phase pruning, and capsule/sphere/AABB colliders. The priority now is contact manifold generation, constraint solving, and instrumentation as captured in [modules/physics/ROADMAP.md](modules/physics/ROADMAP.md).
- **Mid-Term** – Scale collision management with persistent manifolds and constraint solvers so physics can drive richer runtime scenes.
- **Long-Term** – Advance dynamics fidelity with improved integrators, sleeping/activation heuristics, and an extensible collider set coordinated with geometry.

### Rendering

- **Near-Term** – Extend frame-graph resource descriptors, propagate queue/command metadata, and prototype a reference GPU scheduler according to [modules/rendering/ROADMAP.md](modules/rendering/ROADMAP.md).
- **Mid-Term** – Provide backend resource providers (Vulkan/DX12) and schedulers along with a baseline library of passes.
- **Long-Term** – Layer on validation, profiling, and extensive documentation/samples to support production use.

### Runtime

- **Near-Term** – `RuntimeHost` already orchestrates animation → physics → geometry via the compute dispatcher. Harden lifecycle diagnostics, scene mirroring, and streaming hooks as detailed in the [Runtime Expansion Plan](#runtime-expansion-plan).
- **Mid-Term** – Integrate asynchronous asset streaming and render submission so the runtime can coordinate full-frame workloads.
- **Long-Term** – Move onto the global job system, support deterministic replay, and expose hot-reloadable configuration for live tuning.

#### Runtime Expansion Plan

- **Current Observations**
  - `RuntimeHost` replaces the ad-hoc singleton while advancing animation, physics, geometry, and scene mirroring through the fixed kernel chain (`animation.evaluate → physics.accumulate → physics.integrate → geometry.deform → geometry.finalize`).
  - Initialization builds a toy physics world, generates a linear animation clip, and mirrors joint transforms into an EnTT-backed scene registry accessible through C++ and C front-ends.
  - Shutdown resets transient buffers but lacks subsystem teardown, diagnostics, streaming, or render scheduling.

- **Near-Term Goals (1–2 Milestones)**
  1. **Formalise lifecycle management.** Replace implicit singletons with explicit ownership, extend lifecycle diagnostics, and expand tests that exercise repeated initialize/shutdown sequences and invalid usage.
  2. **Extend frame orchestration.** Promote the dispatcher from a fixed linear chain to a frame-graph-driven scheduler, surface per-kernel timing through `runtime_frame_state`, and add tests validating topological execution and telemetry integrity.
  3. **Scene synchronisation hardening.** Support dynamic entity lifecycles when content streams in, preserve hierarchy metadata, and add regressions that verify world transforms for branching rigs.

- **Mid-Term Goals (3–5 Milestones)**
  4. **Streaming asset integration.** Interface with assets/IO for asynchronous loading, maintain double-buffered resources, and test resilience to slow streams.
  5. **Render scheduling hooks.** Define a render submission interface that packages scene snapshots and propagates bounds/camera configuration, then validate an end-to-end smoke test.
  6. **Diagnostics and tooling.** Emit structured telemetry, integrate profiling hooks, and expand runtime documentation with lifecycle diagrams and troubleshooting guidance.

- **Long-Term Goals (5+ Milestones)**
  7. **High-frequency job system integration.** Migrate the dispatcher onto the global task graph, track CPU/GPU dependencies, and gate resource usage with completion fences.
  8. **Deterministic replay and state capture.** Record authoritative simulation inputs each frame, persist snapshots, and feed tooling pipelines for offline analysis.
  9. **Hot-reloadable configuration.** Expose configuration files or scripting bindings for runtime parameters, backed by validation layers that reject incompatible changes.

- **Documentation and Tracking**
  - Update the runtime README as milestones close and mirror progress into the central roadmap and CI/backlog dashboards so dependent teams remain aligned.

### Scene

- **Near-Term** – Define schemas for first-class runtime components (lights, cameras, visibility volumes) and extend traversal helpers beyond raw registry views.
- **Mid-Term** – Broaden serialization to cover the new component families, add versioning, and ensure forward/backward compatibility tests land alongside loaders.
- **Long-Term** – Maintain profiling scenarios and authoring tooling that stress lifecycle, hierarchy manipulation, and serialization throughput.

### Platform

- **Near-Term** – Replace mock window/input providers with real GLFW/SDL integrations and extend the virtual filesystem with write/watch utilities to unblock hot reload.
- **Mid-Term** – Harden backend selection, expose advanced window features (high DPI, fullscreen, swapchain surfaces), and wire real input sampling into the runtime.
- **Long-Term** – Deliver robust diagnostics, configuration toggles, and automated coverage for cross-platform deployments.

### Tools

- **Near-Term** – Stand up shared tooling infrastructure (common utilities, Dear ImGui backends) and prototype the asset pipeline CLI.
- **Mid-Term** – Integrate profiling data capture and live viewers, then iterate on editor shells with dockable panels and hot-reload hooks.
- **Long-Term** – Package distribution-ready tooling with documentation, telemetry options, and reproducible sample projects.

## Cross-Cutting Initiatives

1. **Data Contracts** – Animation, physics, and geometry must converge on shared mesh, rig, and collider representations before runtime orchestration expands. Geometry's foundational work is a prerequisite for advanced animation deformation and physics manifolds.
2. **Scheduling Infrastructure** – Rendering's scheduler prototype and compute's unified dispatch abstractions inform the runtime's migration to a job-graph-driven frame loop.
3. **Asset Pipelines** – The IO and asset modules must deliver reliable import/cache flows before tooling and runtime streaming can stabilise.
4. **Diagnostics** – Profiling hooks introduced in physics, rendering, and runtime feed the tooling roadmap and should share consistent telemetry schemas.
