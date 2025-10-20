## PRIORITY_DECISION
- Selected Task: CC-001 — Extend runtime telemetry viewer summaries with prefix filtering
- Score Table:
  | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
  | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
  | CC-001: Metric summary prefix filters | 3 | 4 | 3 | 3 | 4 | 4 | 21 |
  | CC-001: Histogram emission support | 1 | 2 | 1 | 1 | 1 | 3 | 9 |
- Tie-break Rationale: N/A
- Decision Rationale (≤5 bullets)
  - Prefix filtering unlocks practical consumption of the schema introduced in `CC-001.1` without waiting on new metric sources.
  - Enables diagnostics users to inspect lifecycle, stage, and subsystem metrics today, unblocking TL-101 viewer iterations.
  - Low risk to integrate—pure Python change with targeted unit coverage.
  - Keeps observability workstream momentum ahead of histogram data availability.
  - Aligns directly with `CC-001` roadmap goals for tooling readiness.

## DESIGN_BRIEF
Problem Statement: `runtime_frame_telemetry.py` only surfaces streaming metrics despite the runtime now publishing a complete telemetry schema, limiting diagnostics and blocking TL-101 viewer work.
Acceptance Criteria:
- CLI exposes prefix filtering (`--metric-prefix`) and an opt-in flag to emit the entire metric set (`--metrics-all`).
- Summary output prints any schema metric matching the configured prefixes, grouping by namespace and surfacing label metadata.
- Unit tests cover prefix selection, ordering stability, and empty-filter messaging.
- Documentation updated to describe new options.
Interfaces & Data Flow: Extend existing telemetry snapshot consumption path—no C API changes; only Python presentation layer updates.
Invariants: Preserve deterministic ordering (stable label sorting), avoid mutating runtime data structures, and maintain existing summary output for streaming metrics by default.
Compatibility/Migrations: Backwards compatible—the default prefix preserves previous streaming-focused output; additional options are opt-in.
Security/Performance/Edge Cases: No external inputs beyond CLI args; guard against invalid descriptor indices; format floats consistently; ensure empty results handled gracefully.
Test Plan: Extend `scripts/tests/test_runtime_frame_telemetry.py` with unit tests for prefix filtering and summary output messaging; run pytest target.

## PATCH
```diff
@@
-from typing import Dict, Iterable, List, MutableMapping, Optional, Sequence
+from typing import Dict, Iterable, List, MutableMapping, Optional, Sequence, Tuple
@@
 _METRIC_UNIT = {
     0: "none",
     1: "count",
     2: "milliseconds",
     3: "seconds",
     4: "bytes",
     5: "percentage",
 }
+
+_METRIC_UNIT_SUFFIX = {
+    "none": "",
+    "count": "",
+    "milliseconds": " ms",
+    "seconds": " s",
+    "bytes": " bytes",
+    "percentage": " %",
+}
@@
 def _samples_to_dict(
     samples: Sequence[FrameSample], diagnostics: Optional[RuntimeDiagnosticsSnapshot]
 ) -> Dict[str, object]:
@@
-    return {
-        "frames": [
-            {
-                "index": sample.index,
-                "simulation_time": sample.simulation_time,
-                "dt": sample.timestep,
-                "frame_total_ms": sample.frame_total_ms,
-                "category_totals_ms": sample.category_totals_ms,
-                "dispatches": [
-                    {
-                        "name": dispatch.name,
-                        "duration_ms": dispatch.duration_ms,
-                        "category": dispatch.category,
-                    }
-                    for dispatch in sample.dispatches
-                ],
-            }
-            for sample in samples
-        ],
-        "summary": summarise(samples),
-        "runtime_diagnostics": _diagnostics_to_dict(diagnostics)
-        if diagnostics is not None
-        else None,
-    }
+    return {
+        "frames": [
+            {
+                "index": sample.index,
+                "simulation_time": sample.simulation_time,
+                "dt": sample.timestep,
+                "frame_total_ms": sample.frame_total_ms,
+                "category_totals_ms": sample.category_totals_ms,
+                "dispatches": [
+                    {
+                        "name": dispatch.name,
+                        "duration_ms": dispatch.duration_ms,
+                        "category": dispatch.category,
+                    }
+                    for dispatch in sample.dispatches
+                ],
+            }
+            for sample in samples
+        ],
+        "summary": summarise(samples),
+        "runtime_diagnostics": _diagnostics_to_dict(diagnostics)
+        if diagnostics is not None
+        else None,
+    }
+
+
+def _metric_matches_prefix(name: str, prefixes: Optional[Sequence[str]]) -> bool:
+    if prefixes is None:
+        return True
+    return any(name.startswith(prefix) for prefix in prefixes)
+
+
+def _select_metrics(
+    snapshot: RuntimeMetricsSnapshot,
+    prefixes: Optional[Sequence[str]],
+) -> List[Tuple[RuntimeMetricDescriptor, RuntimeMetricSample]]:
+    entries: List[
+        Tuple[RuntimeMetricDescriptor, RuntimeMetricSample, Tuple[Tuple[str, str], ...]]
+    ] = []
+    descriptor_count = len(snapshot.descriptors)
+    for sample in snapshot.samples:
+        index = sample.descriptor_index
+        if index >= descriptor_count:
+            continue
+        descriptor = snapshot.descriptors[index]
+        if not _metric_matches_prefix(descriptor.name, prefixes):
+            continue
+        label_key = tuple(sorted(descriptor.labels.items()))
+        entries.append((descriptor, sample, label_key))
+
+    entries.sort(key=lambda entry: (entry[0].name, entry[2]))
+    return [(descriptor, sample) for descriptor, sample, _ in entries]
+
+
+def _format_metric_value(sample: RuntimeMetricSample) -> str:
+    if sample.is_integral:
+        return f"{sample.int_value}"
+    return f"{sample.value:.4f}"
+
+
+def _format_metric_unit(unit: str) -> str:
+    return _METRIC_UNIT_SUFFIX.get(unit, f" {unit}" if unit else "")
+
+
+def _format_metric_labels(labels: Dict[str, str]) -> str:
+    if not labels:
+        return ""
+    parts = ", ".join(f"{key}={value}" for key, value in sorted(labels.items()))
+    return f" [{parts}]"
+
+
+def _print_metric_summary(
+    snapshot: RuntimeMetricsSnapshot,
+    prefixes: Optional[Sequence[str]],
+) -> None:
+    filtered = _select_metrics(snapshot, prefixes)
+    if prefixes is None:
+        prefix_info = ""
+    else:
+        joined = ", ".join(prefixes)
+        prefix_info = f" (filter: {joined})" if joined else ""
+
+    print(f"  metrics{prefix_info}:")
+    if not filtered:
+        if prefixes is None:
+            print("    (no metrics available)")
+        else:
+            print("    (no metrics matched the provided prefix filters)")
+        return
+
+    current_group: Optional[str] = None
+    for descriptor, sample in filtered:
+        if "." in descriptor.name:
+            group, leaf = descriptor.name.rsplit(".", maxsplit=1)
+        else:
+            group, leaf = "metrics", descriptor.name
+
+        if group != current_group:
+            print(f"    [{group}]")
+            current_group = group
+
+        value = _format_metric_value(sample)
+        unit = _format_metric_unit(descriptor.unit)
+        labels = _format_metric_labels(descriptor.labels)
+        print(f"      {leaf:<32} {value}{unit}{labels}")
@@
 def _print_summary(
     samples: Sequence[FrameSample],
     verbose: bool,
-    diagnostics: Optional[RuntimeDiagnosticsSnapshot],
+    diagnostics: Optional[RuntimeDiagnosticsSnapshot],
+    metric_prefixes: Optional[Sequence[str]],
 ) -> None:
@@
-        if diagnostics.metrics is not None:
-            streaming_entries = [
-                (
-                    descriptor,
-                    diagnostics.metrics.samples[index],
-                )
-                for index, descriptor in enumerate(diagnostics.metrics.descriptors)
-                if descriptor.name.startswith("runtime.streaming.") and not descriptor.labels
-            ]
-            if streaming_entries:
-                print("  streaming metrics:")
-                for descriptor, sample in streaming_entries:
-                    value = sample.int_value if sample.is_integral else sample.value
-                    metric_name = descriptor.name.split(".")[-1]
-                    unit = f" {descriptor.unit}" if descriptor.unit != "none" else ""
-                    print(
-                        "    "
-                        f"{metric_name:<24} "
-                        f"value={value}"
-                        f"{unit}"
-                    )
+        if diagnostics.metrics is not None:
+            _print_metric_summary(diagnostics.metrics, metric_prefixes)
@@
-    _print_summary(samples, args.verbose, diagnostics)
+    if args.metrics_all:
+        metric_prefixes: Optional[Sequence[str]] = None
+    else:
+        prefixes = tuple(args.metric_prefix or ())
+        metric_prefixes = prefixes if prefixes else ("runtime.streaming.",)
+
+    _print_summary(samples, args.verbose, diagnostics, metric_prefixes)
```

## TESTS
```diff
@@
+def _make_metrics_snapshot() -> telemetry.RuntimeMetricsSnapshot:
+    descriptors = [
+        telemetry.RuntimeMetricDescriptor(
+            name="runtime.streaming.total_completed",
+            kind="counter",
+            unit="count",
+            description="",
+            labels={},
+        ),
+        telemetry.RuntimeMetricDescriptor(
+            name="runtime.lifecycle.last_tick_ms",
+            kind="gauge",
+            unit="milliseconds",
+            description="",
+            labels={},
+        ),
+        telemetry.RuntimeMetricDescriptor(
+            name="runtime.stage.last_ms",
+            kind="gauge",
+            unit="milliseconds",
+            description="",
+            labels={"stage": "geometry.deform"},
+        ),
+        telemetry.RuntimeMetricDescriptor(
+            name="runtime.stage.last_ms",
+            kind="gauge",
+            unit="milliseconds",
+            description="",
+            labels={"stage": "animation.evaluate"},
+        ),
+    ]
+    samples = [
+        telemetry.RuntimeMetricSample(
+            descriptor_index=0,
+            is_integral=True,
+            value=10.0,
+            int_value=10,
+        ),
+        telemetry.RuntimeMetricSample(
+            descriptor_index=1,
+            is_integral=False,
+            value=0.5,
+            int_value=0,
+        ),
+        telemetry.RuntimeMetricSample(
+            descriptor_index=2,
+            is_integral=False,
+            value=1.23,
+            int_value=0,
+        ),
+        telemetry.RuntimeMetricSample(
+            descriptor_index=3,
+            is_integral=False,
+            value=2.34,
+            int_value=0,
+        ),
+    ]
+    return telemetry.RuntimeMetricsSnapshot(descriptors=descriptors, samples=samples)
@@
+def test_select_metrics_filters_by_prefix() -> None:
+    snapshot = _make_metrics_snapshot()
+    filtered = telemetry._select_metrics(snapshot, ("runtime.stage.",))
+    assert [descriptor.labels["stage"] for descriptor, _ in filtered] == [
+        "animation.evaluate",
+        "geometry.deform",
+    ]
+
+
+def test_select_metrics_returns_all_without_prefix() -> None:
+    snapshot = _make_metrics_snapshot()
+    filtered = telemetry._select_metrics(snapshot, None)
+    assert [descriptor.name for descriptor, _ in filtered] == [
+        "runtime.lifecycle.last_tick_ms",
+        "runtime.stage.last_ms",
+        "runtime.stage.last_ms",
+        "runtime.streaming.total_completed",
+    ]
+
+
+def test_print_metric_summary_reports_missing_prefix(capsys: pytest.CaptureFixture[str]) -> None:
+    snapshot = _make_metrics_snapshot()
+    telemetry._print_metric_summary(snapshot, ("physics.",))
+    captured = capsys.readouterr().out
+    assert "metrics (filter: physics.)" in captured
+    assert "no metrics matched" in captured
+
+
+def test_print_metric_summary_emits_values(capsys: pytest.CaptureFixture[str]) -> None:
+    snapshot = _make_metrics_snapshot()
+    telemetry._print_metric_summary(snapshot, ("runtime.lifecycle.",))
+    captured = capsys.readouterr().out
+    assert "metrics (filter: runtime.lifecycle.)" in captured
+    assert "[runtime.lifecycle]" in captured
+    assert "last_tick_ms" in captured
```

## DOCS
```diff
@@
-   telemetry schema (`runtime.streaming.*`, lifecycle counters, etc.), surfaces
+   telemetry schema (lifecycle counters, streaming gauges, stage/subsystem
+   samples, etc.), surfaces
@@
-Use `--verbose` to emit per-frame tables on stdout when investigating specific
-regressions. The JSON payload can be checked into performance dashboards or
-post-processed by CI jobs for automated alerts.
+Use `--metric-prefix PREFIX` (repeatable) to restrict the printed metrics to
+specific namespaces (for example `--metric-prefix runtime.lifecycle.`). Pass
+`--metrics-all` to display the full metric set exposed by the runtime snapshot
+instead of the default `runtime.streaming.*` subset.
+
+Use `--verbose` to emit per-frame tables on stdout when investigating specific
+regressions. The JSON payload can be checked into performance dashboards or
+post-processed by CI jobs for automated alerts.
```

## VERIFY
- `pytest scripts/tests/test_runtime_frame_telemetry.py` — passes, validates prefix selection and summary formatting.

## REVIEW_FINDINGS
- No blocking issues; reviewer approved.
- Suggestion: expose metric descriptions in verbose output to aid operators (tracked for TL-101 viewer backlog).

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Unchanged from ## PATCH (no additional adjustments after review).

## FOLLOW_UP_TODOS
- [ ] Ensure histogram formatting logic is added once the runtime begins emitting `MetricKind::Histogram` samples (owner: Tools, priority: medium — needed for full schema coverage).
- [ ] Add an integration smoke test that exercises `runtime_frame_telemetry.py --metrics-all` against a built runtime snapshot within CI to guard against regressions (observability).
- [ ] Evaluate refactoring the metrics presentation into a reusable module for the upcoming TL-101 diagnostics viewer UI (longer-term maintainability).
- [ ] Update `docs/modules/tools/ROADMAP.md` when the CLI viewer work progresses into TL-101 milestones to keep roadmap status in sync (documentation).
