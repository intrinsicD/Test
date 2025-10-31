from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "telemetry_viewer.py"
SPEC = importlib.util.spec_from_file_location("telemetry_viewer", MODULE_PATH)
assert SPEC and SPEC.loader  # narrow type check for mypy/linters
viewer = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = viewer
SPEC.loader.exec_module(viewer)


@pytest.fixture
def telemetry_payload() -> dict[str, object]:
    return {
        "frames": [
            {
                "index": 0,
                "simulation_time": 0.0,
                "dt": 0.016,
                "frame_total_ms": 2.5,
                "category_totals_ms": {"animation": 1.0, "geometry": 1.5},
                "dispatches": [],
            }
        ],
        "summary": {
            "total_ms": 2.5,
            "handoff_ms": 1.5,
            "category:animation": 1.0,
            "category:geometry": 1.5,
        },
        "runtime_diagnostics": {
            "initialize_count": 4,
            "initialize_failure_count": 1,
            "has_initialize_failure": True,
            "last_initialize_failure": {
                "runtime": "example_scene",
                "subsystem": "physics",
                "category": "physics.startup",
                "message": "World failed to initialize",
                "duration_ms": 2.75,
            },
            "streaming": {
                "worker_count": 2,
                "queue_capacity": 64,
                "pending_tasks": 1,
                "active_workers": 1,
                "streaming_pending": 1,
                "streaming_loading": 0,
                "streaming_total_requests": 8,
                "streaming_total_completed": 6,
                "streaming_total_failed": 1,
                "streaming_total_cancelled": 1,
                "streaming_total_rejected": 0,
            },
            "hot_reload": {
                "attempt_count": 4,
                "failure_count": 1,
                "cancelled_count": 1,
                "rejected_count": 0,
                "pending_count": 1,
                "loading_count": 0,
                "total_requests": 4,
                "last_error": "compile error",
                "error_hint": "Verify shader includes",
                "recent_failures": [
                    {
                        "identifier": "textures/paint_albedo.ktx2",
                        "error": "compile error",
                        "hint": "Verify shader includes",
                    }
                ],
            },
            "stages": [
                {
                    "name": "animation.evaluate",
                    "last_ms": 0.5,
                    "average_ms": 0.45,
                    "max_ms": 0.6,
                    "sample_count": 4,
                }
            ],
            "subsystems": [
                {
                    "name": "animation",
                    "last_tick_ms": 0.5,
                    "max_tick_ms": 0.7,
                    "tick_count": 4,
                    "initialize_count": 1,
                    "initialize_failure_count": 0,
                    "last_initialize_failure_ms": 0.0,
                    "last_initialize_failure_category": "",
                    "last_initialize_failure_message": "",
                }
            ],
            "scene_validation": {
                "issue_count": 2,
                "cycle_count": 1,
                "dangling_parent_count": 0,
                "non_finite_transform_count": 0,
                "issues": [
                    {
                        "entity": 7,
                        "related": 3,
                        "type": "cycle",
                        "message": "Cycle detected",
                    }
                ],
            },
            "metrics": {
                "descriptors": [
                    {
                        "name": "runtime.streaming.total_completed",
                        "kind": "counter",
                        "unit": "count",
                        "description": "Completed streaming requests",
                        "labels": {},
                    },
                    {
                        "name": "runtime.lifecycle.last_tick_ms",
                        "kind": "gauge",
                        "unit": "milliseconds",
                        "description": "Last tick duration",
                        "labels": {},
                    },
                ],
                "samples": [
                    {
                        "descriptor_index": 0,
                        "is_integral": True,
                        "value": 6,
                        "int_value": 6,
                    },
                    {
                        "descriptor_index": 1,
                        "is_integral": False,
                        "value": 0.5,
                        "int_value": 0,
                    },
                ],
            },
        },
    }


def test_viewer_filters_metrics_without_verbose(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    telemetry_payload: dict[str, object],
) -> None:
    payload_path = tmp_path / "telemetry.json"
    payload_path.write_text(json.dumps(telemetry_payload), encoding="utf-8")

    exit_code = viewer.main(
        [
            "--input",
            str(payload_path),
            "--metric-prefix",
            "runtime.streaming.",
            "--max-issues",
            "1",
        ]
    )
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "Runtime Telemetry Viewer" in captured
    assert "runtime.streaming.total_completed" in captured
    assert "Completed streaming requests" not in captured
    assert "runtime.lifecycle.last_tick_ms" not in captured
    assert "Sample issues:" in captured
    assert "Hot Reload Guidance" in captured
    assert "Failed reload attempts: 1" in captured
    assert "Verify the source asset path" in captured
    assert "Recent reload failures" in captured
    assert "textures/paint_albedo.ktx2" in captured
    assert "Hint: Verify shader includes" in captured
    assert "Initialization Failures" in captured
    assert "Total initialization failures: 1" in captured
    assert "physics.startup" in captured


def test_viewer_verbose_includes_metric_descriptions(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    telemetry_payload: dict[str, object],
) -> None:
    payload_path = tmp_path / "telemetry.json"
    payload_path.write_text(json.dumps(telemetry_payload), encoding="utf-8")

    exit_code = viewer.main(
        [
            "--input",
            str(payload_path),
            "--metric-prefix",
            "runtime.streaming.",
            "--max-issues",
            "1",
            "--verbose",
        ]
    )
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "runtime.streaming.total_completed" in captured
    assert "Completed streaming requests" in captured


def test_viewer_handles_missing_runtime_diagnostics(tmp_path: Path, capsys: pytest.CaptureFixture[str], telemetry_payload: dict[str, object]) -> None:
    payload = dict(telemetry_payload)
    payload.pop("runtime_diagnostics", None)
    payload_path = tmp_path / "telemetry.json"
    payload_path.write_text(json.dumps(payload), encoding="utf-8")

    exit_code = viewer.main(["--input", str(payload_path)])
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "No runtime diagnostics available" in captured


def test_viewer_suppresses_hot_reload_guidance_without_failures(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    telemetry_payload: dict[str, object],
) -> None:
    payload = json.loads(json.dumps(telemetry_payload))
    diagnostics = payload["runtime_diagnostics"]
    diagnostics["initialize_failure_count"] = 0
    diagnostics["has_initialize_failure"] = False
    diagnostics["last_initialize_failure"] = None
    streaming = payload["runtime_diagnostics"]["streaming"]
    streaming["streaming_total_failed"] = 0
    streaming["streaming_total_cancelled"] = 0
    streaming["streaming_total_rejected"] = 0
    hot_reload = payload["runtime_diagnostics"]["hot_reload"]
    hot_reload["failure_count"] = 0
    hot_reload["cancelled_count"] = 0
    hot_reload["rejected_count"] = 0
    hot_reload["last_error"] = ""
    hot_reload["error_hint"] = ""
    hot_reload["recent_failures"] = []
    payload_path = tmp_path / "telemetry.json"
    payload_path.write_text(json.dumps(payload), encoding="utf-8")

    exit_code = viewer.main(["--input", str(payload_path)])
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "Hot Reload Guidance" not in captured
    assert "Initialization Failures" not in captured


def test_compare_subcommand_generates_report(tmp_path: Path) -> None:
    summary_path = tmp_path / "comparative_summary.json"
    plot_dir = tmp_path / "plots"
    plot_dir.mkdir()
    plot_file = plot_dir / "demo_fps.svg"
    plot_file.write_text("<svg xmlns='http://www.w3.org/2000/svg'></svg>", encoding="utf-8")
    summary_payload = {
        "passed": True,
        "scenarios": [
            {
                "name": "demo",
                "dataset": "demo-dataset",
                "passed": True,
                "metrics": [
                    {
                        "name": "fps",
                        "engine_value": 120.0,
                        "reference_value": 110.0,
                        "delta": 10.0,
                        "relative_delta": 0.0909,
                        "passed": True,
                        "threshold": {"mode": "relative", "limit": 0.2},
                        "plot": "plots/demo_fps.svg",
                    }
                ],
            }
        ],
    }
    summary_path.write_text(json.dumps(summary_payload), encoding="utf-8")

    output_path = tmp_path / "report.html"
    exit_code = viewer.main(
        [
            "compare",
            "--summary",
            str(summary_path),
            "--output",
            str(output_path),
            "--plots-root",
            str(tmp_path),
            "--embed-plots",
        ]
    )
    assert exit_code == 0
    html_output = output_path.read_text(encoding="utf-8")
    assert "demo" in html_output
    assert "demo-dataset" in html_output
    assert "PASS" in html_output
    assert "<svg" in html_output
