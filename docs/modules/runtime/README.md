# Runtime Module

## Overview

> **Status:** ✅ **Stable** — `RuntimeHost` executes the declarative `RuntimeLoopPlan` with deterministic stage planning, shared presentation adapters, and telemetry delivered through archived [`RT-410`](../../../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) per [`ADR-0008`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md).

The runtime module orchestrates the engine's main execution loop through `RuntimeHost`, which coordinates animation evaluation, physics simulation, geometry deformation, scene graph updates, and rendering submission. It acts as the integration point for all subsystems and provides comprehensive diagnostics and telemetry.

**NEW:** The runtime module now includes an `Application` base class that provides high-level application lifecycle management, automatic subsystem wiring, and a built-in main loop. Applications can inherit from `Application` and override lifecycle callbacks instead of manually managing windows, scenes, and the main loop.

## Application Framework (NEW)

### Quick Start

The simplest way to create an engine application is to inherit from `runtime::Application`.
When rendering is enabled (see [Rendering Integration (TL-310-2a)](#rendering-integration-tl-310-2a)),
the base class exposes a `rendering::RenderExecutionContext` so derived applications can execute
frame graphs without manually wiring presentation backends.

```cpp
#include "engine/runtime/application.hpp"

class MyApp : public engine::runtime::Application
{
protected:
    void on_initialize() override
    {
        // Setup scene, load resources
        auto entity = scene().create_entity();
        // ...
    }
    
    void on_update(double dt) override
    {
        // Handle input and game logic
        auto& inp = input();
        if (inp.is_key_down(platform::input::Key::W)) {
            // Move forward
        }
    }
    
    void on_render() override
    {
#if ENGINE_ENABLE_RENDERING
        // Use render_context() to execute your frame graph or presentation logic.
        // Example: viewer_frame_graph.execute(render_context());
#endif
    }
};

int main()
{
    MyApp app;
    return app.run();
}
```

### Application Configuration

Customize the application with `ApplicationConfig`:

```cpp
class MyApp : public engine::runtime::Application
{
public:
    MyApp()
        : Application({
            .window = {
                .title = "My Application",
                .width = 1920,
                .height = 1080,
                .visible = true,
                .resizable = true
            },
            .window_backend = platform::WindowBackend::Mock,
            .target_fps = 60.0,
#if ENGINE_ENABLE_RENDERING
            .rendering = {
                .enable = true,
                .backend = engine::runtime::ApplicationConfig::RenderingConfig::Backend::Mock,
            },
#endif
        })
    {
    }
};
```

### Lifecycle Callbacks

Override virtual methods to implement application-specific behavior:

- **`on_initialize()`** - Called once before main loop starts. Setup scene, load assets, etc.
- **`on_update(double dt)`** - Called every frame. Handle input, update game logic.
- **`on_render()`** - Called every frame after update. Rendering logic (driven by RT-410 presentation adapters).
- **`on_shutdown()`** - Called once after main loop exits. Clean up resources.

### Subsystem Accessors

Access engine subsystems through protected methods:

- **`window()`** - Access the platform window
- **`input()`** - Convenience accessor for `window().input_state()`
- **`scene()`** - Access the scene graph
- **`elapsed_time()`** - Total time since application start
- **`frame_count()`** - Current frame number

### Shutdown

Request graceful shutdown from anywhere in your application:

```cpp
void on_update(double dt) override
{
    if (input().was_key_pressed(platform::input::Key::Escape)) {
        quit();  // Exit with code 0
    }
    
    if (some_error_condition) {
        quit(1);  // Exit with error code
    }
}
```

### Example: Geometry Viewer

See [`engine/tools/examples/geometry_viewer.cpp`](../../../engine/tools/examples/geometry_viewer.cpp) for a complete example using the Application framework.

### Rendering Integration (TL-310-2a)

`TL-310-2a` connects the `Application` base class to the presentation backends delivered in
[`RT-410`](../../../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md). Enabling
`ApplicationConfig::rendering.enable` now wires the entire rendering stack:

- A `rendering::RenderExecutionContext` is constructed during initialization and is available
  via the protected `render_context()` accessor.
- GPU resource providers, material systems, scheduler stubs, and command encoders are provisioned
  automatically so frame graphs can execute without additional boilerplate.
- A presentation backend is created on demand. Provide `rendering.backend_factory` to inject a
  concrete backend; otherwise the mock backend is instantiated to keep headless environments working.
- `RuntimeHost` is spun up lazily the first time a presentation backend is used, preserving
  compatibility with pre-existing stage-planner workflows.

**Headless validation.** CI containers omit the XRandR headers required by GLFW, so
`cmake --preset linux-gcc-debug` skips the `geometry_viewer` target. The mock backend still drives
`ApplicationRendering.ProvidesContextAndInvokesPresentation`, giving deterministic coverage of the
rendering lifecycle even when real swap-chain integration is unavailable.

**Desktop backends.** Installing GLFW dependencies (for example, `apt install libglfw3-dev libxrandr-dev`)
allows a desktop build to provide an OpenGL-backed presentation factory via
`rendering.backend_factory`, while the mock backend remains available for tests and automation.

## Outstanding Work

- Expand the new `RuntimeLoopPlan` stage planner with presentation adapters and runtime configurability described in [`ADR-0008`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) (delivered via archived `RT-410`).
- Provide synchronisation hooks for scripting, diagnostics, and tooling integrations in tandem with rendering backends.
- Exercise the GPU-backed submission stack shipped in [`T-0120`](../../../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) while extending presentation hooks; the command encoder integration ([`T-0119`](../../../hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md)) is already live.

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

Callbacks or presentation backends can be hot-swapped at runtime. `RuntimeHost`
rebuilds its default loop plan whenever handlers change or the presentation
backend pointer is replaced so `presentation.dispatch` telemetry only appears
when a callback or presenter is active. Rendering builds may attach presenters
after initialization via `host.set_presentation_backend(...)` without
reconfiguring dependencies, and new presenters activate immediately even if a
callback was already registered.
Check whether presentation handlers are active through
`host.presentation_stage_active()` (or the global
`engine::runtime::presentation_stage_active()` helper) to gate tooling and
diagnostics behaviour without parsing execution reports.
Tooling that links against the C interface can query
`engine_runtime_presentation_stage_active()` (exposed in Python via
`EngineRuntimeHandle.presentation_stage_active()`) for the same boolean without
touching diagnostics payloads.

Use `engine_runtime_loop_plan_serialization()` to retrieve the JSON description
of the currently active loop plan. Python tooling can call
`EngineRuntimeHandle.loop_plan_serialization()` to confirm stage ordering before
recording telemetry or orchestrating scripted presentation captures. The helper
returns an empty string when the runtime has not been initialized or when a
plan has not yet been compiled, matching the native runtime diagnostics.

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

Each stage descriptor also captures a `thread_affinity` hint so the stage planner can reason about where work should execute. The default (`main_thread`) keeps behaviour unchanged, while `worker_thread` and `any` reserve room for future parallel execution policies. Pass the hint when registering a stage:

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
Scripting clients can pull the JSON snapshot directly through the
`engine_runtime_loop_plan_serialization()` C export (exposed as
`EngineRuntimeHandle.loop_plan_serialization()` in the Python loader) to validate
stage ordering without traversing the entire diagnostics payload.

`RuntimeStagePlanner` now wraps the compiled plan inside the host. Each stage iteration returns a
`RuntimeStageExecution` containing a stable `RuntimeStageHandle` with the stage index, phase,
thread affinity, execution-report flag, and a `StageBudget` hint. The budget records the nominal
target duration (in milliseconds) plus an `enforce_budget` flag that telemetry surfaces whenever a
stage exceeds its allocation, allowing diagnostics to warn long before PM-510 demos regress. Calling
`next_stage()` without first invoking `configure_plan()` surfaces
`RuntimeError::loop_stage_planner_unconfigured`, while unexpected index gaps raise
`RuntimeError::loop_stage_planner_invalid_iteration` and abort the frame so invariants stay intact.

`presentation.dispatch` bridges the simulation stack to presentation tooling. Provide a presenter by attaching a `rendering::PresentationBackend` to `RuntimeHostDependencies::presentation_backend`; the host invokes it every tick with a `rendering::RuntimePresentationContext` so the backend can submit frame-graph work, composite UI, or trigger readbacks before diagnostics run. Lightweight integrations may continue to register callbacks with `RuntimeHost::set_presentation_callback()` (or the global `engine::runtime::set_presentation_callback()` helper). Both backends and callbacks receive the frame `dt` so presentation logic can track timing alongside simulation state.

Headless workflows that run without a GPU-backed swap chain can attach
`rendering::backend::mock::MockPresentationBackend` to keep the presentation
stage active and capture invocation cadence for diagnostics or tests without
driving real rendering hardware.

When wiring presentation backends, construct surfaces via
`runtime::create_presentation_surface()`. The helper accepts a
`RuntimePresentationSurfaceConfig` bundling a platform window description,
backend preference (mock, GLFW, or auto), renderer identifier, and optional
swapchain hook so runtime hosts, tooling harnesses, and presentation backends
share a single configuration path. The returned
`RuntimePresentationSurface` exposes the window, event queue, and swapchain
surface; errors are reported through `RuntimeError::presentation_surface_*`
codes so diagnostics align with the stage planner's telemetry exports.

Backends can also be swapped in and out after initialization through
`RuntimeHost::set_presentation_backend()`, ensuring the presentation stage is
recorded or removed from execution reports automatically as presenters become
available.

The rendering module now ships `rendering::backend::opengl::OpenGLPresentationBackend`
for hosts that want a ready-made OpenGL execution path. Supply a mesh resolver
 and register materials with its exposed `MaterialSystem` before attaching the
 backend to the runtime dependencies; runtime dispatch populates
 `RuntimePresentationContext::submit_render_graph` so `present()` can forward
 the bundled submission context to `RuntimeHost::submit_render_graph()` each
 tick. Presentation consumers can tune transient GPU caching by passing a
 retention-frame count to the backend constructor or invoking
 `set_resource_retention_frames()`, which forwards to the GPU resource
 provider so demos can balance memory pressure against reuse while RT-410
 stage planner integration proceeds.

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
- **GPU resource telemetry**: Buffer/texture/auxiliary usage gauges (`rendering.resources.*`) sourced from the active GPU resource provider so tooling can monitor residency alongside frame-graph execution.
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
  [`AI-004 Prototyping Playbook`](../../design/AI_004_PROTOTYPING_PLAYBOOK.md) to align schema validation, dataset packaging,
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
  [`docs/design/RT_321_CASE_STUDIES.md`](../../design/RT_321_CASE_STUDIES.md).
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
     [`docs/design/AI_004_CONFIGURATION_SCHEMA.md`](../../design/AI_004_CONFIGURATION_SCHEMA.md).
  3. Re-run the harness with `--require-schema` (or set
     `ENGINE_AI004_SCHEMA_V1=1`) to confirm the updated manifests satisfy v1.
  4. Remove temporary overrides once all modules publish schema-compliant
     manifests; the feature flag can then become the default behaviour.

## TODO / Next Steps

- Leverage archived [`RT-410`](../../../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md): stage planner, presentation adapters, and synchronisation hooks mandated by [`ADR-0008`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) are available for tooling integration.
- Continue pairing with rendering on [`T-0120`](../../../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) deliverables by exercising the shipped command encoder (`T-0119`) against GPU-backed submissions during RT-410 validation.
- Maintain the prototyping harness/case studies as new datasets land and record follow-on scenarios in [`../../ROADMAP.md`](../../ROADMAP.md) when they enter planning.

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module-specific task tracking
- [`DIAGNOSTICS.md`](DIAGNOSTICS.md): Comprehensive telemetry reference and troubleshooting
- [`ASYNC_STREAMING_INTEGRATION.md`](ASYNC_STREAMING_INTEGRATION.md): Asset loading workflows
- [`../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md): Main loop configuration, presentation, and tooling registry.
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): System-level data flow and invariants
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Shared metric definitions
- [`../../archive/backlog/legacy/tasks/T_0104_RUNTIME_FRAME_GRAPH_INTEGRATION.md`](../../archive/backlog/legacy/tasks/T_0104_RUNTIME_FRAME_GRAPH_INTEGRATION.md): Frame-graph integration milestone
