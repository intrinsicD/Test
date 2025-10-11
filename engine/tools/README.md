# Tools Module

## Current State

- Houses editor, pipeline automation, and profiling utilities that sit atop the runtime.
- Presently acts as scaffolding for future tooling features.

## Vision

- Provide first-class authoring and diagnostics experiences that shorten the iteration loop between content, runtime systems, and shipping builds.
- Keep tooling optional but well-integrated so headless builds or dedicated servers can exclude them without forking the codebase.

## Constraints & Dependencies

- Depends on the runtime, scene, rendering, and asset subsystems to expose data for inspection and modification.
- Shares platform requirements with the runtime (windowing, input, filesystem). Editor and profiling fronts should reuse common abstractions.
- Content pipelines must align with asset import/export formats defined under `engine/io` and asset schemas maintained in `engine/assets`.

## Roadmap

The phase breakdown below participates in the shared priorities outlined in
[docs/global_roadmap.md](../../docs/global_roadmap.md).

| Phase | Focus | Key Tasks | Dependencies |
| --- | --- | --- | --- |
| **Phase 0: Foundations** | Establish build targets and shared infrastructure | <ul><li>Create `engine_tools_common` utility library for shared UI, logging, and service wiring.</li><li>Define configuration schema for tooling (hot-reloadable JSON/TOML).</li><li>Integrate Dear ImGui/ImPlot backends as optional third-party dependencies.</li></ul> | Requires stable runtime service locator and platform windowing hooks. |
| **Phase 1: Content Pipelines** | Automate asset ingestion | <ul><li>Implement import graph orchestrator invoking `engine/io` translators.</li><li>Add dependency tracking and cache invalidation for derived assets.</li><li>Provide CLI tooling for batch conversions with progress reporting.</li></ul> | `engine/io`, `engine/assets`, filesystem providers. |
| **Phase 2: Profiling Suite** | Instrument frame and job execution | <ul><li>Expose profiling markers from runtime, rendering, and compute subsystems.</li><li>Persist captured traces (JSON/Chrome trace format) and integrate live viewers.</li><li>Add comparative benchmarking harness tied into `ctest` performance presets.</li></ul> | Runtime scheduler, job system metrics, serialization helpers. |
| **Phase 3: Editor Shell** | Real-time authoring environment | <ul><li>Stand up dockable UI with scene hierarchy, inspector, and asset browser panels.</li><li>Enable gizmo-based transforms and undo/redo stack.</li><li>Hook editor state into runtime hot-reload channels for materials, scripts, and scenes.</li></ul> | Scene graph, rendering viewport embedding, scripting interfaces. |
| **Phase 4: Distribution** | Harden for production use | <ul><li>Package tooling binaries with plugin discovery and update channels.</li><li>Document workflows and provide sample projects with reproducible pipelines.</li><li>Integrate telemetry/diagnostics opt-in for live issue triage.</li></ul> | Build system packaging scripts, documentation infrastructure. |

## Immediate Next Steps

1. Draft CMake targets for shared tooling utilities and ensure optional compilation via feature flags.
2. Survey existing runtime and platform services to identify gaps needed for editor/profiling integration (event buses, hot-reload, metrics export).
3. Prototype asset pipeline CLI covering import, cook, and package steps; capture requirements for caching and dependency tracking.
4. Define profiling data schema (sampling frequency, capture formats) aligned with runtime instrumentation.
5. Produce UI wireframes and interaction flows for the first editor milestone; validate feasibility with rendering subsystem leads.

## Documentation & Tracking

- Update this roadmap alongside progress to keep the aggregate backlog in `README.md` synchronised.
- Maintain per-submodule notes (`editor/README.md`, `pipelines/README.md`, `profiling/README.md`) describing detailed milestones and integration requirements.
- Capture design decisions in `docs/design/tools/` (to be created) once implementation commences.
