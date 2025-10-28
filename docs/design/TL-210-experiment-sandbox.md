# TL-210 Experiment Sandbox Architecture

**Status:** Draft implementation shipped

## Purpose

The experiment sandbox aligns the tools module with the AI-004 prototyping
initiative by providing an ImGui-driven workspace that orchestrates datasets,
rendering presets, telemetry feeds, and benchmark execution. The sandbox is the
interactive counterpart to the headless prototyping harness (`RT-320`), enabling
researchers to iterate without editing configuration files.

## Goals

- Present datasets, rendering presets, and runtime context in a single window.
- Surface telemetry counters (FPS, CPU/GPU frame timing, custom series).
- Persist user preferences and window layout across sessions.
- Provide callbacks so the runtime harness, benchmark automation, and scripting
  layers can react to UI changes deterministically.

## Architecture Overview

```
+----------------------+        +-------------------------+
| Prototype harness    |        | ExperimentSandbox       |
| (python/engine3g)    |        | (engine/tools/sandbox)  |
|                      |        |                         |
|  load_harness(...)   |        |  set_configuration(...) |
|  describe_configuration() --->|  render_dataset_panel() |
|  run_headless(...)   <--- callbacks (dataset, render,   |
|                      |        |  benchmark)             |
+----------------------+        +-------------------------+
```

1. The harness exports a configuration summary via
   `--describe-json`, detailing datasets, rendering presets, runtime
   parameters, and telemetry expectations.
2. `ExperimentSandbox` ingests the summary through
   `set_configuration`, keeping the UI state in sync with runtime data.
3. User interactions fire callbacks so harness callers can apply
   configuration overrides or schedule benchmark runs.
4. Telemetry updates arrive via `update_telemetry`, allowing the sandbox to
   visualise live metrics without owning the sampling loop.

## Key Types

- `ExperimentSandbox` – orchestration class exposing `render()` and persistence
  helpers.
- `ExperimentConfigurationSummary` – in-memory mirror of the harness summary
  containing datasets, rendering presets, and runtime metadata.
- `SandboxPreferences` – captures the mutable UI state (selection, overlays,
  benchmark parameters).
- `TelemetrySnapshot` – lightweight struct containing aggregated timing data
  and arbitrary series for plotting.

### Dataset Browser

Datasets are rendered in a searchable list. The class maintains an internal
lookup map so selection persists even when summaries refresh. Tags, statistics,
metrics, and asset provenance populate the detail column for quick inspection.

### Rendering Controls

Rendering presets expose:

- Shading mode selection (`Forward`, `Deferred`, ...)
- Overlay toggles (normals, UVs, material inspection)
- Default resolution metadata for future viewport wiring

Overlay states merge descriptor defaults with stored preferences during
configuration refreshes to prevent user overrides from being lost.

### Benchmark Panel

Benchmark controls gather frame count and timestep parameters. Pressing the
`Run Benchmark` button invokes the `on_run_benchmark` callback with the current
`SandboxPreferences`, enabling harness integration to dispatch headless runs.

### Telemetry Panel

Telemetry metrics are provided externally and plotted using ImGui's `PlotLines`
primitive. The sandbox does not sample metrics itself; it simply renders the
latest snapshot pushed by the runtime or diagnostics bridge.

## Persistence Strategy

Two persistence layers keep the workspace reproducible:

1. **Preferences file** – `save_preferences`/`load_preferences` serialise
   dataset selection, rendering overrides, and benchmark controls to a simple
   key-value file (`overlay.<key>=0/1`, `selected_preset=...`).
2. **Layout file** – `save_layout`/`load_layout` delegate to ImGui's INI
   persistence so window placement and split proportions survive restarts.

Both helpers create parent directories on demand, allowing tooling to store
state in per-user configuration locations.

## Integration Notes

- Always call `set_configuration` before attempting to load preferences so the
  sandbox can reconcile overlay keys and preset availability.
- Refresh the configuration whenever the harness schema output changes; the
  sandbox merges stored state into the new descriptors without losing user
  selections.
- Telemetry snapshots should clamp vector sizes to avoid excessive UI work;
  the sandbox plots whatever sample series the caller provides.
- When wiring the sandbox into the prototyping harness UI loop, invoke
  `render()` after calling `ImGui::NewFrame()` and before `ImGui::Render()`.

## Next Steps

- Bind the benchmark callback to the comparative benchmarking orchestrator
  (`CC-310`) once the automation surface is ready.
- Extend rendering controls with resolution overrides and preset-specific
  parameter widgets.
- Integrate dataset ingestion tooling (`AS-330`) so missing assets can be
  queued directly from the UI.

