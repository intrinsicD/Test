from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, MutableMapping, Optional, Sequence, Tuple


@dataclass
class SummaryStats:
    samples: int
    mean_ms: float
    min_ms: float
    max_ms: float
    stddev_ms: float
    total_ms: float

    @property
    def fps(self) -> float:
        if self.mean_ms <= 0.0:
            return 0.0
        return 1000.0 / self.mean_ms


@dataclass
class Capture:
    path: Path
    scenario: str
    task: str
    clip_name: str
    clip_duration: float
    track_count: int
    rig_joint_count: int
    frames_requested: int
    timestep: float
    cuda_available: Optional[bool]
    summary: SummaryStats
    jitter_budget_ms: Optional[float]
    jitter_exceeds_budget: bool
    dispatch_totals_ms: Dict[str, float]
    category_totals_ms: Dict[str, float]
    queue_totals_ms: Dict[str, float]

    @property
    def jitter_label(self) -> str:
        budget = self.jitter_budget_ms
        budget_text = f"budget {budget:.3f} ms" if budget is not None else "no budget"
        return (
            f"Frame jitter σ: {self.summary.stddev_ms:.3f} ms ({budget_text})"
        )


def _load_capture(path: Path) -> Capture:
    payload = json.loads(path.read_text(encoding="utf-8"))

    metadata = payload.get("metadata", {})
    summary_payload = payload.get("summary", {})
    frame_totals = summary_payload.get("frame_totals_ms", {})
    summary_stats = SummaryStats(
        samples=int(frame_totals.get("samples", summary_payload.get("samples", metadata.get("frames", 0)))),
        mean_ms=float(frame_totals.get("mean_ms", summary_payload.get("mean_ms", 0.0))),
        min_ms=float(frame_totals.get("min_ms", summary_payload.get("min_ms", 0.0))),
        max_ms=float(frame_totals.get("max_ms", summary_payload.get("max_ms", 0.0))),
        stddev_ms=float(frame_totals.get("stddev_ms", summary_payload.get("stddev_ms", 0.0))),
        total_ms=float(frame_totals.get("total_ms", summary_payload.get("total_ms", 0.0))),
    )

    if summary_stats.samples <= 0 and payload.get("frames"):
        summary_stats.samples = len(payload["frames"])

    frames = payload.get("frames", [])
    dispatch_totals: Dict[str, float] = {}
    category_totals: Dict[str, float] = {}
    queue_totals: Dict[str, float] = {}

    for frame in frames:
        for entry in frame.get("dispatches", []):
            name = str(entry.get("name", ""))
            dispatch_totals[name] = dispatch_totals.get(name, 0.0) + float(entry.get("duration_ms", 0.0))
        for entry in frame.get("category_totals_ms", []):
            category = str(entry.get("category", ""))
            category_totals[category] = category_totals.get(category, 0.0) + float(entry.get("duration_ms", 0.0))
        for entry in frame.get("queue_totals_ms", []):
            queue = str(entry.get("queue", ""))
            queue_totals[queue] = queue_totals.get(queue, 0.0) + float(entry.get("duration_ms", 0.0))

    frame_count = float(len(frames)) if frames else 0.0
    if frame_count > 0.0:
        dispatch_totals = {name: value / frame_count for name, value in dispatch_totals.items()}
        category_totals = {name: value / frame_count for name, value in category_totals.items()}
        queue_totals = {name: value / frame_count for name, value in queue_totals.items()}

    return Capture(
        path=path,
        scenario=str(metadata.get("scenario", "unknown")),
        task=str(metadata.get("task", "")),
        clip_name=str(metadata.get("clip_name", metadata.get("clip", "<unknown>"))),
        clip_duration=float(metadata.get("clip_duration", 0.0)),
        track_count=int(metadata.get("track_count", 0)),
        rig_joint_count=int(metadata.get("rig_joint_count", metadata.get("rig_joints", 0))),
        frames_requested=int(metadata.get("frames", summary_stats.samples)),
        timestep=float(metadata.get("timestep", 0.0)),
        cuda_available=(
            bool(metadata["cuda_available"]) if "cuda_available" in metadata else None
        ),
        summary=summary_stats,
        jitter_budget_ms=(
            float(payload.get("frame_jitter_budget_ms"))
            if "frame_jitter_budget_ms" in payload
            else None
        ),
        jitter_exceeds_budget=bool(payload.get("frame_jitter_exceeds_budget", False)),
        dispatch_totals_ms=dict(sorted(dispatch_totals.items(), key=lambda item: item[0])),
        category_totals_ms=dict(sorted(category_totals.items(), key=lambda item: item[0])),
        queue_totals_ms=dict(sorted(queue_totals.items(), key=lambda item: item[0])),
    )


def _format_top_entries(entries: Iterable[Tuple[str, float]], limit: int) -> List[str]:
    sorted_entries = sorted(entries, key=lambda item: item[1], reverse=True)
    lines: List[str] = []
    for name, duration in sorted_entries[:limit]:
        lines.append(f"  - {name}: {duration:.3f} ms/frame")
    if not lines:
        lines.append("  (no samples)")
    return lines


def _render_capture(capture: Capture, *, top: int) -> List[str]:
    lines = [
        f"Scenario: {capture.scenario} ({capture.path})",
        f"  Task ID: {capture.task or 'n/a'}",
        f"  Clip: {capture.clip_name or '<unknown>'} (duration {capture.clip_duration:.3f} s)",
        f"  Tracks: {capture.track_count}  Rig joints: {capture.rig_joint_count}",
        f"  Frames sampled: {capture.summary.samples} (requested {capture.frames_requested})",
        f"  Timestep: {capture.timestep:.6f} s",
        f"  Average sample time: {capture.summary.mean_ms:.3f} ms ({capture.summary.fps:.2f} FPS)",
        capture.jitter_label,
    ]
    if capture.cuda_available is not None:
        lines.append(f"  CUDA dispatcher available: {'yes' if capture.cuda_available else 'no'}")
    if capture.jitter_exceeds_budget:
        lines.append("  WARNING: Frame jitter exceeds configured budget")

    if capture.dispatch_totals_ms:
        lines.append("  Top dispatches (mean per frame):")
        lines.extend(
            [f"    {entry}" for entry in _format_top_entries(capture.dispatch_totals_ms.items(), top)]
        )
    if capture.category_totals_ms:
        lines.append("  Top categories (mean per frame):")
        lines.extend(
            [f"    {entry}" for entry in _format_top_entries(capture.category_totals_ms.items(), top)]
        )
    if capture.queue_totals_ms:
        lines.append("  Queue utilisation (mean per frame):")
        lines.extend(
            [f"    {entry}" for entry in _format_top_entries(capture.queue_totals_ms.items(), top)]
        )

    return lines


def _render_speedups(captures: Sequence[Capture]) -> List[str]:
    baselines = [capture for capture in captures if capture.scenario.lower() == "cpu_baseline"]
    if not baselines:
        return []
    baseline = min(baselines, key=lambda capture: capture.summary.mean_ms)
    lines = [
        "Comparison vs CPU baseline:",
        f"  Baseline: {baseline.path} ({baseline.summary.mean_ms:.3f} ms)"
    ]
    for capture in captures:
        if capture is baseline:
            continue
        if capture.summary.mean_ms <= 0.0:
            continue
        speedup = baseline.summary.mean_ms / capture.summary.mean_ms if capture.summary.mean_ms > 0 else 0.0
        lines.append(
            f"  {capture.scenario}: {speedup:.3f}x speed-up (mean {capture.summary.mean_ms:.3f} ms)"
        )
    if len(lines) == 2:
        return []
    return lines


def render_report(captures: Sequence[Capture], *, top: int) -> str:
    ordered = sorted(
        captures,
        key=lambda capture: (
            {"cpu_baseline": 0, "gpu_async": 1}.get(capture.scenario.lower(), 2),
            capture.path.name,
        ),
    )
    sections: List[str] = ["Animation Sampling Benchmark Report", ""]
    for capture in ordered:
        sections.extend(_render_capture(capture, top=top))
        sections.append("")
    comparison = _render_speedups(ordered)
    if comparison:
        sections.extend(comparison)
        sections.append("")
    return "\n".join(section.rstrip() for section in sections).strip() + "\n"


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Summarise animation sampling benchmark telemetry (AN-230)."
    )
    parser.add_argument(
        "--input",
        dest="inputs",
        nargs="+",
        required=True,
        help="Path(s) to animation benchmark JSON telemetry captures.",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=5,
        help="Number of dispatch/category/queue entries to list (default: 5).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional file to persist the rendered summary.",
    )
    args = parser.parse_args(argv)

    captures = [_load_capture(Path(path)) for path in args.inputs]
    if not captures:
        parser.error("No input captures provided")
    report = render_report(captures, top=max(1, args.top))
    print(report, end="")
    if args.output:
        args.output.write_text(report, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
