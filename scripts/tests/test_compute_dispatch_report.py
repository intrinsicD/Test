import importlib.util
import json
import sys
from pathlib import Path
from typing import Any, Dict, List

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "compute_dispatch_report.py"
SPEC = importlib.util.spec_from_file_location("compute_dispatch_report", MODULE_PATH)
assert SPEC and SPEC.loader
report = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = report
SPEC.loader.exec_module(report)


def _make_frame(index: int, dispatches: Dict[str, float]) -> Dict[str, Any]:
    return {
        "index": index,
        "simulation_time": index * 0.016,
        "timestep": 0.016,
        "total_ms": sum(dispatches.values()),
        "dispatches": [
            {
                "name": name,
                "category": name.split(".", maxsplit=1)[0],
                "duration_ms": duration,
            }
            for name, duration in dispatches.items()
        ],
        "category_totals_ms": [
            {
                "category": name.split(".", maxsplit=1)[0],
                "duration_ms": duration,
            }
            for name, duration in dispatches.items()
        ],
    }


def _write_payload(path: Path, frames: List[Dict[str, Any]]) -> Path:
    payload = {
        "metadata": {
            "timestep": 0.016,
            "clock": {"name": "steady_clock", "domain": "cpu"},
        },
        "frames": frames,
        "summary": {
            "stage_timings": [
                {"name": "animation.evaluate", "last_ms": 0.5, "average_ms": 0.45, "max_ms": 0.7, "samples": 8}
            ]
        },
    }
    path.write_text(json.dumps(payload), encoding="utf-8")
    return path


def test_render_summary_lists_top_kernels(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    payload_path = _write_payload(
        tmp_path / "report.json",
        [
            _make_frame(0, {"animation.evaluate": 0.4, "physics.integrate": 0.8, "geometry.deform": 0.6}),
            _make_frame(1, {"animation.evaluate": 0.5, "physics.integrate": 0.9, "geometry.deform": 0.7}),
        ],
    )

    report.main(["--input", str(payload_path), "--top", "2"])
    output = capsys.readouterr().out

    assert "Compute Dispatcher Report" in output
    assert "physics.integrate" in output
    assert "Runtime stage timings" in output


def test_jitter_threshold_warning(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    payload_path = _write_payload(
        tmp_path / "jitter.json",
        [
            _make_frame(0, {"animation.evaluate": 0.4, "physics.integrate": 1.0}),
            _make_frame(1, {"animation.evaluate": 0.4, "physics.integrate": 2.0}),
            _make_frame(2, {"animation.evaluate": 0.4, "physics.integrate": 0.5}),
        ],
    )

    report.main(["--input", str(payload_path), "--jitter-threshold", "5.0", "--top", "1"])
    output = capsys.readouterr().out

    assert "WARNING" in output
    assert "physics.integrate" in output
