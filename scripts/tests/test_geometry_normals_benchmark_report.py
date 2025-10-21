import importlib.util
import json
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "geometry_normals_benchmark_report.py"
SPEC = importlib.util.spec_from_file_location("geometry_normals_benchmark_report", MODULE_PATH)
assert SPEC and SPEC.loader
report = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = report
SPEC.loader.exec_module(report)


@pytest.fixture
def benchmark_payload() -> dict[str, object]:
    return {
        "benchmark": "geometry_normal_recompute",
        "config": {
            "resolution": 256,
            "iterations": 128,
            "vertex_count": 66049,
            "triangle_count": 131072,
        },
        "metrics": {
            "duration_seconds": 0.12,
            "iterations_per_second": 1024.0,
            "vertices_per_second": 67502080.0,
            "triangles_per_second": 134217728.0,
            "normal_checksum": 66049.0,
        },
    }


def test_report_without_baseline(
    tmp_path: Path, capsys: pytest.CaptureFixture[str], benchmark_payload: dict[str, object]
) -> None:
    payload_path = tmp_path / "current.json"
    payload_path.write_text(json.dumps(benchmark_payload), encoding="utf-8")

    exit_code = report.main(["--current", str(payload_path), "--current-label", "nightly"])
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "Geometry Normals Benchmark Report" in captured
    assert "Current run: nightly" in captured
    assert "Iterations / second: 1024.0000" in captured
    assert "Baseline:" not in captured


def test_report_with_baseline(tmp_path: Path, capsys: pytest.CaptureFixture[str], benchmark_payload: dict[str, object]) -> None:
    current_path = tmp_path / "current.json"
    baseline_path = tmp_path / "baseline.json"
    current = dict(benchmark_payload)
    current["metrics"] = dict(benchmark_payload["metrics"])
    current["metrics"]["iterations_per_second"] = 1100.0
    baseline = dict(benchmark_payload)
    baseline["metrics"] = dict(benchmark_payload["metrics"])
    baseline["metrics"]["iterations_per_second"] = 1000.0
    current_path.write_text(json.dumps(current), encoding="utf-8")
    baseline_path.write_text(json.dumps(baseline), encoding="utf-8")

    exit_code = report.main(
        [
            "--current",
            str(current_path),
            "--baseline",
            str(baseline_path),
            "--current-label",
            "current",
            "--baseline-label",
            "baseline",
        ]
    )
    assert exit_code == 0
    captured = capsys.readouterr().out
    assert "Baseline: baseline" in captured
    assert "Iterations / second: 1100.0000 (Δ +100.0000, +10.00%)" in captured


def test_report_rejects_invalid_payload(tmp_path: Path) -> None:
    payload_path = tmp_path / "invalid.json"
    payload_path.write_text("[]", encoding="utf-8")
    exit_code = report.main(["--current", str(payload_path)])
    assert exit_code == 1
