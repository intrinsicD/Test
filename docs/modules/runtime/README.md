# Runtime Module

## Overview

> **Status:** ⚠️ **At Risk** — `RuntimeHost` now compiles a declarative `RuntimeLoopPlan` with per-phase telemetry and supports the `rendering::PresentationBackend` interface, yet GPU-backed presenters and synchronisation APIs mandated by [`ADR-0008`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md) remain outstanding until [`RT-410`](../../backlog/active/RT-410-runtime-stage-planner.md) lands.

The runtime module orchestrates the engine's main execution loop through `RuntimeHost`, which coordinates animation evaluation, physics simulation, geometry deformation, scene graph updates, and rendering submission. It acts as the integration point for all subsystems and provides comprehensive diagnostics and telemetry.

## Outstanding Work

- Expand the new `RuntimeLoopPlan` stage planner with presentation adapters and runtime configurability described in [`ADR-0008`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md) (`RT-410`).
- Provide synchronisation hooks for scripting, diagnostics, and tooling integrations in tandem with rendering backends.
- Validate the new loop against OpenGL/Vulkan once [`T-0120`](../../backlog/active/T-0120-gpu-resource-provider.md) and [`T-0119`](../../backlog/active/T-0119-command-encoder-integration.md) ship.

## Core Concepts

### RuntimeHost

`RuntimeHost` is the primary entry point for managing the engine's lifecycle:

```cpp
#include "engine/runtime/api.hpp"

engine::runtime::RuntimeHostDependencies deps{};
deps.controller = animation::make_linear_controller(animation::make_default_clip());
deps.mesh = geometry::make_unit_quad();
deps.world = physics::PhysicsWorld{};
deps.streaming_config.worker_count = 2;
deps.streaming_config.queue_capacity = 64;

engine::runtime::RuntimeHost host{std::move(deps)};
host.initialize();

// Main loop
while (running) {
    auto state = host.tick(delta_time);
    // Process state.pose, state.bounds, state.body_positions, etc.
}

host.shutdown();
```

Register a presentation callback to execute rendering or capture logic during
the `presentation.dispatch` stage:

```cpp
host.set_presentation_callback([](double dt) {
    // Submit swap chain work, composite overlays, or capture screenshots.
});
```

### Lifecycle

1. **Construction**: Accept `RuntimeHostDependencies` with animation controllers, physics world, geometry, and subsystem plugins
2. **Initialization**: `initialize()` validates dependencies, starts the IO thread pool, and initializes all registered subsystems
3. **Tick**: `tick(dt)` advances animation, physics, deformation, scene updates, and optionally rendering submission
4. **Shutdown**: `shutdown()` tears down subsystems and the IO thread pool gracefully

### Subsystem Integration

The runtime discovers and manages subsystems through the plugin architecture:

- **Explicit plugins**: Pass subsystem instances via `RuntimeHostDependencies::subsystem_plugins`
- **Automatic discovery**: Use `configure_with_default_subsystems()` to load all compiled subsystems
- **Selective enablement**: Provide subsystem names to `configure_with_default_subsystems(enabled_subsystems)`

The subsystem registry validates dependencies and detects cycles during initialization, emitting `RuntimeError::dependency_cycle` when configuration is invalid.

`SubsystemRegistry` also computes a deterministic topological ordering whenever subsystems are loaded so that each subsystem's dependencies are initialized before the subsystem itself, even if the registration order was inverted in configuration code.

### Runtime Loop Planning

`RuntimeLoopBuilder` assembles the stage graph that powers `RuntimeHost::tick`. Stage registration now returns `RuntimeValidationResult` and surfaces structured error codes rather than throwing. Expect the following failures:

- `RuntimeError::loop_stage_invalid_name` when a stage identifier is empty.
- `RuntimeError::loop_stage_duplicate_name` when registering the same identifier twice.
- `RuntimeError::loop_stage_unknown_dependency` when declaring a dependency on an unknown stage.
- `RuntimeError::loop_stage_dependency_cycle` when dependency resolution detects a cycle.

`RuntimeLoopBuilder::build()` returns `RuntimeResult<RuntimeLoopPlan>`; callers must propagate or handle these errors before executing the plan. The default runtime loop logs a validation error and falls back to an empty plan if compilation ever fails.

Each stage descriptor also captures a `thread_affinity` hint so upcoming stage planners can reason about where work should execute. The default (`main_thread`) keeps behaviour unchanged, while `worker_thread` and `any` reserve room for future parallel execution policies. Pass the hint when registering a stage:

```cpp
builder.add_stage(
    "diagnostics.refresh",
    RuntimeLoopPhase::Diagnostics,
    [](double) { /* refresh metrics */ },
    {"runtime.plugins"},
    /*record_in_execution_report=*/false,
    RuntimeLoopThreadAffinity::WorkerThread);
```

The compiled default plan executes the following stages:

1. `animation.evaluate`
2. `physics.accumulate`
3. `physics.integrate`
4. `geometry.deform`
5. `geometry.finalize`
6. `runtime.plugins`
7. `presentation.dispatch`
8. `diagnostics.refresh`

`RuntimeHost::set_loop_plan()` accepts a compiled `RuntimeLoopPlan` and queues it for activation
before the next frame begins. The host clears stage timing history and refreshes
`diagnostics.loop_plan_serialization` whenever a plan swap occurs so tooling immediately observes
the updated graph. Inspect the active plan via `RuntimeHost::loop_plan()` (or the global
`engine::runtime::loop_plan()` helper) when exporting diagnostics or wiring control surfaces.

`presentation.dispatch` bridges the simulation stack to presentation tooling. Provide a presenter by attaching a `rendering::PresentationBackend` to `RuntimeHostDependencies::presentation_backend`; the host invokes it every tick with a `rendering::RuntimePresentationContext` so the backend can submit frame-graph work, composite UI, or trigger readbacks before diagnostics run. Lightweight integrations may continue to register callbacks with `RuntimeHost::set_presentation_callback()` (or the global `engine::runtime::set_presentation_callback()` helper). Both backends and callbacks receive the frame `dt` so presentation logic can track timing alongside simulation state.

## Diagnostics & Telemetry

Access runtime metrics through `RuntimeHost::diagnostics()`:

```cpp
const auto& diag = host.diagnostics();
fmt::print("Ticks: {}, Avg: {:.3f}ms\n", diag.tick_count, diag.average_tick_ms);

// Stage timings
for (const auto& stage : diag.stage_timings) {
    fmt::print("  {}: {:.3f}ms\n", stage.name, stage.last_ms);
}

// Streaming metrics
fmt::print("Streaming: {}/{} completed\n", 
    diag.streaming.streaming_total_completed,
    diag.streaming.streaming_total_requests);

// Scene validation
if (diag.scene_validation.has_cycles) {
    fmt::print("Scene has {} cycles\n", diag.scene_validation.cycle_count);
}
```

### Available Metrics

- **Lifecycle counters**: `initialize_count`, `tick_count`, `shutdown_count`, `initialize_failure_count`
- **Timing data**: `last_*_ms`, `max_*_ms`, `average_tick_ms` plus per-stage timing (`RuntimeStageTiming`, including each stage's `phase` and `thread_affinity`) and aggregated per-phase totals exposed through `RuntimeDiagnostics::phase_timings`
- **Streaming telemetry**: Worker health, queue depth, completion/failure rates (see [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md))
- **Animation telemetry**: Clip/pose metadata plus dispatcher aggregates grouped by animation category and queue, exposed via `RuntimeDiagnostics::animation` and mirrored through the C API for tooling.
- **Scene validation**: Cycle detection, depth analysis, alert levels (see [`DIAGNOSTICS.md`](DIAGNOSTICS.md))
- **Physics collisions**: Contact manifolds, broad-phase metrics
- **Handle validation**: Asset and rendering handle lifecycle tracking (when `ENGINE_ENABLE_ASSETS` is on)

See [`DIAGNOSTICS.md`](DIAGNOSTICS.md) for complete metric reference and troubleshooting workflows.

## Async Streaming Integration

The runtime manages the IO thread pool for asynchronous asset loading:

```cpp
deps.streaming_config.worker_count = 2;
deps.streaming_config.queue_capacity = 64;
deps.streaming_config.enable = true;
```

Access streaming health metrics via `engine::runtime::streaming_metrics()` or through the diagnostics snapshot. Supply cache providers through `RuntimeHostDependencies::asset_streaming` to orchestrate requests directly from the runtime:

```cpp
engine::assets::MeshCache mesh_cache;
engine::assets::PointCloudCache point_cache;

engine::runtime::RuntimeHostDependencies deps{};
deps.asset_streaming.mesh_cache = &mesh_cache;
deps.asset_streaming.point_cloud_cache = &point_cache;

engine::runtime::RuntimeHost host{std::move(deps)};
host.initialize();

auto request = engine::assets::AssetLoadRequest::from_path(
    engine::assets::AssetType::mesh,
    "assets/meshes/rigged_character.glb",
    /*params*/ {},
    engine::assets::AssetLoadPriority::High,
    /*deadline*/ std::nullopt,
    /*allow_blocking_fallback*/ true);

engine::assets::AssetLoadFuture<engine::assets::MeshHandle> future = host.request_mesh_asset(request);
future.wait();
```

`RuntimeHost::request_mesh_asset()` and `RuntimeHost::request_point_cloud_asset()` validate that the IO thread pool is
initialised and the relevant caches are configured before dispatching work. When prerequisites are missing (for example, the
runtime is not initialised or a cache pointer was not supplied) these helpers return futures resolved with
`AssetLoadErrorCategory::ValidationError`, increment streaming rejection telemetry, and log a descriptive message instead of
throwing exceptions. Call `RuntimeHost::mesh_asset_state()` or `RuntimeHost::point_cloud_asset_state()` to inspect the async
state machine for a specific identifier; the accessors return `AssetLoadState::Failed` when the runtime lacks the necessary
cache configuration. Subsystems that require finer control can continue to invoke cache-level `load_async()` helpers directly.

See [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md) for detailed integration patterns and telemetry expectations.

## Rendering Submission

When built with rendering support (`ENGINE_ENABLE_RENDERING=ON`), the runtime submits frame graphs:

```cpp
#if ENGINE_ENABLE_RENDERING
engine::runtime::RuntimeHost::RenderSubmissionContext context{
    .scheduler = my_scheduler,
    .backend = rendering::Backend::Vulkan
};
host.submit_render_graph(context);
#endif
```

Frame-graph metadata and resource events are captured in `diagnostics().frame_graph_serialization` and `diagnostics().frame_graph_events`.

## C API for Tooling

The runtime exposes a C API for Python bindings and external tooling:

```c
engine_runtime_initialize();
engine_runtime_tick(0.016);
size_t body_count = engine_runtime_body_count();
const auto& diag = engine::runtime::diagnostics();
engine_runtime_shutdown();
```

Python scripts in `scripts/diagnostics/` consume these APIs to generate telemetry reports and performance snapshots.

## Configuration

Key configuration surfaces:

- `RuntimeHostDependencies::scene_name`: Labels for diagnostics and telemetry
- `RuntimeHostDependencies::enabled_subsystems`: Explicit subsystem selection
- `RuntimeHostDependencies::streaming_config`: IO thread pool tuning
- `RuntimeHostDependencies::asset_streaming`: Optional asset cache providers for async loads
- Environment variable `ENGINE_PLATFORM_WINDOW_BACKEND`: Override window backend at runtime

## Dependencies

- **Core**: ECS registry, telemetry schema, plugin interfaces, IO thread pool
- **Animation**: Clip evaluation, blend trees, deformation transforms
- **Physics**: Rigid-body simulation, collision detection
- **Geometry**: Mesh deformation, spatial queries, bounds computation
- **Scene**: Hierarchy validation, transform propagation
- **Assets** (optional): Handle validation, async queue metrics
- **Rendering** (optional): Frame-graph submission, resource tracking
- **Compute**: Kernel dispatcher for physics and deformation workloads

## Testing

Integration tests live in `engine/tests/integration/test_runtime_integration.cpp` and cover:

- Lifecycle validation (initialize, tick, shutdown)
- Subsystem registration and dependency resolution
- Frame-graph submission determinism
- Streaming telemetry accuracy
- Scene validation integration

Run tests via:
```bash
ctest --preset linux-gcc-debug -R runtime
```

## Current State

- `RuntimeHost` orchestration advancing animation, compute-driven physics, CPU linear blend skinning, geometry deformation, and submission into the rendering pipeline. Diagnostics expose stage timings, streaming metrics, scene validation, and frame-graph metadata.
- Compiled loop plans surface through the `RuntimeLoopInspector`, emitting a deterministic JSON description (`diagnostics.loop_plan_serialization`) so tooling can introspect phases, dependencies, thread affinity hints, and execution reporting flags alongside frame-graph captures.

## Usage

- Run runtime integration tests:
  - `ctest --preset linux-gcc-debug -R runtime`
- See `engine/tests/integration/test_runtime_integration.cpp` and `scripts/diagnostics/` for telemetry scripts.
- Capture dispatcher telemetry using the compute runtime sample:
  ```bash
  cmake --build --preset <preset> --target engine_compute_runtime_sample
  ./out/build/<preset>/engine/compute/engine_compute_runtime_sample \
      --frames 1024 --dt 0.016 --workload balanced --dispatcher-backend cpu --queues 3 --baseline \
      --repeat 3 --jitter-budget-ms 0.5 \
      --queue-names rt-main,rt-async,rt-deform --queue-map geometry=rt-deform \
      --output telemetry/compute_dispatch.json --output-dir telemetry/runs
  python scripts/diagnostics/compute_dispatch_report.py --input telemetry/compute_dispatch.json --top 5
  ```
  The workflow exercises `RuntimeHost` end-to-end, records per-kernel timings,
  reports queue utilisation, enumerates cross-queue fences, and surfaces jitter
  warnings for `CO-170`. The runtime sample exports the frame dispatch jitter σ
  alongside the configured budget (default 0.5 ms) so the console summary and
  diagnostics report flag runs that exceed the latency target. With `--baseline`,
  the report also captures a single-queue reference run and flags when the
  observed speed-up drops below the `1.5×` target. The JSON summary includes a
  `summary.memory` section and the CLI output prints a GPU staging estimate so
  runs breaching the 256 MiB animation budget are easy to diagnose. Provide
  `--dispatcher-backend cuda` when available to profile GPU dispatchers; the
  metadata records the backend for both the optimised run and the baseline so
  diagnostics remain self-contained. When `--repeat` is used the harness emits
  multiple captures automatically, names them with a `-runXX` suffix, and embeds
  `run_index`/`run_count` in metadata so diagnostics and CI dashboards can
  correlate per-run summaries with run-to-run variance checks. Queue labels fall
  back to deterministic FNV-1a hashing so workload categories without explicit
  overrides remain stable between runs.

- Automate the ≤2% variance verification with
  `scripts/diagnostics/compute_dispatch_benchmark.py`:
  ```bash
  python scripts/diagnostics/compute_dispatch_benchmark.py \
      --sample ./out/build/<preset>/engine/compute/engine_compute_runtime_sample \
      --runs 3 --frames 1024 --workload balanced --queues 3 --baseline \
      --jitter-budget-ms 0.5 --variance-threshold 2.0 --exit-on-regression \
      --output-dir telemetry/dispatch_benchmark
  ```
  The helper forwards queue overrides/backends, emits the same telemetry, and
  warns when variance, jitter, or speed-up budgets regress. Supply `--input`
  when benchmarking previously captured JSON payloads and `--report` to archive
  the textual summary in CI artefacts.

### Prototyping Harness (AI-004)

- Follow the end-to-end workflow in the
  [`AI-004 Prototyping Playbook`](../../design/AI-004-prototyping-playbook.md) to align schema validation, dataset packaging,
  harness execution, sandbox integration, and benchmarking automation steps.
- A sample configuration lives at `docs/examples/ai004_sample.json` and
  references the `assets/datasets/remesh_sample` manifest so new contributors
  can exercise the harness without authoring manifests from scratch. Review
  `docs/examples/README.md` for a field-by-field breakdown before iterating.
  Validate AI-004 configurations with the Python harness scaffold:
  ```bash
  python -m scripts.prototyping.run_prototype_harness --config docs/examples/ai004_sample.json --dry-run
  ```
  The harness consumes the shared configuration schema, reports the selected
  dataset/preset pairing, and can execute a headless tick loop once the runtime
  shared library is available locally. Execution summaries surface the runtime's
  rolling `average_tick_ms` diagnostic and per-kernel dispatch telemetry (order +
  milliseconds) so teams can monitor timing and scheduling drift without
  additional tooling. Pass `--frames` and `--dt` to control
  execution cadence; use `--dry-run` to perform schema validation without
  loading the native runtime library.
- Packaged case studies ship under `assets/datasets/case_studies/`. Launch them
  directly without locating manifests manually:
  ```bash
  python -m scripts.prototyping.run_prototype_harness --case-study geometry-baseline --dry-run
  python -m scripts.prototyping.run_prototype_harness --case-study rendering-debug --dry-run
  ```
  The CLI prints the resolved manifest path, writes summaries when
  `--summary-json`/`--describe-json` are provided, and keeps the registry in
  sync with the AI-004 kickoff plan tracked by `RT-321`. Baseline expectations
  for both scenarios (datasets, rendering presets, telemetry outputs, and
  manifest metrics) are recorded in
  [`docs/design/RT-321-case-studies.md`](../../design/RT-321-case-studies.md).
- Continuous integration exercises both case studies via CTest:
  ```bash
  ctest --preset <preset> -R runtime_prototype_harness_geometry_case_study
  ctest --preset <preset> -R runtime_prototype_harness_rendering_case_study
  ```
  The tests launch the harness CLI in dry-run mode with schema enforcement
  enabled, write run summaries into the build artefact directory, and apply the
  mock window backend so they remain deterministic on headless runners.
- A native C++ sample (`runtime_prototype_harness`) mirrors the Python CLI for
  dry-run validation:
  ```bash
  ./out/build/<preset>/engine/runtime/runtime_prototype_harness \
      --config docs/examples/ai004_sample.json \
      --dry-run --require-schema \
      --summary-json telemetry/runtime_sample_summary.json
  ```
  The executable resolves AI-004 manifests through the shared configuration
  loader, validates dataset references, and records execution summaries that the
  integration test `runtime_prototype_harness_sample_dry_run` consumes. Those
  summaries now report dispatch counts, execution order, and kernel durations in
  milliseconds so CC-310 automation can diff native runs without parsing logs.
  Build and smoke-test instructions live in `engine/runtime/samples/README.md`.
- Enable strict schema enforcement during the migration by exporting
  `ENGINE_AI004_SCHEMA_V1=1`. When the variable is unset the loaders inject
  default schema headers so legacy manifests remain compatible while teams
  complete the transition to the v1 specification.
- `run_prototype_harness` also exposes `--require-schema`, which forwards the
  enforcement request directly to the loader for per-run validation. Use it in
  CI or local smoke tests to guarantee manifests declare `ai-004.*` headers
  before runtime ticks execute.
- Harness construction validates runtime scene manifests and configuration
  summaries expose `runtime.scene_manifest_path` so tooling surfaces the
  resolved asset location alongside dataset integrity checks.
- Native runtime integrations consume the same schema helpers through
  `engine::runtime::config::load_dataset_manifest()` and
  `engine::runtime::config::load_configuration()` in
  [`engine/runtime/config_schema.hpp`](../../../engine/runtime/include/engine/runtime/config_schema.hpp).
  The implementation relies on `yaml-cpp` and returns
  `RuntimeResult<T>` so harness code can surface actionable errors. Regression
  coverage lives in
  [`engine/runtime/tests/test_config_schema.cpp`](../../../engine/runtime/tests/test_config_schema.cpp).
- Migration checklist for legacy manifests:
  1. Run `python -m scripts.validate_ai004_config --dataset <path> --config <path>`
     to surface missing headers or field violations.
  2. Update manifests to include the schema blocks described in
     [`docs/design/AI-004-configuration-schema.md`](../../design/AI-004-configuration-schema.md).
  3. Re-run the harness with `--require-schema` (or set
     `ENGINE_AI004_SCHEMA_V1=1`) to confirm the updated manifests satisfy v1.
  4. Remove temporary overrides once all modules publish schema-compliant
     manifests; the feature flag can then become the default behaviour.

## TODO / Next Steps

- Execute [`RT-410`](../../backlog/active/RT-410-runtime-stage-planner.md): ship the stage planner, presentation adapters, and synchronisation hooks mandated by [`ADR-0008`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md).
- Pair with rendering on [`T-0119`](../../backlog/active/T-0119-command-encoder-integration.md) and [`T-0120`](../../backlog/active/T-0120-gpu-resource-provider.md) milestones so submission timing and telemetry contracts stay aligned.
- Maintain the prototyping harness/case studies as new datasets land and record follow-on scenarios in [`../../ROADMAP.md`](../../ROADMAP.md) when they enter planning.

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module-specific task tracking
- [`DIAGNOSTICS.md`](DIAGNOSTICS.md): Comprehensive telemetry reference and troubleshooting
- [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md): Asset loading workflows
- [`../../specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md): Main loop configuration, presentation, and tooling registry.
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): System-level data flow and invariants
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Shared metric definitions
- [`../../archive/backlog/legacy/tasks/T-0104-runtime-frame-graph-integration.md`](../../archive/backlog/legacy/tasks/T-0104-runtime-frame-graph-integration.md): Frame-graph integration milestone
