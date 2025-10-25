import importlib.util
import json
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "animation_sampling_report.py"
SPEC = importlib.util.spec_from_file_location("animation_sampling_report", MODULE_PATH)
assert SPEC and SPEC.loader
report = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = report
SPEC.loader.exec_module(report)


def _make_frame(
    index: int,
    timestep: float,
    dispatches: Sequence[Tuple[str, str, str, float]],
) -> Dict[str, object]:
    total_ms = sum(duration for _, _, _, duration in dispatches)
    category_totals: Dict[str, float] = {}
    queue_totals: Dict[str, float] = {}
    dispatch_entries: List[Dict[str, object]] = []

    for name, category, queue, duration in dispatches:
        dispatch_entries.append(
            {
                "name": name,
                "category": category,
                "queue": queue,
                "duration_ms": duration,
            }
        )
        category_totals[category] = category_totals.get(category, 0.0) + duration
        queue_totals[queue] = queue_totals.get(queue, 0.0) + duration

    return {
        "index": index,
        "simulation_time": (index + 1) * timestep,
        "timestep": timestep,
        "total_ms": total_ms,
        "dispatches": dispatch_entries,
        "category_totals_ms": [
            {"category": category, "duration_ms": duration}
            for category, duration in category_totals.items()
        ],
        "queue_totals_ms": [
            {"queue": queue, "duration_ms": duration}
            for queue, duration in queue_totals.items()
        ],
    }


def _write_capture(
    path: Path,
    *,
    scenario: str,
    frames: Iterable[Sequence[Tuple[str, str, str, float]]],
    mean_ms: float,
    stddev_ms: float,
    jitter_budget_ms: float,
    jitter_exceeds: bool,
    task: str,
    clip_name: str,
    cuda_available: bool | None,
) -> Path:
    timestep = 0.016
    frame_list = [
        _make_frame(index, timestep, dispatches)
        for index, dispatches in enumerate(frames)
    ]
    payload = {
        "metadata": {
            "scenario": scenario,
            "task": task,
            "clip_name": clip_name,
            "clip_duration": 10.0,
            "track_count": 42,
            "rig_joint_count": 60,
            "frames": len(frame_list),
            "timestep": timestep,
        },
        "frames": frame_list,
        "summary": {
            "samples": len(frame_list),
            "mean_ms": mean_ms,
            "min_ms": mean_ms - 0.1,
            "max_ms": mean_ms + 0.1,
            "stddev_ms": stddev_ms,
            "total_ms": mean_ms * len(frame_list),
        },
        "frame_jitter_ms": stddev_ms,
        "frame_jitter_budget_ms": jitter_budget_ms,
        "frame_jitter_exceeds_budget": jitter_exceeds,
    }
    if cuda_available is not None:
        payload["metadata"]["cuda_available"] = cuda_available

    path.write_text(json.dumps(payload), encoding="utf-8")
    return path


def test_report_lists_scenarios_and_speedup(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    cpu_path = _write_capture(
        tmp_path / "cpu.json",
        scenario="cpu_baseline",
        frames=[[("animation.sample_clip", "animation.sample", "cpu", 0.45)]] * 3,
        mean_ms=0.45,
        stddev_ms=0.02,
        jitter_budget_ms=0.5,
        jitter_exceeds=False,
        task="AN-230.1",
        clip_name="walk",
        cuda_available=None,
    )
    gpu_path = _write_capture(
        tmp_path / "gpu.json",
        scenario="gpu_async",
        frames=[
            [
                ("animation.submit_sample", "animation.control", "cpu", 0.12),
                ("animation.sample_clip.gpu", "animation.sample", "gpu", 0.18),
            ]
        ]
        * 3,
        mean_ms=0.30,
        stddev_ms=0.04,
        jitter_budget_ms=0.4,
        jitter_exceeds=True,
        task="AN-230.2",
        clip_name="walk",
        cuda_available=False,
    )

    exit_code = report.main(["--input", str(cpu_path), str(gpu_path), "--top", "2"])
    assert exit_code == 0
    output = capsys.readouterr().out

    assert "Animation Sampling Benchmark Report" in output
    assert "Scenario: cpu_baseline" in output
    assert "Scenario: gpu_async" in output
    assert "WARNING: Frame jitter exceeds configured budget" in output
    assert "CUDA dispatcher available: no" in output
    assert "Comparison vs CPU baseline" in output
    assert "gpu_async: 1.500x speed-up" in output
    assert "animation.sample_clip.gpu" in output
    assert "Queue utilisation" in output


def test_output_written_when_requested(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    capture_path = _write_capture(
        tmp_path / "single.json",
        scenario="cpu_baseline",
        frames=[[("animation.sample_clip", "animation.sample", "cpu", 0.50)]] * 2,
        mean_ms=0.50,
        stddev_ms=0.03,
        jitter_budget_ms=0.5,
        jitter_exceeds=False,
        task="AN-230.1",
        clip_name="idle",
        cuda_available=True,
    )
    output_path = tmp_path / "report.txt"

    exit_code = report.main(
        ["--input", str(capture_path), "--output", str(output_path), "--top", "1"]
    )
    assert exit_code == 0
    captured = capsys.readouterr().out

    assert output_path.exists()
    assert output_path.read_text(encoding="utf-8") == captured
    assert "Scenario: cpu_baseline" in captured
    assert "animation.sample_clip" in captured
