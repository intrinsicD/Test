# Tools Module

## Overview

The tools module provides editor utilities, profiling tools, pipeline automation, and diagnostics viewers. It includes the telemetry viewer CLI for runtime snapshots, integration with Dear ImGui for debug UI, and runtime packaging scripts for CI/CD workflows.

**Status:** 🔧 **Feature-gated** – The module builds when `ENGINE_ENABLE_TOOLS=ON` (the repository presets enable it by default) and now rides on the archived [`TL-310`](../../../hybrid_workflow/backlog/archive/TL-310-editor-foundations.md) baseline that restored the editor harness, panel registry, and runtime bridge.

> **Note:** The `geometry_viewer` sample depends on the GLFW backend and the generated `glad::gl_core` loader. CMake automatically skips the executable when either dependency is unavailable (for example, when Python/Jinja are missing or GLFW is disabled) so the canonical build presets continue to configure successfully.

## Outstanding Work

- Execute the performance metrics panel and diagnostics overlays captured in [`TL-312`](../../../hybrid_workflow/backlog/TL-312-performance-metrics-panel.md), [`TL-313`](../../../hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md), and [`TL-314`](../../../hybrid_workflow/backlog/TL-314-telemetry-visualization-panel.md) on top of the TL-310 foundation; TL-312 and TL-314 implementations are now in review pending acceptance of the editor panels.
- Implement the panel registry, runtime harness bridge, and ImGui reuse strategy from [`ADR-0008`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md).
- Validate editor flows against GPU-enabled runtime now that [`T-0120`](../../../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) shipped, and integrate the archived [`RT-410`](../../../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) synchronisation hooks so tooling telemetry stays aligned with runtime submissions.
- Drive the shared selection engine across runtime/editor tooling. [`TL-315`](../../../hybrid_workflow/backlog/archive/TL-315-selection-engine-and-hit-testing.md)
  landed the ordered selection stack, change notifications, and bounding-box hit-testing fallback that future TL-316/TL-317
  follow-ups will extend with primitive-level selections and overlay rendering.

## Telemetry Viewer CLI

Command-line tool for capturing and analyzing runtime diagnostics:

```bash
# Capture telemetry snapshot
# Adjust the preset directory to match your build tree.
python scripts/diagnostics/runtime_frame_telemetry.py \
    --library-dir out/build/linux-gcc-debug \
    --output telemetry_snapshot.json

# View telemetry report
python scripts/diagnostics/telemetry_viewer.py telemetry_snapshot.json
```

Features:
- **Lifecycle metrics**: Initialize/tick/shutdown counts and timings
- **Streaming health**: Async queue metrics, completion/failure rates
- **Scene validation**: Cycle detection, depth analysis, alert levels
- **Physics telemetry**: Collision metrics, manifold cache statistics
- **Handle validation**: Asset and rendering handle lifecycle tracking

The viewer integrates with the telemetry schema from `CC-001`:

```python
from engine3g import telemetry

# Load snapshot
snapshot = telemetry.load_snapshot("telemetry_snapshot.json")

# Query metrics
tick_count = snapshot.get_counter("runtime.lifecycle.tick")
avg_tick_ms = snapshot.get_histogram("runtime.tick_ms").average

print(f"Ticks: {tick_count}, Avg: {avg_tick_ms:.3f}ms")
```

## Dear ImGui Integration

Debug UI for runtime inspection:

```cpp
#include "engine/tools/imgui_helpers.hpp"

// In main loop
tools::imgui::begin_frame();

if (ImGui::Begin("Runtime Diagnostics")) {
    const auto& diag = runtime::diagnostics();
    
    ImGui::Text("Tick: %llu", diag.tick_count);
    ImGui::Text("Avg: %.3fms", diag.average_tick_ms);
    
    if (ImGui::CollapsingHeader("Streaming")) {
        ImGui::Text("Pending: %zu", diag.streaming.streaming_pending);
        ImGui::Text("Completed: %llu", diag.streaming.streaming_total_completed);
        ImGui::Text("Failed: %llu", diag.streaming.streaming_total_failed);
    }
    
    if (ImGui::CollapsingHeader("Scene Validation")) {
        tools::imgui::render_validation_report(diag.scene_validation);
    }
}
ImGui::End();

tools::imgui::end_frame();
```

## Selection Engine & Hit Testing

[`SelectionEngine`](../../../engine/scene/include/engine/scene/selection/selection_engine.hpp) now lives in the **scene**
module so runtime systems and editor tooling share the same selection primitives. Strategies implement
`SelectionStrategy::try_pick` and register with explicit priorities, letting GPU color picking preempt bounding-box fallbacks
while marquee/scripted selections still push events directly into the stack. The engine deduplicates entities by default,
exposes `ordered_selection()` for tooling overlays, and notifies listeners whenever a new selection is committed.

The Geometry Viewer consumes `SelectionOutlineRenderer` to visualise the selection stack directly inside the viewport. The
Selection Inspector now exposes the full outline surface (enable/disable, colour/alpha, hybrid thickness tunables, occlusion
modes, quality preset, and per-strategy override) backed by
[`OutlineConfig`](../../../engine/scene/include/engine/scene/selection/visualization/outline_config.hpp), so PM-510 rehearsals
can experiment with JFA-style halos or fast edge detection without recompiling shaders. Because the renderer plugs into the
research baseline frame graph, future tooling (scene hierarchy overlays, diagnostics) can reuse the same API to draw outlines
around the entities they manage.

The initial [`BoundingBoxSelectionStrategy`](../../../engine/scene/include/engine/scene/selection/bounding_box_strategy.hpp)
implements the TL-315 fallback path. It iterates world transforms, derives a conservative axis-aligned bounding box per entity,
and resolves hits via the existing ray–box intersection helpers. Integrations can inject a `BoundsProvider` when higher-fidelity
data exists (BVHs, GPU ID buffers, etc.), while `SelectionContext` packages the active `scene::Scene` pointer and cursor ray so
strategies share projection logic. The `SceneHierarchyPanel` and `RuntimePanelBridge` bind directly to the selection engine so
editor clicks, runtime hit tests, and scripted selections all reuse the same ordered stack and change notifications.

`TL-316` layers primitive metadata on top of that stack through
[`PrimitiveSelectionRegistry`](../../../engine/scene/include/engine/scene/selection/primitive_selection.hpp). Tooling panels can
register adapters that translate `SelectionHit` events into lists of vertices, edges, faces, or point samples, stream those hits
into bounded buffers, and iterate them in fixed-size chunks when emitting overlay draw calls. The registry mirrors TL-315’s
selection context (cursor ray, entity ordering) so TL-317’s outline renderer and diagnostics overlays receive both entity IDs and
the exact primitive set the user interacted with without mutating mesh buffers.

## Experiment Sandbox UI

The experiment sandbox provides an ImGui workspace for AI-004 prototyping
workflows. It consumes the harness configuration summary generated by
`python -m scripts.prototyping.run_prototype_harness --describe-json` and
exposes dataset, rendering, telemetry, and benchmarking controls in a single
window. The summary now includes schema versions, dataset provenance
(feature-preservation flags, parameterisation metrics, asset checksums), and
telemetry outputs/metrics/sampling cadence so the UI can validate
compatibility before applying user actions.

Key capabilities:

- **Dataset browser** – searchable list of datasets with statistics, schema
  metadata, metric snapshots, feature-preservation flags, parameterisation
  metrics, and asset verification summaries (existence, size/hash checks,
  and harness-supplied diagnostics for each source/output artefact).
- **Case study presets** – the preferences panel lists packaged AI-004 case
  studies (see [`docs/design/RT_321_CASE_STUDIES.md`](../../design/RT_321_CASE_STUDIES.md))
  with default dataset/preset/runtime selections and benchmark scenarios. The
  "Load" action synchronises harness callbacks (`on_case_study_requested`,
  `on_case_study_selected`) while updating dataset, rendering, and benchmark
  panels to mirror the case study defaults.
- **Rendering controls** – preset and shading-mode selection with overlay
  toggles that mirror the research baseline options.
- **Benchmark triggers** – frame-count and timestep inputs with a one-click
  "Run Benchmark" action that surfaces results through the callback
  interface. The UI records the most recent run outcome, lists configured
  benchmark scenarios (dataset/preset/profile assignments plus metric
  thresholds), and displays the harness summary directly in the panel.
- **Telemetry panel** – live FPS/CPU/GPU timing counters and plotted series
  aligned with the shared telemetry schema, validating streams against the
  telemetry summary provided by the harness. Incoming series are decimated to
  256 samples before plotting so the ImGui path stays within the 1 ms/frame UI
  budget even when callers submit long histories.
- **Programmatic control** – helpers (`select_dataset`, `select_rendering_preset`,
  `set_shading_mode`, `set_overlay_enabled`) let automation mirror UI selections
  while the registered callbacks update the harness in lockstep.
- **Immediate synchronisation** – `set_configuration`, `set_preferences`, and
  `load_preferences` dispatch dataset/rendering callbacks as soon as the active
  selection changes. Registering callbacks via `set_callbacks` replays the
  current preferences so harness integrations stay in sync even when listeners
  attach after the initial configuration step.
- **Persistence** – user preferences persist via
  `ExperimentSandbox::save_preferences`, while layout state uses ImGui's
  INI saving hooks for reproducible workspace setups.

Minimal usage:

```cpp
#include "engine/tools/sandbox/experiment_sandbox.hpp"
#include "engine/tools/sandbox/configuration_loader.hpp"

using namespace engine::tools::sandbox;

ExperimentSandbox sandbox;
ExperimentConfigurationSummary summary = load_summary_from_json(path);
sandbox.set_configuration(summary);
sandbox.set_callbacks({
    .on_dataset_selected = [](const std::string& id) { /* update harness */ },
    .on_rendering_changed = [&](const SandboxPreferences& prefs) {
        persist_rendering_overrides(prefs);
    },
    .on_run_benchmark = [&](const SandboxPreferences& prefs) {
        queue_benchmark_run(prefs);
    },
});

// Programmatically drive selections without synthesising ImGui input.
sandbox.select_dataset("remesh-sample");
sandbox.select_rendering_preset("research");
sandbox.set_overlay_enabled("normals", true);

// During the UI frame
sandbox.render();
```

Persist sandbox state before shutdown to keep preferences in sync across
research sessions:

```cpp
sandbox.save_preferences(config_dir / "sandbox.ini");
sandbox.save_layout(config_dir / "sandbox_layout.ini");
```

Refer to [`docs/design/TL_210_EXPERIMENT_SANDBOX.md`](../../design/TL_210_EXPERIMENT_SANDBOX.md)
for architecture and integration guidance.

### Benchmark Automation

`PrototypeHarnessBenchmarkRunner` provides a ready-to-use integration with the
headless prototyping harness. Configure it with the command prefix used to
launch the harness (for example,
`{"python", "-m", "scripts.prototyping.run_prototype_harness", "--config", ...}`)
and the directory where summary files should be written. The runner appends the
frame count, timestep, and `--summary-json` arguments, executes the command, and
parses the resulting JSON to populate the sandbox callback:

```cpp
PrototypeHarnessBenchmarkRunner runner({
    "python", "-m", "scripts.prototyping.run_prototype_harness",
    "--config", config_path.string(),
}, summaries_dir);

sandbox.set_callbacks({
    .on_dataset_selected = update_dataset,
    .on_rendering_changed = persist_rendering,
    .on_run_benchmark = [&](const SandboxPreferences& prefs) {
        return runner.run(prefs);
    },
});
```

The runner forwards sandbox preferences to the harness CLI via `--dataset`,
`--runtime-profile`, `--rendering-preset`, `--shading-mode`, and repeated
`--overlay <key>=<bool>` arguments so headless executions mirror UI selections.
These options are also available when invoking the CLI directly:

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dry-run --dataset remesh-variant --runtime-profile diagnostics \
    --rendering-preset diagnostics --shading-mode forward \
    --overlay normals=1 --overlay uv=0 \
    --summary-json telemetry/diagnostics.json
```

The sandbox automatically displays the returned `SandboxBenchmarkResult`,
highlighting success in green and errors in red, while preserving the summary
file for audit trails.

`ComparativeBenchmarkRunner` extends this workflow to the
[`scripts/benchmarks/run_comparative_benchmarks.py`](../../../scripts/benchmarks/run_comparative_benchmarks.py)
orchestrator. Instantiate it with the Python interpreter + script prefix and a
working directory for temporary configurations:

```cpp
auto comparative_runner = std::make_shared<ComparativeBenchmarkRunner>(
    std::vector<std::string>{
        "python",
        (project_root / "scripts/benchmarks/run_comparative_benchmarks.py").string(),
    },
    summaries_dir);

sandbox.set_comparative_benchmark_runner(comparative_runner);
```

When configured, the sandbox "Run Benchmark" button automatically selects the
scenario whose dataset/runtime profile/rendering preset match the active
preferences, writes the comparative config to disk, executes the orchestrator,
and parses the resulting JSON/CSV summaries into a `SandboxBenchmarkResult`.
Scenario and metric failures are surfaced inline, while summary/table paths are
preserved for audit trails. If the current selection does not match any scenario
the sandbox falls back to the first declared scenario and annotates the result
with mismatch warnings so the operator can reconcile their selections.

Programmatic integrations can call `ExperimentSandbox::run_active_benchmark()`
to mirror the button without synthesising ImGui input, making automation
scripts deterministic.

## Runtime Packaging

Automate runtime artifact packaging for distribution:

```bash
# Package runtime artifacts
python scripts/ci/package_runtime_artifacts.py \
    --build-dir build \
    --output artifacts/ \
    --platform linux-x64

# Generates:
# - artifacts/libengine_runtime.so
# - artifacts/manifest.json (with checksums)
# - artifacts/telemetry_schema.json
```

The packaging script:
- Collects runtime libraries and dependencies
- Generates manifest with version and checksums
- Includes telemetry schema for tooling compatibility
- Validates artifact completeness

Integrated into CI pipelines to ensure reproducible builds.

## Profiling Utilities

Performance profiling helpers:

```cpp
#include "engine/tools/profiling/profiler.hpp"

// Scoped profiling
{
    PROFILE_SCOPE("PhysicsUpdate");
    world.step(dt);
}

// Manual profiling
tools::profiling::Profiler profiler;
profiler.begin("RenderSubmission");
submit_frame_graph();
profiler.end("RenderSubmission");

// Generate report
auto report = profiler.generate_report();
for (const auto& entry : report.entries) {
    fmt::print("{}: {:.3f}ms\n", entry.name, entry.duration_ms);
}
```

## Pipeline Automation

Tools for asset pipeline automation:

```bash
# Process asset directory
python scripts/tools/process_assets.py \
    --input assets/raw/ \
    --output assets/processed/ \
    --formats obj,stl,ply

# Validate asset integrity
python scripts/tools/validate_assets.py assets/processed/
```

Asset processor features:
- Format conversion (OBJ → custom binary format)
- Mesh optimization (vertex cache, overdraw reduction)
- Texture compression
- Validation and checksumming

## Editor (Planned)

Future editor application built on the tools module:

- **Scene editor**: Visual scene graph editing with ImGui
- **Material editor**: Shader graph authoring
- **Animation editor**: Timeline-based clip editing
- **Performance profiler**: Frame graph visualization

Currently in early planning stages. See module ROADMAP for milestones.

## Diagnostics Shell

Interactive shell for runtime diagnostics:

```bash
# Start diagnostics shell
# Adjust the path to reflect your selected preset.
python scripts/diagnostics/shell.py --runtime out/build/linux-gcc-debug/libengine_runtime.so

# Interactive commands
> show metrics runtime.lifecycle.*
> show validation scene
> export snapshot.json
> reload assets/meshes/character.obj
```

The shell integrates with the hot reload infrastructure (`CC-002`) to trigger asset reloads from the command line.

## Testing

Tools tests validate:
- Telemetry viewer accuracy (`tests/test_telemetry_viewer.py`)
- ImGui integration (`tests/test_imgui.cpp`)
- Packaging script correctness (`tests/test_packaging.py`)
- Asset processor validation (`tests/test_asset_processor.py`)

Run tests:
```bash
pytest scripts/tests/tools/
ctest --preset linux-gcc-debug -R tools  # When enabled
cmake --build --preset linux-gcc-debug --target test_tools_module  # build smoke binary
pytest scripts/tests/test_editor_smoke.py
```

Set `TOOLS_EDITOR_SMOKE_PRESET` to target a non-default CMake preset or
`TOOLS_EDITOR_SMOKE_BINARY` to point directly at the compiled
`test_tools_module` executable.

## Dependencies

- **Core**: Telemetry schema, diagnostics data structures
- **Runtime**: Diagnostics API, C bindings for tooling
- **Python 3.12+**: For CLI tools and automation scripts
- **Dear ImGui**: Debug UI framework
- **All engine modules**: For comprehensive diagnostics coverage

## Related Documentation

- [`../../ROADMAP.md`](../../ROADMAP.md): Tools module status in roadmap
- [`../../design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md): Telemetry metric definitions
- [`../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](../../design/TELEMETRY_INSTRUMENTATION_GUIDE.md): How to add telemetry
- [`../runtime/DIAGNOSTICS.md`](../runtime/DIAGNOSTICS.md): Runtime diagnostics reference
- [`../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md): Panel registry and runtime loop integration guidance.
- Python tooling: `python/README.md`, `scripts/README.md`

## Current Status

The tools module is guarded by the `ENGINE_ENABLE_TOOLS` CMake cache entry. Repository presets (`linux-gcc-*`, `windows-msvc-*`) now set the flag to `ON`, restoring the library, tests, and Dear ImGui helpers in default developer builds. Consumers that require slimmer builds (for example, CI images without editor dependencies) can pass `-DENGINE_ENABLE_TOOLS=OFF` to disable the module until TL-310 finishes re-integrating the editor harness.

## Current State

- Editor/profiling/pipeline automation staging area with the telemetry viewer CLI surfacing runtime snapshots—including recent
  asset reload failures with per-asset hints—Dear ImGui integration for diagnostics UI, and profiler utilities; runtime packaging
  script available for CI.
- Shared `PanelRegistry` (`engine/tools/imgui/panel_registry.hpp`) centralises Dear ImGui panels with deterministic ordering so
  the runtime, sandbox UI, and future editor reuse diagnostics/presentation surfaces without duplicating layout code. RAII
  registration handles automatically unregister panels when editor modules tear down, and C++ tests exercise registration,
  ordering, and invocation semantics until the editor build is re-enabled (`TL-310`).
- `engine::tools::editor::RuntimePanelBridge` registers runtime diagnostics, profiler, scene validation, and asset browser
  panels with the shared registry and exposes a single `render_all()` entry point for the editor harness. Consumers provide
  cache adapters through `AssetPanelHooks` so the bridge refreshes asset metadata every frame while respecting feature toggles.
- `engine::tools::editor::PerformanceMetricsPanel` captures frame-time history, aggregates runtime stage timings, surfaces
  profiler zones, renders GPU pass timings sourced from `RuntimeDiagnostics::gpu_pass_timings`, and compares benchmark baselines
  so PM-510 demos document regressions directly from the editor UI. The bridge wires the panel automatically and exposes
  `PerformancePanelHooks` for benchmark deltas/history tuning.
- `engine::tools::editor::TelemetryVisualizationPanel` streams runtime telemetry series directly into Dear ImGui, preserves a
  rolling history, and highlights warning/critical thresholds so demos can snapshot health without leaving the editor. Hook it
  up through `RuntimePanelBridge::TelemetryPanelHooks` by passing a `TelemetryPanelHooks` instance to the
  `RuntimePanelBridge` constructor so diagnostics wiring stays consistent with other panels.
- `engine::tools::editor::SceneHierarchyPanel` visualises the runtime `scene::Scene` graph, lazily expands entity trees, and
  surfaces hierarchy validation issues inline so tooling teams can triage structure problems without leaving the editor.
- `engine::tools::editor::AssetBrowserPanel` enumerates loaded asset caches through the new introspection helpers, applies
  deterministic sorting and text filtering, and renders metadata via Dear ImGui tables with list clipping so large datasets stay
  interactive.
- `scripts/tests/test_editor_smoke.py` runs the compiled `test_tools_module` binary with GoogleTest filters to validate the
  sandbox configuration loader, panel registry, and runtime panel bridge in a headless smoke scenario.

## Usage

- Python CLI tools live under `scripts/diagnostics/` and `scripts/tools/`.
- Run tools tests:
  - `pytest scripts/tests/`
  - `ctest --preset linux-gcc-debug -R tools` (when C++ tools are enabled)
- Review the
  [`AI-004 Prototyping Playbook`](../../design/AI_004_PROTOTYPING_PLAYBOOK.md)
  for the cross-module workflow that governs schema validation, dataset
  selection, sandbox integration, and benchmarking hand-offs.
- Sandbox UI and prototyping harness consumers should adopt the shared AI-004
  configuration schema during migration:
  1. Validate manifests with `python -m scripts.validate_ai004_config` before
     loading them in tooling prototypes.
  2. When iterating locally, pass `--require-schema` to
     `python -m scripts.prototyping.run_prototype_harness` (or export
     `ENGINE_AI004_SCHEMA_V1=1`) so the Python harness fails fast on missing
     schema blocks.
  3. Use the repository-provided sample configuration at
     `docs/examples/ai004_sample.json` for quick sandbox smoke tests; it
     references the remeshing dataset under `assets/datasets/remesh_sample`.
  4. Use `--describe-json <path>` to export dataset/rendering/runtime metadata for the sandbox UI and
     `--summary-json <path>` to persist headless execution summaries that feed benchmark capture.
  5. Update sandbox layout persistence to record schema IDs once the UI begins
     emitting AI-004 configuration fragments.
- Register reusable diagnostics panels through the C++ panel registry so the sandbox and future editor surfaces share widgets:
  ```cpp
  engine::tools::imgui::PanelRegistry registry;
  auto streaming_panel = registry.register_scoped_panel(
      "telemetry.streaming",
      [](const engine::tools::imgui::PanelRenderContext& ctx)
      {
          render_streaming_panel(ctx);
      }
  );
  registry.render_all(engine::tools::imgui::PanelRenderContext{delta_time});
  ```
  Panels execute in registration order and can be invoked individually via `registry.render("panel.id", ctx)` when UI flows
  require targeted composition.

- Bridge runtime telemetry directly into the registry when the editor harness starts:
  ```cpp
  engine::tools::imgui::PanelRegistry registry;
  entt::entity selected_entity = entt::null;
  engine::tools::editor::RuntimePanelBridge::PerformancePanelHooks perf_hooks{};
  perf_hooks.history_capacity = 240;
  perf_hooks.benchmark_provider = []() {
      return std::vector<engine::tools::editor::PerformanceMetricsPanel::BenchmarkEntry>{};
  };
  engine::tools::editor::RuntimePanelBridge::TelemetryPanelHooks telemetry_hooks{};
  // Retain the default 180-sample history; increase this window when demos require longer trends.
  telemetry_hooks.history_capacity = 180;
  telemetry_hooks.series_provider = [](const engine::runtime::RuntimeDiagnostics& diagnostics) {
      std::vector<engine::tools::editor::TelemetryVisualizationPanel::SeriesSample> samples;
      samples.reserve(diagnostics.stage_timings.size());
      for (const auto& timing : diagnostics.stage_timings)
      {
          engine::tools::editor::TelemetryVisualizationPanel::SeriesSample entry{};
          entry.identifier = timing.name;
          entry.label = timing.name;
          entry.value = timing.last_ms;
          entry.warning_threshold = 8.0;
          entry.critical_threshold = 12.0;
          entry.unit = "ms";
          samples.push_back(std::move(entry));
      }
      return samples;
  };

  engine::tools::editor::RuntimePanelBridge bridge(
      registry,
      [&runtime]() -> const engine::runtime::RuntimeDiagnostics& { return runtime.diagnostics(); },
      [&runtime]() -> const engine::scene::validation::HierarchyValidationReport* {
          return &runtime.diagnostics().scene_validation;
      },
      engine::tools::editor::RuntimePanelBridge::Renderers{},
      engine::tools::editor::RuntimePanelBridge::HierarchyPanelHooks{
          [&runtime]() -> engine::scene::Scene* { return &runtime.scene(); },
          [&]() -> entt::entity { return selected_entity; },
          [&](entt::entity entity_id) { selected_entity = entity_id; },
      },
      engine::tools::editor::RuntimePanelBridge::AssetPanelHooks{},
      perf_hooks,
      telemetry_hooks
  );

  // In the editor loop
  bridge.render_all(delta_time_seconds);
  ```

- The scene hierarchy panel registers under the identifier `editor.scene_hierarchy`; use `PanelRegistry::render("editor.scene_hierarchy", ctx)` to render it selectively or rely on `render_all()` to include it with diagnostics and profiler panels.

## TODO / Next Steps

- Maintain sandbox UI health: expand golden screenshot coverage and capture layout regression tests ahead of upcoming reviews; record scheduling in [`../../ROADMAP.md`](../../ROADMAP.md).
- Continue iterating on diagnostics viewer and telemetry exports to feed comparative benchmarking (`CC-310`) without duplicating charting pipelines.
- Track follow-up automation and accessibility workstreams alongside runtime/rendering dependencies in [`../../ROADMAP.md`](../../ROADMAP.md).
