# Telemetry Schema (`CC-001.1`)

_Last Updated: 2025-02-28_

## Motivation

Roadmap item `CC-001.1` requires a canonical schema for runtime telemetry so
modules, tooling, and diagnostics viewers exchange metrics without duplicating
structure-specific adapters. This document specifies the core telemetry data
model implemented in `engine/core/telemetry/schema.hpp` and consumed by the
runtime diagnostics pipeline.

The schema must satisfy the following goals:

- Represent counters, gauges, and histogram aggregations emitted by engine
  subsystems.
- Preserve units and human-readable descriptions so diagnostics tooling can
  render values with minimal configuration.
- Attach optional labels (key/value pairs) to metrics without sacrificing
  determinism or lookup speed.
- Transport values as either integers or floating-point numbers while making
  their type explicit to downstream consumers.
- Remain header-only for the public interface with a tiny implementation
  dependency (`schema.cpp`) to avoid expanding the `engine::core` surface area.

## Data Model

All telemetry payloads are encoded as a `MetricSet` containing descriptor and
sample arrays of equal length:

```cpp
struct MetricDescriptor {
    std::string name;                // Stable, dotted identifier
    MetricKind kind;                 // Counter, gauge, histogram
    MetricUnit unit;                 // Count, milliseconds, bytes, ...
    std::string description;         // Human-readable summary
    std::vector<Label> labels;       // Optional key/value metadata
};

struct MetricSample {
    std::size_t descriptor_index;    // Index into MetricSet::descriptors
    MetricValue value;               // std::variant<std::int64_t, double>
};

struct MetricSet {
    std::vector<MetricDescriptor> descriptors;
    std::vector<MetricSample> samples;
};
```

### Kinds and Units

`MetricKind` enumerates semantic categories:

| Kind        | Description                                 |
| ----------- | ------------------------------------------- |
| `Counter`   | Monotonic integer value (e.g., total ticks). |
| `Gauge`     | Instantaneous measurement (e.g., last frame duration). |
| `Histogram` | Aggregated distribution bucket. Histograms are defined but not yet populated by the runtime. |

`MetricUnit` conveys expected presentation units:

| Unit             | Interpretation            |
| ---------------- | ------------------------- |
| `None`           | Unitless value            |
| `Count`          | Integer count             |
| `Milliseconds`   | Time in milliseconds      |
| `Seconds`        | Time in seconds           |
| `Bytes`          | Memory size               |
| `Percentage`     | Ratio scaled to 0–100     |

Utilities defined in `schema.cpp` provide string conversions and helpers to
query whether a `MetricValue` holds an integer or floating-point quantity.

### Labels

Labels are optional key/value pairs that scope a metric to an entity. Examples:

- `{"stage": "physics.integrate"}` for dispatcher timings.
- `{"subsystem": "assets"}` for subsystem lifecycle metrics.
- `{"type": "cycle"}` for hierarchy validation issue counts.

Label vectors are deterministic and maintained in insertion order. Downstream
consumers should treat labels as a small map keyed by `Label::key`.

## Runtime Integration

`RuntimeDiagnostics` now embeds a `MetricSet` populated by
`RuntimeHost::Impl::rebuild_metric_snapshot()`. Metrics cover lifecycle
statistics, streaming health, dispatcher stages, subsystem execution, and
scene-validation outcomes. Each update rebuilds the snapshot to prevent stale
entries when stages or subsystems disappear.

When the rendering module is enabled, the snapshot also emits research baseline
metrics aligned with task `RE-610`:

- `rendering.research.shading_mode.selection` (counter, labels: `mode`) — number
  of times the preset was configured for each shading variant.
- `rendering.research.shading_mode.active` (gauge, unit None) — currently active
  shading mode (0 = forward, 1 = deferred).
- `rendering.research.pass.draw_calls_total` (counter, labels: `pass`, `phase`) —
  cumulative draw calls per research pass.
- `rendering.research.pass.last_draw_calls` (gauge, unit Count, labels: `pass`,
  `phase`) — draw calls observed during the most recent pass execution.
- `rendering.research.pass.last_gpu_time_ms` (gauge, unit Milliseconds, labels:
  `pass`, `phase`) — most recent GPU execution time per pass.
- `rendering.research.pass.max_gpu_time_ms` (gauge, unit Milliseconds, labels:
  `pass`, `phase`) — maximum recorded GPU execution time per pass since the
  telemetry state was reset.

The C API exposes descriptors and samples through the following functions
(`engine/runtime/api.hpp`):

- `engine_runtime_diagnostic_metric_count`
- `engine_runtime_diagnostic_metric_name`
- `engine_runtime_diagnostic_metric_kind`
- `engine_runtime_diagnostic_metric_unit`
- `engine_runtime_diagnostic_metric_description`
- `engine_runtime_diagnostic_metric_label_count`
- `engine_runtime_diagnostic_metric_label_key`
- `engine_runtime_diagnostic_metric_label_value`
- `engine_runtime_diagnostic_metric_is_integral`
- `engine_runtime_diagnostic_metric_value`
- `engine_runtime_diagnostic_metric_value_int`

Python tooling in `scripts/diagnostics/runtime_frame_telemetry.py` consumes
these endpoints to mirror the schema into JSON and console summaries.

## Usage Guidance

- Prefer descriptive, dotted metric names (e.g., `runtime.streaming.total_failed`).
- Keep labels sparse. Use them to disambiguate entities rather than encode the
  entire hierarchy into the metric name.
- Emit counters as monotonic `std::uint64_t` values; the runtime clamps to
  `std::int64_t::max()` before exposing them.
- Treat gauges as floating-point values even when representing integer counts
  that fluctuate.
- Populate `MetricDescriptor::description` with actionable text—scripts surface
  descriptions in help output and dashboards.

## Future Work

- Extend `MetricKind::Histogram` with bucket definitions for frame-time
  distributions.
- Wire additional modules (assets, IO, physics) to emit metrics using the
  shared schema once their diagnostics pipelines adopt the telemetry bridge.
- Persist telemetry snapshots to structured logs for offline analysis.

