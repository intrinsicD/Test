# Runtime Diagnostics Guide

## Purpose
`RuntimeDiagnostics` exposes lifecycle, scheduling, and streaming telemetry for
`RuntimeHost`. This guide fulfils roadmap item `RU-320` by consolidating
instrumentation expectations and troubleshooting flows for teams consuming the
runtime loop. It complements the async streaming design note and prepares the
runtime module for upcoming hierarchy validation work (`RT-005`).

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
asset streaming caches:

- `worker_count`, `queue_capacity`, `pending_tasks`, `active_workers`
- Cumulative counters: `total_enqueued`, `total_executed`
- Cache state totals: `streaming_pending`, `streaming_loading`
- Request outcomes: `streaming_total_completed`, `streaming_total_failed`,
  `streaming_total_cancelled`, `streaming_total_rejected`

These values feed the async streaming diagnostics described in
[`docs/design/async_streaming.md`](../../design/async_streaming.md) and the
associated task record [`T-0115`](../../tasks/T-0115-assets-async-streaming-mvp.md).

### Scene Validation
`scene_validation` embeds the latest
`scene::validation::HierarchyValidationReport`, including `metrics` (issue
counts) and detailed `issues`. Use `report.ok()` to detect whether hierarchy
invariants hold before submitting to rendering (`RT-005`).

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
- **Invalid scene hierarchies** – review `scene_validation.issues` for
  `HierarchyIssueType` entries and feed them into the upcoming diagnostics bridge
  (`RT-005.2`).
- **Rendering divergence** – diff `frame_graph_serialization` outputs between
  builds to ensure deterministic compilation and match backend expectations.
- **CI regression tracking** – persist JSON output from the diagnostics scripts
  to performance dashboards and alert on sustained drift.

## Related Roadmap Items

- `AI-002` – async streaming telemetry depends on the metrics documented here.
- `RT-005.2` – runtime diagnostics bridge will forward the hierarchy report to
  tooling consumers.
- `CC-001` – diagnostics viewer work consumes the metrics schema captured in this
  guide.
