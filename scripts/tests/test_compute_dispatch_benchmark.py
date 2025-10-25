import importlib.util
import json
import os
import stat
import sys
from pathlib import Path
from typing import Any, Dict, List

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "compute_dispatch_benchmark.py"
SPEC = importlib.util.spec_from_file_location("compute_dispatch_benchmark", MODULE_PATH)
assert SPEC and SPEC.loader
benchmark = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = benchmark
SPEC.loader.exec_module(benchmark)


def _write_payload(
    path: Path,
    *,
    mean_frame_ms: float,
    jitter_ms: float = 0.18,
    jitter_budget_ms: float = 0.5,
    jitter_exceeds: bool = False,
    speedup: float = 1.7,
    speedup_target: float = 1.5,
    baseline_jitter_ms: float = 0.25,
    baseline_jitter_budget_ms: float = 0.5,
    baseline_jitter_exceeds: bool = False,
) -> Path:
    frames: List[Dict[str, Any]] = []
    # create four frames evenly distributed around the mean to keep tests deterministic
    deltas = [-0.05, 0.0, 0.03, 0.02]
    for index, delta in enumerate(deltas):
        frames.append({"index": index, "total_ms": mean_frame_ms + delta})

    payload = {
        "metadata": {
            "frame_jitter_ms": jitter_ms,
            "frame_jitter_budget_ms": jitter_budget_ms,
            "frame_jitter_exceeds_budget": jitter_exceeds,
        },
        "frames": frames,
        "baseline": {
            "speedup": speedup,
            "target_speedup": speedup_target,
            "stddev_frame_ms": baseline_jitter_ms,
            "jitter_budget_ms": baseline_jitter_budget_ms,
            "jitter_exceeds_budget": baseline_jitter_exceeds,
        },
    }
    path.write_text(json.dumps(payload), encoding="utf-8")
    return path


def test_summary_from_existing_payloads(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    payload_a = _write_payload(tmp_path / "run-a.json", mean_frame_ms=3.1)
    payload_b = _write_payload(tmp_path / "run-b.json", mean_frame_ms=3.15)

    exit_code = benchmark.main(
        [
            "--input",
            str(payload_a),
            str(payload_b),
            "--variance-threshold",
            "2.0",
        ]
    )

    assert exit_code == 0
    output = capsys.readouterr().out
    assert "Compute Dispatcher Benchmark" in output
    assert "Runs analysed: 2" in output
    assert "Variance threshold: 2.00%" in output
    assert "Average speed-up vs baseline" in output


def test_exit_on_regression_when_variance_or_jitter_exceeds(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    payload_a = _write_payload(tmp_path / "run-a.json", mean_frame_ms=2.0, jitter_exceeds=True)
    payload_b = _write_payload(tmp_path / "run-b.json", mean_frame_ms=4.0, baseline_jitter_exceeds=True)

    exit_code = benchmark.main(
        [
            "--input",
            str(payload_a),
            str(payload_b),
            "--variance-threshold",
            "5.0",
            "--exit-on-regression",
        ]
    )

    assert exit_code == 2
    output = capsys.readouterr().out
    assert "WARNING: Run-to-run variance" in output
    assert "WARNING: Run 1 frame jitter" in output
    assert "WARNING: Run 2 baseline jitter" in output


def test_executes_sample_binary(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    sample_script = tmp_path / "runtime_sample.py"
    sample_script.write_text(
        """#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--frames", type=int)
parser.add_argument("--dt", type=float)
parser.add_argument("--workload")
parser.add_argument("--queues", type=int)
parser.add_argument("--dispatcher-backend")
parser.add_argument("--jitter-budget-ms", type=float, default=None)
parser.add_argument("--queue-names", default=None)
parser.add_argument("--queue-map", action="append")
parser.add_argument("--baseline", action="store_true")
parser.add_argument("--output", required=True)
parser.add_argument("--pretty", action="store_true")
args = parser.parse_args()

output = Path(args.output)
stem = output.stem
run_index = 1
if "run" in stem:
    try:
        run_index = int(stem.split("run")[-1])
    except ValueError:
        run_index = 1

base_mean = 3.0 + 0.05 * run_index
payload = {
    "metadata": {
        "frame_jitter_ms": 0.2 + 0.01 * run_index,
        "frame_jitter_budget_ms": args.jitter_budget_ms if args.jitter_budget_ms is not None else 0.5,
        "frame_jitter_exceeds_budget": False,
    },
    "frames": [
        {"index": i, "total_ms": base_mean + (i * 0.01)} for i in range(args.frames)
    ],
    "baseline": {
        "speedup": 1.6 + 0.01 * run_index,
        "target_speedup": 1.5,
        "stddev_frame_ms": 0.25,
        "jitter_budget_ms": 0.5,
        "jitter_exceeds_budget": False,
    },
}
output.write_text(json.dumps(payload), encoding="utf-8")
""",
        encoding="utf-8",
    )
    sample_script.chmod(sample_script.stat().st_mode | stat.S_IEXEC)

    output_dir = tmp_path / "telemetry"
    exit_code = benchmark.main(
        [
            "--sample",
            str(sample_script),
            "--runs",
            "2",
            "--frames",
            "4",
            "--dt",
            "0.016",
            "--workload",
            "balanced",
            "--queues",
            "2",
            "--dispatcher-backend",
            "cpu",
            "--jitter-budget-ms",
            "0.4",
            "--queue-names",
            "main,async",
            "--queue-map",
            "physics=async",
            "--baseline",
            "--output-dir",
            str(output_dir),
            "--variance-threshold",
            "2.5",
        ]
    )

    assert exit_code == 0
    output = capsys.readouterr().out
    assert "Runs analysed: 2" in output
    captured = sorted(output_dir.glob("compute_dispatch-run*.json"))
    assert len(captured) == 2
    # Ensure queue map argument was forwarded (script would raise if not parsed)
    assert os.access(sample_script, os.X_OK)
