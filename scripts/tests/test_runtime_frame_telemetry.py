"""Unit tests for the runtime frame telemetry diagnostics utility."""

from __future__ import annotations

import importlib.util
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
        shutdown_count=2,
        tick_count=3,
        last_initialize_ms=0.1,
        last_shutdown_ms=0.2,
        last_tick_ms=0.3,
        average_tick_ms=0.25,
        max_tick_ms=0.4,
        stages=[
            telemetry.RuntimeStageMetric(
                name="animation.evaluate",
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
            )
        ],
    )
    payload = telemetry._diagnostics_to_dict(snapshot)
    assert payload["initialize_count"] == 1
    assert payload["tick_count"] == 3
    assert payload["stages"][0]["name"] == "animation.evaluate"
    assert payload["subsystems"][0]["last_tick_ms"] == pytest.approx(0.6)

