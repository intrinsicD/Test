# Runtime Diagnostics Guide

## Purpose
`RuntimeDiagnostics` exposes lifecycle, scheduling, and streaming telemetry for
`RuntimeHost`. This guide fulfils roadmap item `RU-320` by consolidating
instrumentation expectations and troubleshooting flows for teams consuming the
runtime loop. It complements the async streaming design note and captures the
scene hierarchy validation workflows delivered in `RT-005.2`/`RT-005.3`.

All metrics adhere to the shared schema documented in
[`docs/design/TELEMETRY_SCHEMA.md`](../../design/TELEMETRY_SCHEMA.md). Consumers
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
invocations. `initialize_failure_count` records how many initialization attempts
failed across the process lifetime. `last_*_ms`, `max_*_ms`, and
`average_tick_ms` capture rolling wall-clock durations measured with the
runtime's steady clock.

The runtime now logs initialization failures with structured key/value output
(`runtime.lifecycle.initialize_failure ...`) and surfaces the most recent event
via `RuntimeDiagnostics::last_initialize_failure`. Tooling can read the stored
`runtime`, `subsystem`, `category`, `message`, and `duration_ms` fields or query
the `runtime.lifecycle.initialize.failures` counter exposed in
`RuntimeDiagnostics::metrics` to correlate failures with lifecycle activity.

### Initialization Failure Triage

- `initialize_failure_count` and per-subsystem `initialize_failure_count`
  counters expose the lifetime failure totals for the process and each plugin.
  Use these to determine whether repeated startup issues are isolated or systemic.
- `runtime_frame_telemetry.py` now records the full failure payload in the JSON
  export. The telemetry viewer prints an **Initialization Failures** section that
  summarizes the total failure count, the most recent subsystem/runtime that
  failed, and per-subsystem diagnostics (including the last failure category,
  duration, and message).
- Operators should triage failures by capturing a telemetry snapshot, reviewing
  the viewer output, and then following the logging guidance below. The viewer
  points directly to `runtime.lifecycle.initialize_failure` log entries so the
  structured log stream and telemetry remain aligned.
- When failures persist, consult the troubleshooting checklist in this document
  to validate configuration, subsystem ordering, and dependency availability.
  Escalate recurring issues into the Core module backlog to extend automation or
  additional diagnostics if manual remediation becomes repetitive.

### Dependency Cycle Diagnostics
`RuntimeHostDependencies` validation emits
`RuntimeError::dependency_cycle` when explicit subsystem selections produce a
cycle. The exception message includes the offending path (for example,
`assets -> runtime -> assets`) so operators can trace the configuration loop.
When telemetry is available, filter metrics with
`--metric-prefix runtime.lifecycle.` in
`scripts/diagnostics/runtime_frame_telemetry.py` to correlate the failure with
recent subsystem startup durations.

### Stage Timings
Each entry in `stage_timings` represents a named phase inside the runtime
(controllers, deformation, physics, submission). The runtime reuses entries
across frames and updates `last_ms`, `average_ms`, `max_ms`, and
`sample_count` deterministically.

### Subsystem Timings
`subsystem_timings` aggregates initialise/tick/shutdown durations for every
registered subsystem plugin. Entries persist while the subsystem stays loaded so
long-running plugins can be analysed over time. Each entry also tracks
`initialize_failure_count`, `last_initialize_failure_category`,
`last_initialize_failure_message`, and `last_initialize_failure_ms` so operators
can reconcile structured logs with per-subsystem telemetry.

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
- Error attribution: `streaming_geometry_failures` (C++) / `geometry_failures_by_error` (C ABI) enumerate failures by
  `GeometryIoErrorCode` and drive the `runtime.streaming.geometry_failures` telemetry counters.

These values feed the async streaming diagnostics described in
[`docs/design/ASYNC_STREAMING.md`](../../design/ASYNC_STREAMING.md), the shared
schema reference, and the associated task record
[`T-0115`](../../archive/backlog/legacy/tasks/T-0115-assets-async-streaming-mvp.md).

### Animation Telemetry
`RuntimeDiagnostics::animation` exposes metadata about the currently evaluated
clip and aggregates from the animation dispatcher. The struct records:

- `clip_track_count` / `pose_joint_count` — structural data for the active clip
  and pose to confirm controller/rig parity.
- `clip_duration`, `playback_time`, `playback_speed` — playback state sampled
  from the controller to detect sync issues.
- `category_totals` — per-category dispatch totals produced by the benchmarking
  helpers (for example, sampling vs skinning), reported in milliseconds.
- `queue_totals` — cumulative dispatcher cost grouped by queue label (`cpu`,
  `gpu`, `unknown`).

Metrics are exported into the shared schema as `runtime.animation.*` gauges so
dashboards can chart playback and dispatcher trends alongside other runtime
signals. The C ABI mirrors the data via
`engine_runtime_diagnostic_animation_*` accessors. Python tooling
(`runtime_frame_telemetry.py`) consumes these helpers to persist clip metadata
and queue/category aggregates inside the JSON snapshot and console summary.

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

Hierarchy validation metrics now include alert-oriented counters so dashboards
can identify sustained regressions:

- `runtime.scene_validation.consecutive_failure_frames` — consecutive frames
  that reported hierarchy errors.
- `runtime.scene_validation.max_consecutive_failure_frames` — longest streak
  observed since initialisation for historical comparison.
- `runtime.scene_validation.failure_frame_count` — total frames containing
  hierarchy issues.
- `runtime.scene_validation.last_failure_simulation_time` and
  `runtime.scene_validation.last_failure_wall_seconds` — timestamps (simulation
  seconds and wall-clock seconds since `RuntimeHost::initialize`) for the most
  recent failure. Values of `-1` indicate that no failure has occurred.
- `runtime.scene_validation.alert_threshold.warning_frames` and
  `runtime.scene_validation.alert_threshold.critical_frames` — documented alert
  thresholds (3 and 10 consecutive frames respectively) consumed by tooling and
  operators.
- `runtime.scene_validation.alert_level` — derived alert state
  (`0 = none`, `1 = warning`, `2 = critical`).

Dashboards should trigger an informational alert when
`runtime.scene_validation.alert_level >= 1` (3 consecutive failing frames) and
escalate to paging when the value reaches `2` (10 consecutive failing frames).
These defaults keep short-lived authoring mistakes from paging operators while
still surfacing persistent regressions quickly.

Scene authors should review the
[scene diagnostics guide](../scene/DIAGNOSTICS.md) for module-specific
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
  alerts when the failure rate exceeds acceptable thresholds. The
  `runtime.scene_validation.alert_level` metric mirrors the recommended warning
  (`>= 1`) and paging (`>= 2`) breakpoints for dashboards.
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
- **Initialization failures** – capture telemetry with
  `runtime_frame_telemetry.py`, review the **Initialization Failures** section in
  the telemetry viewer to identify the subsystem/category involved, and inspect
  matching `runtime.lifecycle.initialize_failure` logs for context before
  reconfiguring plugins or dependencies.
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
