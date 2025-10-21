# Telemetry Instrumentation Guide (`CC-001.3`)

_Last updated: 2025-03-22_

This guide explains how Test Engine subsystems instrument metrics using the
shared telemetry schema delivered in `CC-001`. It targets engineers wiring new
counters or gauges into the runtime diagnostics bridge, Python tooling, or
external dashboards.

## Scope and Goals

- Provide a repeatable workflow for emitting metrics from C++ modules using
  `engine::core::telemetry::MetricSet`.
- Describe how module-level counters feed the `RuntimeDiagnostics` snapshot so
  tooling can render a coherent view of engine health.
- Capture naming, units, and labelling conventions enforced across modules.
- Document validation commands for telemetry-heavy changes.

This document complements the schema reference in
[`telemetry_schema.md`](telemetry_schema.md) and replaces ad-hoc notes in task
files.

## Prerequisites

Before adding instrumentation:

1. Review the schema in [`engine/core/telemetry/schema.hpp`](../../engine/core/include/engine/core/telemetry/schema.hpp)
   to understand descriptor and sample structures.
2. Audit the owning module README/roadmap for open telemetry tasks (for example,
   `docs/modules/assets/README.md#execution-checklist`).
3. Confirm the runtime surface you intend to expose metrics through. Most
   modules contribute via `RuntimeDiagnostics::rebuild_metric_snapshot()` in
   [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp).

## Instrumentation Workflow

Follow these steps whenever you introduce new metrics.

### 1. Capture Metrics Close to the Source

Store raw counters inside the subsystem that owns the behaviour. For example,
`engine::assets::AssetStreamingTelemetry` increments atomics on each state
transition:

```cpp
void AssetStreamingTelemetry::on_transition(AssetLoadState from, AssetLoadState to)
{
    if (from == to)
    {
        return;
    }

    decrement_state(from);
    increment_state(to);

    switch (to)
    {
    case AssetLoadState::Ready:
        total_completed_.fetch_add(1, std::memory_order_relaxed);
        break;
    case AssetLoadState::Failed:
        total_failed_.fetch_add(1, std::memory_order_relaxed);
        break;
    case AssetLoadState::Cancelled:
        total_cancelled_.fetch_add(1, std::memory_order_relaxed);
        break;
    default:
        break;
    }
}
```

Keep these data structures lock-free where possible and add unit tests covering
counter transitions (see `engine/assets/tests/test_async.cpp`).

### 2. Convert Module State into `MetricSet`

The runtime snapshot rebuilds a fresh `MetricSet` on each diagnostics poll. Use
helper lambdas to minimise duplication when populating descriptors and samples:

```cpp
const auto add_counter = [&](std::string_view name,
                             std::string_view description,
                             std::int64_t value,
                             std::vector<core::telemetry::Label> labels = {}) {
    const std::size_t index = snapshot.descriptors.size();
    core::telemetry::MetricDescriptor descriptor{};
    descriptor.name.assign(name);
    descriptor.kind = core::telemetry::MetricKind::Counter;
    descriptor.unit = core::telemetry::MetricUnit::Count;
    descriptor.description.assign(description);
    descriptor.labels = std::move(labels);
    snapshot.descriptors.push_back(std::move(descriptor));

    core::telemetry::MetricSample sample{};
    sample.descriptor_index = index;
    sample.value = value;
    snapshot.samples.push_back(std::move(sample));
};
```

Guidelines:

- Rebuild the snapshot atomically: clear descriptors/samples and repopulate them
  each tick to avoid stale entries when entities disappear.
- Clamp unsigned counters to `std::int64_t::max()` before storing them inside the
  variant (see the `clamp_to_int` helper in the runtime implementation).
- Reserve descriptor/sample storage to avoid reallocations when the metric set is
  stable between frames.

### 3. Name Metrics Deterministically

Use dotted paths scoped to the subsystem (`runtime.lifecycle.max_tick_ms`).
Prefer:

- `runtime.<subsystem>.<noun>_<metric>` for runtime-owned metrics.
- `<module>.<component>.<noun>` for other modules emitting via diagnostics.

Attach labels sparingly via `MetricDescriptor::labels`. Deterministic label
vectors keep the viewer and downstream dashboards stable. Example label usage:

```cpp
auto labels = make_single_label("stage", subsystem.name);
add_gauge("runtime.dispatcher.stage_ms",
          "Last execution duration for a runtime stage",
          stage.last_duration_ms,
          core::telemetry::MetricUnit::Milliseconds,
          std::move(labels));
```

### 4. Surface Metrics to Tooling

The runtime C API exposes descriptors and samples for the diagnostics tooling.
`python scripts/diagnostics/runtime_frame_telemetry.py` can dump metrics as JSON
or render a textual summary. To inspect new metrics locally:

```bash
python scripts/diagnostics/runtime_frame_telemetry.py \
    --library-dir <build/preset> \
    --metric-prefix runtime.streaming
```

Pass `--metric-prefix` to focus on a subsystem. Integrations embedding telemetry
into other dashboards should reuse the same C API functions documented in
`engine/runtime/api.hpp`.

### 5. Document the Behaviour

Update the owning module README with a short paragraph describing the new
metrics, how to interpret them, and where they surface inside diagnostics.
Link back to this guide so future contributors follow the same conventions.

## Validation Checklist

Run these commands after updating instrumentation:

- `cmake --build --preset <preset>` for the affected target(s).
- `ctest --preset <preset> --tests-regex <module>` to exercise telemetry unit
  tests.
- `python scripts/validate_docs.py` to ensure this guide and linked READMEs stay
  lint-free.
- Optional: execute the telemetry viewer script against a debug build to verify
  new metrics appear as expected.

Record outcomes in the PR summary to keep CI reproducible.

## Troubleshooting

| Symptom | Diagnostic Steps | Fix |
| --- | --- | --- |
| Metric missing in viewer | Confirm the descriptor name matches the expected prefix and that the snapshot reserves enough space for descriptors/samples. | Rebuild the metric snapshot after registering new descriptors; ensure lambdas push descriptors before samples. |
| Counter never increments | Add unit tests for the underlying telemetry struct and verify all state transitions call `on_transition`. | Ensure instrumentation hooks execute on every code path; add logging when counters saturate. |
| Viewer mislabels metrics | Labels must be inserted in deterministic order. Rebuild label vectors using helper utilities (e.g., `make_single_label`). | Stabilise label construction and avoid using unordered containers to store label key/value pairs. |

## References

- [`telemetry_schema.md`](telemetry_schema.md) — canonical data model.
- [`design/architecture_improvement_plan.md`](architecture_improvement_plan.md) — initiative context for `CC-001`.
- [`scripts/diagnostics/runtime_frame_telemetry.py`](../../scripts/diagnostics/runtime_frame_telemetry.py) — CLI viewer and regression tests.
- [`docs/modules/tools/README.md`](../modules/tools/README.md) — diagnostics tooling overview.
