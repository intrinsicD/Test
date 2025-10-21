# Runtime Diagnostics Guide

## Purpose
`RuntimeDiagnostics` exposes lifecycle, scheduling, and streaming telemetry for
`RuntimeHost`. This guide fulfils roadmap item `RU-320` by consolidating
instrumentation expectations and troubleshooting flows for teams consuming the
runtime loop. It complements the async streaming design note and captures the
scene hierarchy validation workflows delivered in `RT-005.2`/`RT-005.3`.

All metrics adhere to the shared schema documented in
[`docs/design/telemetry_schema.md`](../../design/telemetry_schema.md). Consumers
should prefer the schema APIs over bespoke structs to remain compatible with
future tooling such as the diagnostics viewer (`CC-001`).

## Access Patterns

### C++ API
Call `RuntimeHost::diagnostics()` after initialisation to inspect the most
recent metrics snapshot. The reference remains valid until `RuntimeHost` is
moved.

```cpp
engine::runtime::RuntimeHost host{make_runtime_dependencies()};
host.initialize();
const auto& diagnostics = host.diagnostics();
for (const auto& timing : diagnostics.stage_timings)
{
    fmt::print("Stage {} took {:.3f} ms\n", timing.name, timing.last_ms);
}
```

### C ABI and Tooling Integration
The global `engine::runtime::diagnostics()` accessor mirrors the C++ API for the
Python tooling under `scripts/diagnostics/`. Run the helpers below against a
shared library build to capture structured telemetry:

- `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>` –
  records lifecycle timings, streaming metrics, and (when rendering is enabled)
  frame-graph metadata snapshots.
- `python scripts/diagnostics/streaming_report.py --library-dir <build>` –
  focuses on async queue health sourced from `StreamingMetrics`.

### Configuration Dependencies

- The async streaming counters populate when
  `RuntimeHostDependencies::streaming_config.enable` is `true` and the assets
  module has registered async caches (see `T-0115`).
- Rendering metadata is available only when the runtime is built with rendering
  support (`ENGINE_ENABLE_RENDERING=ON`).
- Scene validation metrics are emitted after `RuntimeHost::tick()` updates the
  scene graph and invokes `scene::validation::validate_hierarchy`.

## Metrics Reference

### Lifecycle Counters
`initialize_count`, `tick_count`, and `shutdown_count` track lifecycle
invocations. `last_*_ms`, `max_*_ms`, and `average_tick_ms` capture rolling
wall-clock durations measured with the runtime's steady clock.

### Stage Timings
Each entry in `stage_timings` represents a named phase inside the runtime
(controllers, deformation, physics, submission). The runtime reuses entries
across frames and updates `last_ms`, `average_ms`, `max_ms`, and
`sample_count` deterministically.

### Subsystem Timings
`subsystem_timings` aggregates initialise/tick/shutdown durations for every
registered subsystem plugin. Entries persist while the subsystem stays loaded so
long-running plugins can be analysed over time.

### Streaming Metrics
`StreamingMetrics` mirrors `engine::core::threading::IoThreadPool` state and the
asset streaming caches. Runtime diagnostics also encode these counters inside
`RuntimeDiagnostics::metrics` with `MetricDescriptor` entries named
`runtime.streaming.*` so downstream tooling can extract them using the shared
schema.

- `worker_count`, `queue_capacity`, `pending_tasks`, `active_workers`
- Cumulative counters: `total_enqueued`, `total_executed`
- Cache state totals: `streaming_pending`, `streaming_loading`
- Request outcomes: `streaming_total_completed`, `streaming_total_failed`,
  `streaming_total_cancelled`, `streaming_total_rejected`

These values feed the async streaming diagnostics described in
[`docs/design/async_streaming.md`](../../design/async_streaming.md), the shared
schema reference, and the associated task record
[`T-0115`](../../tasks/T-0115-assets-async-streaming-mvp.md).

### Handle Validation
`handle_validation` captures a snapshot from the asset handle validator
registry. Each entry reports the handle type, total successes/failures, and the
most recent failure context/reason. Metrics are also emitted into
`RuntimeDiagnostics::metrics` as `runtime.handles.<type>.{success,failure}`
counters. Call `engine::assets::validate_handle` before dereferencing handles to
populate these diagnostics and surface stale-handle bugs quickly.

### Scene Validation
`scene_validation` embeds the latest
`scene::validation::HierarchyValidationReport`, including `metrics` (issue
counts) and detailed `issues`. Use `report.ok()` to detect whether hierarchy
invariants hold before submitting to rendering (`RT-005`). When issues are
present the diagnostics bridge emits structured log entries, forwards the
payload to registered callbacks, and updates the runtime telemetry so scripting
environments receive identical context. The runtime C ABI exposes
`engine_runtime_diagnostic_scene_*` helpers; `runtime_frame_telemetry.py`
serialises the hierarchy report in its JSON output and console summary,
surfacing entity IDs, relationship context, and error messages for up to five
issues per invocation.

Scene authors should review the
[scene diagnostics guide](../scene/diagnostics.md) for module-specific
remediation steps and planned follow-up samples introduced by `SC-220`.

## Hierarchy Diagnostics Playbook

Follow this playbook whenever `scene_validation.ok()` returns `false`.

1. **Capture the latest report** — obtain the reference from
   `RuntimeHost::diagnostics().scene_validation` (C++) or
   `engine_runtime_diagnostic_scene_*` (C ABI) immediately after
   `RuntimeHost::tick()`.
2. **Persist structured evidence** — run
   `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>`
   to emit JSON + console snapshots. These include the hierarchy report,
   streaming counters, and stage timings so downstream tooling correlates
   failures with frame workload.
3. **Triaging with callbacks** — register a listener with
   `RuntimeDiagnosticsBridge::register_listener` to mirror reports into custom
   tooling (e.g., GUI overlays or editor integrations). Callbacks execute on the
   runtime thread immediately after validation and must avoid blocking.
4. **Escalate persistent failures** — if identical issues span multiple ticks,
   capture two consecutive frames and compare `issues[i].context` to detect
   whether authoring data or runtime mutation triggered the regression.

### Issue Reference

| Issue Type | Symptom | Recommended Mitigation |
| --- | --- | --- |
| `DuplicateRoot` | More than one entity lacks a parent. | Restrict importers to produce a single root and re-parent detached entities explicitly. |
| `MissingParent` | Child references a non-existent parent entity. | Stabilise parent IDs in asset pipelines and refresh handles before mutating transforms. |
| `CycleDetected` | Hierarchy contains a cycle preventing topological traversal. | Audit recent re-parenting logic and rely on `scene::HierarchyEditor::set_parent` safeguards. |
| `InvalidTransform` | Transform propagation failed (e.g., NaNs, zero scale). | Clamp authoring data, reset affected nodes, and inspect physics/animation systems injecting invalid transforms. |

Augment the table with module-specific issue codes as validation expands. The
bridge forwards the enum value and human-readable `message` so tooling can map
issues to remediation guides.

### CLI Workflow

1. Build the runtime with diagnostics enabled (default).
2. Launch `runtime_frame_telemetry.py` against the shared library build.
3. Observe the console for a `Scene validation failed` banner followed by the
   top issues. The script also writes JSON to the working directory when
   `--output` is specified.
4. Feed the JSON into dashboards to detect recurring issue types across scenes
   or commits.

### Logging and Alerting

- The diagnostics bridge emits `engine.runtime.diagnostics.scene` logs with the
  issue summary. Integrate the logging sink with existing monitoring to raise
  alerts when the failure rate exceeds acceptable thresholds.
- Tooling consuming the bridge should debounce notifications to avoid flooding
  UI surfaces when the same issue persists across ticks.
- When running in headless CI environments, persist the JSON output as part of
  the artefact bundle to streamline triage.

### Rendering Metadata
When rendering backends are available, `frame_graph_serialization` stores the
latest JSON serialisation of the compiled frame graph and `frame_graph_events`
replays resource lifetime events. Diagnostics scripts surface these payloads to
validate queue affinity and resource hazards (`AI-003`, `RT-003`).

## Troubleshooting Workflows

- **Slow frames or spikes** – inspect `stage_timings` to locate the phase with
  elevated `last_ms`/`max_ms`, then drill into per-subsystem timings.
- **Async backlog growth** – compare `pending_tasks` against `queue_capacity` and
  `streaming_total_rejected`. Increase worker count or investigate cache
  bottlenecks when the queue saturates.
- **Invalid scene hierarchies** – follow the [Hierarchy Diagnostics
  Playbook](#hierarchy-diagnostics-playbook) to capture telemetry, run the
  Python tooling, and reconcile issue categories across frames.
- **Rendering divergence** – diff `frame_graph_serialization` outputs between
  builds to ensure deterministic compilation and match backend expectations.
- **CI regression tracking** – persist JSON output from the diagnostics scripts
  to performance dashboards and alert on sustained drift.

## Related Roadmap Items

- `AI-002` – async streaming telemetry depends on the metrics documented here.
- `RT-005.2` – runtime diagnostics bridge forwards hierarchy reports to tooling.
- `RT-005.3` – hierarchy diagnostics troubleshooting guidance published here and
  referenced by runtime + tooling documentation.
- `CC-001` – diagnostics viewer work consumes the metrics schema captured in this
  guide.
