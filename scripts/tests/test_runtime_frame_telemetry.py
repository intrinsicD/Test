"""Unit tests for the runtime frame telemetry diagnostics utility."""

from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "runtime_frame_telemetry.py"
SPEC = importlib.util.spec_from_file_location("runtime_frame_telemetry", MODULE_PATH)
assert SPEC and SPEC.loader  # narrow type check for mypy/linters
telemetry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = telemetry
SPEC.loader.exec_module(telemetry)


def _make_frame(index: int, dispatches: dict[str, float]) -> telemetry.FrameSample:
    samples = [
        telemetry.DispatchSample(
            name=name,
            duration_ms=value,
            category=name.split(".", maxsplit=1)[0],
        )
        for name, value in dispatches.items()
    ]
    category_totals = {sample.category: sample.duration_ms for sample in samples}
    return telemetry.FrameSample(
        index=index,
        simulation_time=float(index) * 0.01,
        timestep=0.01,
        dispatches=samples,
        category_totals_ms=category_totals,
        frame_total_ms=sum(category_totals.values()),
    )


def _make_metrics_snapshot() -> telemetry.RuntimeMetricsSnapshot:
    descriptors = [
        telemetry.RuntimeMetricDescriptor(
            name="runtime.streaming.total_completed",
            kind="counter",
            unit="count",
            description="",
            labels={},
        ),
        telemetry.RuntimeMetricDescriptor(
            name="runtime.lifecycle.last_tick_ms",
            kind="gauge",
            unit="milliseconds",
            description="",
            labels={},
        ),
        telemetry.RuntimeMetricDescriptor(
            name="runtime.stage.last_ms",
            kind="gauge",
            unit="milliseconds",
            description="",
            labels={"stage": "geometry.deform"},
        ),
        telemetry.RuntimeMetricDescriptor(
            name="runtime.stage.last_ms",
            kind="gauge",
            unit="milliseconds",
            description="",
            labels={"stage": "animation.evaluate"},
        ),
    ]
    samples = [
        telemetry.RuntimeMetricSample(
            descriptor_index=0,
            is_integral=True,
            value=10.0,
            int_value=10,
        ),
        telemetry.RuntimeMetricSample(
            descriptor_index=1,
            is_integral=False,
            value=0.5,
            int_value=0,
        ),
        telemetry.RuntimeMetricSample(
            descriptor_index=2,
            is_integral=False,
            value=1.23,
            int_value=0,
        ),
        telemetry.RuntimeMetricSample(
            descriptor_index=3,
            is_integral=False,
            value=2.34,
            int_value=0,
        ),
    ]
    return telemetry.RuntimeMetricsSnapshot(descriptors=descriptors, samples=samples)


def test_parse_variance_checks_valid() -> None:
    checks = telemetry._parse_variance_checks(
        ["geometry.deform:5", "physics.integrate:3.5"], 0.0
    )
    assert [check.dispatch_name for check in checks] == [
        "geometry.deform",
        "physics.integrate",
    ]
    assert [check.max_percent for check in checks] == [5.0, 3.5]
    assert all(check.trim_fraction == 0.0 for check in checks)


@pytest.mark.parametrize(
    "payload",
    ["invalid", ":3", "geometry.deform:-1"],
)
def test_parse_variance_checks_invalid(payload: str) -> None:
    with pytest.raises(ValueError):
        telemetry._parse_variance_checks([payload], 0.0)


def test_parse_variance_checks_rejects_excessive_trim() -> None:
    with pytest.raises(ValueError):
        telemetry._parse_variance_checks(["geometry.deform:5"], 0.5)


def test_evaluate_variance_pass() -> None:
    samples = [
        _make_frame(0, {"geometry.deform": 1.0}),
        _make_frame(1, {"geometry.deform": 1.01}),
        _make_frame(2, {"geometry.deform": 0.99}),
    ]
    check = telemetry.VarianceCheck("geometry.deform", 5.0)
    result = telemetry.evaluate_variance(samples, check)
    assert result.passed
    assert pytest.approx(result.mean_ms, rel=1e-6) == 1.0
    assert result.percent < 5.0


def test_evaluate_variance_fail_when_threshold_exceeded() -> None:
    samples = [
        _make_frame(0, {"geometry.deform": 1.0}),
        _make_frame(1, {"geometry.deform": 2.0}),
        _make_frame(2, {"geometry.deform": 0.5}),
    ]
    check = telemetry.VarianceCheck("geometry.deform", 10.0)
    result = telemetry.evaluate_variance(samples, check)
    assert not result.passed
    assert result.percent > check.max_percent


def test_evaluate_variance_supports_trimming() -> None:
    samples = [
        _make_frame(0, {"geometry.deform": 1.0}),
        _make_frame(1, {"geometry.deform": 1.0}),
        _make_frame(2, {"geometry.deform": 10.0}),
        _make_frame(3, {"geometry.deform": 1.0}),
        _make_frame(4, {"geometry.deform": 1.0}),
    ]
    check = telemetry.VarianceCheck("geometry.deform", 5.0, trim_fraction=0.2)
    result = telemetry.evaluate_variance(samples, check)
    assert len(result.durations_ms) == 3
    assert result.total_samples == 5
    assert result.passed


def test_evaluate_variance_raises_for_missing_dispatch() -> None:
    samples = [_make_frame(0, {"geometry.finalize": 1.0})]
    check = telemetry.VarianceCheck("geometry.deform", 5.0)
    with pytest.raises(ValueError):
        telemetry.evaluate_variance(samples, check)


def test_diagnostics_to_dict_roundtrip() -> None:
    snapshot = telemetry.RuntimeDiagnosticsSnapshot(
        initialize_count=1,
        initialize_failure_count=2,
        shutdown_count=2,
        tick_count=3,
        last_initialize_ms=0.1,
        last_shutdown_ms=0.2,
        last_tick_ms=0.3,
        has_initialize_failure=True,
        last_initialize_failure=telemetry.RuntimeInitializationFailure(
            runtime="test_runtime",
            subsystem="animation",
            category="test.category",
            message="Subsystem failed",
            duration_ms=1.25,
        ),
        average_tick_ms=0.25,
        max_tick_ms=0.4,
        phases=[
            telemetry.RuntimePhaseMetric(
                phase="simulation",
                last_ms=2.5,
                average_ms=2.0,
                max_ms=3.0,
                sample_count=5,
            )
        ],
        stages=[
            telemetry.RuntimeStageMetric(
                name="animation.evaluate",
                phase="simulation",
                last_ms=1.1,
                average_ms=1.0,
                max_ms=1.2,
                sample_count=4,
            )
        ],
        subsystems=[
            telemetry.RuntimeSubsystemMetric(
                name="physics",
                last_initialize_ms=0.5,
                last_tick_ms=0.6,
                last_shutdown_ms=0.7,
                max_initialize_ms=0.55,
                max_tick_ms=0.65,
                max_shutdown_ms=0.75,
                initialize_count=1,
                tick_count=3,
                shutdown_count=1,
                initialize_failure_count=1,
                last_initialize_failure_ms=0.33,
                last_initialize_failure_category="physics.startup",
                last_initialize_failure_message="configuration missing",
            )
        ],
        command_encoders=[
            telemetry.RuntimeCommandEncoderStat(
                pass_name="ForwardGeometry",
                queue="Graphics",
                command_buffer=1,
                draw_count=2,
                dispatch_count=0,
            )
        ],
        streaming=telemetry.RuntimeStreamingMetrics(
            worker_count=2,
            queue_capacity=16,
            pending_tasks=1,
            active_workers=1,
            total_enqueued=4,
            total_executed=3,
            streaming_pending=1,
            streaming_loading=0,
            streaming_total_requests=3,
            streaming_total_completed=2,
            streaming_total_failed=1,
            streaming_total_cancelled=0,
            streaming_total_rejected=0,
            geometry_failures_by_error={'invalid_argument': 1},
        ),
        hot_reload=telemetry.HotReloadMetrics(
            attempt_count=3,
            failure_count=1,
            cancelled_count=1,
            rejected_count=0,
            pending_count=1,
            loading_count=0,
            total_requests=3,
            last_error="compile error",
            error_hint="Rebuild shader",
            recent_failures=[
                telemetry.HotReloadFailure(
                    identifier="materials/paint.material.json",
                    error="compile error",
                    hint="Rebuild shader",
                )
            ],
        ),
        scene_validation=telemetry.SceneValidationSnapshot(
            issue_count=2,
            cycle_count=1,
            dangling_parent_count=1,
            missing_parent_hierarchy_count=1,
            non_finite_transform_count=0,
            transform_mismatch_count=1,
            issues=[
                telemetry.SceneHierarchyIssue(
                    entity=42,
                    related=24,
                    type="cycle",
                    message="entity references itself",
                )
            ],
        ),
        animation=telemetry.RuntimeAnimationTelemetry(
            clip_track_count=1,
            pose_joint_count=1,
            clip_duration=1.0,
            playback_time=0.25,
            playback_speed=1.0,
            category_totals=[
                telemetry.RuntimeAnimationDispatchTotal(label="animation", duration_ms=0.5)
            ],
            queue_totals=[
                telemetry.RuntimeAnimationDispatchTotal(label="cpu", duration_ms=0.5)
            ],
        ),
    )
    payload = telemetry._diagnostics_to_dict(snapshot)
    assert payload["initialize_count"] == 1
    assert payload["tick_count"] == 3
    assert payload["stages"][0]["name"] == "animation.evaluate"
    assert payload["stages"][0]["phase"] == "simulation"
    assert payload["subsystems"][0]["last_tick_ms"] == pytest.approx(0.6)
    assert payload["scene_validation"]["issue_count"] == 2
    assert payload["scene_validation"]["issues"][0]["type"] == "cycle"
    assert payload["hot_reload"]["failure_count"] == 1
    assert payload["hot_reload"]["error_hint"] == "Rebuild shader"
    assert payload["hot_reload"]["total_requests"] == 3
    assert payload["hot_reload"]["recent_failures"][0]["identifier"] == "materials/paint.material.json"
    assert payload["streaming"]["geometry_failures_by_error"]["invalid_argument"] == 1
    assert payload["phases"][0]["phase"] == "simulation"
    assert payload["phases"][0]["last_ms"] == pytest.approx(2.5)
    assert payload["initialize_failure_count"] == 2
    assert payload["has_initialize_failure"] is True
    assert payload["last_initialize_failure"]["runtime"] == "test_runtime"
    assert payload["last_initialize_failure"]["duration_ms"] == pytest.approx(1.25)
    assert payload["subsystems"][0]["initialize_failure_count"] == 1
    assert payload["subsystems"][0]["last_initialize_failure_category"] == "physics.startup"
    assert payload["subsystems"][0]["last_initialize_failure_message"] == "configuration missing"
    assert payload["command_encoders"][0]["pass_name"] == "ForwardGeometry"
    assert payload["command_encoders"][0]["draw_count"] == 2
    assert payload["animation"]["clip_track_count"] == 1
    assert payload["animation"]["category_totals"][0]["label"] == "animation"


def test_select_metrics_filters_by_prefix() -> None:
    snapshot = _make_metrics_snapshot()
    filtered = telemetry._select_metrics(snapshot, ("runtime.stage.",))
    assert [descriptor.labels["stage"] for descriptor, _ in filtered] == [
        "animation.evaluate",
        "geometry.deform",
    ]


def test_select_metrics_returns_all_without_prefix() -> None:
    snapshot = _make_metrics_snapshot()
    filtered = telemetry._select_metrics(snapshot, None)
    assert [descriptor.name for descriptor, _ in filtered] == [
        "runtime.lifecycle.last_tick_ms",
        "runtime.stage.last_ms",
        "runtime.stage.last_ms",
        "runtime.streaming.total_completed",
    ]


def test_print_metric_summary_reports_missing_prefix(capsys: pytest.CaptureFixture[str]) -> None:
    snapshot = _make_metrics_snapshot()
    telemetry._print_metric_summary(snapshot, ("physics.",))
    captured = capsys.readouterr().out
    assert "metrics (filter: physics.)" in captured
    assert "no metrics matched" in captured


def test_print_metric_summary_emits_values(capsys: pytest.CaptureFixture[str]) -> None:
    snapshot = _make_metrics_snapshot()
    telemetry._print_metric_summary(snapshot, ("runtime.lifecycle.",))
    captured = capsys.readouterr().out
    assert "metrics (filter: runtime.lifecycle.)" in captured
    assert "[runtime.lifecycle]" in captured
    assert "last_tick_ms" in captured


def test_build_profile_trace_generates_chrome_events() -> None:
    samples = [
        _make_frame(0, {"animation.evaluate": 1.0, "physics.integrate": 2.0}),
        _make_frame(1, {"geometry.deform": 3.0}),
    ]
    trace = telemetry.build_profile_trace(samples, title="Test Trace")
    assert trace["displayTimeUnit"] == "ms"
    assert trace["metadata"] == {"title": "Test Trace", "frameCount": 2}

    events = trace["traceEvents"]
    process_events = [event for event in events if event["ph"] == "M" and event["name"] == "process_name"]
    assert process_events and process_events[0]["args"]["name"] == "Test Trace"

    dispatch_events = [event for event in events if event.get("cat") not in (None, "frame") and event["ph"] == "X"]
    assert [event["name"] for event in dispatch_events] == [
        "animation.evaluate",
        "physics.integrate",
        "geometry.deform",
    ]
    for event in dispatch_events:
        assert event["dur"] > 0.0


def test_write_profile_trace(tmp_path: Path) -> None:
    samples = [_make_frame(0, {"animation.evaluate": 1.25})]
    output = tmp_path / "trace.json"
    telemetry.write_profile_trace(samples, output, title="Trace Title")

    payload = json.loads(output.read_text(encoding="utf-8"))
    assert payload["metadata"]["title"] == "Trace Title"
    assert payload["metadata"]["frameCount"] == 1
    events = payload["traceEvents"]
    assert any(event["name"] == "frame.total" for event in events)

