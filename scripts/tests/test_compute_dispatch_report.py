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
    queue_totals: Dict[str, float] = {}
    entries: List[Dict[str, Any]] = []
    for name, duration in dispatches.items():
        category = name.split(".", maxsplit=1)[0]
        queue = {
            "animation": "queue-0",
            "physics": "queue-1",
        }.get(category, "queue-2")
        entries.append(
            {
                "name": name,
                "category": category,
                "duration_ms": duration,
                "queue": queue,
            }
        )
        queue_totals[queue] = queue_totals.get(queue, 0.0) + duration

    return {
        "index": index,
        "simulation_time": index * 0.016,
        "timestep": 0.016,
        "total_ms": sum(dispatches.values()),
        "dispatches": entries,
        "category_totals_ms": [
            {
                "category": name.split(".", maxsplit=1)[0],
                "duration_ms": duration,
            }
            for name, duration in dispatches.items()
        ],
        "queue_totals_ms": [
            {"queue": name, "duration_ms": total}
            for name, total in queue_totals.items()
        ],
    }


def _write_payload(
    path: Path,
    frames: List[Dict[str, Any]],
    *,
    baseline_speedup: float = 1.6,
    frame_jitter_ms: float = 0.2,
    jitter_budget_ms: float = 0.5,
    jitter_exceeds: bool = False,
    baseline_stddev_ms: float = 0.2,
) -> Path:
    payload = {
        "metadata": {
            "timestep": 0.016,
            "clock": {"name": "steady_clock", "domain": "cpu"},
            "requested_frames": len(frames),
            "workload": "balanced",
            "dispatcher_backend": "cpu",
            "queue_count": 3,
            "queues": ["queue-0", "queue-1", "queue-2"],
            "queue_assignments": [
                {"category": "animation", "queue": "queue-0"},
                {"category": "physics", "queue": "queue-1"},
                {"category": "geometry", "queue": "queue-2"},
            ],
            "frame_jitter_ms": frame_jitter_ms,
            "frame_jitter_budget_ms": jitter_budget_ms,
            "frame_jitter_exceeds_budget": jitter_exceeds,
        },
        "frames": frames,
        "summary": {
            "stage_timings": [
                {"name": "animation.evaluate", "last_ms": 0.5, "average_ms": 0.45, "max_ms": 0.7, "samples": 8}
            ],
            "queue_dependencies": [
                {
                    "from_queue": "queue-0",
                    "to_queue": "queue-1",
                    "edge_count": 2,
                    "consumer_kernels": ["physics.integrate"],
                }
            ],
            "queue_transitions": [
                {
                    "producer": "animation.evaluate",
                    "consumer": "physics.integrate",
                    "from_queue": "queue-0",
                    "to_queue": "queue-1",
                }
            ],
            "memory": {
                "vertex_count": 4096,
                "joint_count": 60,
                "position_bytes": 4096 * 12,
                "normal_bytes": 4096 * 12,
                "transform_bytes": 60 * 64,
                "total_bytes": 4096 * 24 + 60 * 64,
                "budget_bytes": 256 * 1024 * 1024,
                "exceeds_budget": False,
            },
        },
        "baseline": {
            "frames": len(frames),
            "queue_count": 1,
            "dispatcher_backend": "cpu",
            "queue_names": ["queue-0"],
            "average_frame_ms": 3.2,
            "min_frame_ms": 3.0,
            "max_frame_ms": 3.4,
            "stddev_frame_ms": baseline_stddev_ms,
            "jitter_percent": (baseline_stddev_ms / 3.2) * 100.0,
            "jitter_budget_ms": jitter_budget_ms,
            "jitter_exceeds_budget": jitter_exceeds if baseline_stddev_ms == frame_jitter_ms else baseline_stddev_ms > jitter_budget_ms,
            "speedup": baseline_speedup,
            "target_speedup": 1.5,
            "memory_total_bytes": 4096 * 24 + 60 * 64,
            "memory_total_mebibytes": (4096 * 24 + 60 * 64) / (1024 * 1024),
            "memory_budget_mebibytes": 256.0,
            "memory_exceeds_budget": False,
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
        frame_jitter_ms=0.18,
        jitter_budget_ms=0.5,
        jitter_exceeds=False,
        baseline_stddev_ms=0.22,
    )

    report.main(["--input", str(payload_path), "--top", "2"])
    output = capsys.readouterr().out

    assert "Compute Dispatcher Report" in output
    assert "physics.integrate" in output
    assert "Runtime stage timings" in output
    assert "Workload: balanced" in output
    assert "Dispatcher backend: cpu" in output
    assert "Queues: 3" in output
    assert "Queue assignments: animation→queue-0" in output
    assert "Queue totals:" in output
    assert "Cross-queue synchronization:" in output
    assert "queue-0 -> queue-1" in output
    assert "Baseline frame time (1 queue):" in output
    assert "Baseline backend: cpu" in output
    assert "Speed-up vs baseline: 1.600x (target 1.50x)" in output
    assert "GPU staging estimate:" in output
    assert "Baseline GPU staging:" in output
    assert "Frame dispatch jitter σ: 0.180 ms (budget 0.500 ms)" in output
    assert "WARNING: Frame dispatch jitter exceeds budget" not in output
    assert "WARNING: Baseline frame dispatch jitter" not in output


def test_jitter_threshold_warning(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    payload_path = _write_payload(
        tmp_path / "jitter.json",
        [
            _make_frame(0, {"animation.evaluate": 0.4, "physics.integrate": 1.0}),
            _make_frame(1, {"animation.evaluate": 0.4, "physics.integrate": 2.0}),
            _make_frame(2, {"animation.evaluate": 0.4, "physics.integrate": 0.5}),
        ],
        baseline_speedup=1.2,
        frame_jitter_ms=0.75,
        jitter_budget_ms=0.5,
        jitter_exceeds=True,
        baseline_stddev_ms=0.72,
    )

    report.main(["--input", str(payload_path), "--jitter-threshold", "5.0", "--top", "1"])
    output = capsys.readouterr().out

    assert "WARNING" in output
    assert "physics.integrate" in output
    assert "Dispatcher backend: cpu" in output
    assert "Baseline backend: cpu" in output
    assert "Speed-up below performance target" in output
    assert "WARNING: Frame dispatch jitter exceeds budget" in output
    assert "WARNING: Baseline frame dispatch jitter" in output


def test_run_to_run_variance_summary(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    first = _write_payload(
        tmp_path / "run_a.json",
        [
            _make_frame(0, {"animation.evaluate": 0.4, "physics.integrate": 0.8}),
            _make_frame(1, {"animation.evaluate": 0.4, "physics.integrate": 0.8}),
        ],
    )
    second = _write_payload(
        tmp_path / "run_b.json",
        [
            _make_frame(0, {"animation.evaluate": 0.4, "physics.integrate": 1.2}),
            _make_frame(1, {"animation.evaluate": 0.4, "physics.integrate": 1.2}),
        ],
    )

    report.main(
        [
            "--input",
            str(first),
            str(second),
            "--variance-threshold",
            "1.0",
            "--top",
            "1",
        ]
    )
    output = capsys.readouterr().out

    assert "run_a.json" in output
    assert "run_b.json" in output
    assert "Run-to-run frame time variance" in output
    assert "WARNING: Run-to-run variance" in output
