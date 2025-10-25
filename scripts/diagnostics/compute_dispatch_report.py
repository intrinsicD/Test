"""Render summaries for runtime compute dispatcher captures.

The report consumes the JSON payload produced by
``engine_compute_runtime_sample`` and prints a textual overview of the
workload. Use it to highlight the most expensive kernels, aggregate
per-category timings, and enforce jitter thresholds as we mature the
`CO-170` runtime integration sample.
"""

from __future__ import annotations

import argparse
import json
import statistics
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, MutableMapping, Optional, Sequence


@dataclass
class DispatchSample:
    """Single kernel dispatch measurement."""

    name: str
    category: str
    duration_ms: float
    queue: str


@dataclass
class FrameSample:
    """Telemetry captured for a single frame."""

    index: int
    simulation_time: float
    timestep: float
    total_ms: float
    dispatches: List[DispatchSample]
    category_totals_ms: Dict[str, float]
    queue_totals_ms: Dict[str, float]


@dataclass
class SummaryStats:
    """Aggregated statistics used for kernels, categories, and frames."""

    samples: int
    mean_ms: float
    min_ms: float
    max_ms: float
    stddev_ms: float
    total_ms: float

    @property
    def jitter_percent(self) -> float:
        if self.mean_ms <= 0.0:
            return 0.0
        return (self.stddev_ms / self.mean_ms) * 100.0


@dataclass
class ReportSummary:
    dispatches: Dict[str, SummaryStats]
    categories: Dict[str, SummaryStats]
    queues: Dict[str, SummaryStats]
    frame_totals: SummaryStats
    queue_dependencies: List[MutableMapping[str, object]]
    queue_transitions: List[MutableMapping[str, object]]


@dataclass
class BaselineStats:
    frames: int
    queue_count: int
    average_frame_ms: float
    min_frame_ms: float
    max_frame_ms: float
    stddev_frame_ms: float
    jitter_percent: float
    speedup: float
    target_speedup: float


@dataclass
class ReportPayload:
    metadata: MutableMapping[str, object]
    frames: List[FrameSample]
    summary: ReportSummary
    stage_timings: List[MutableMapping[str, object]]
    baseline: Optional[BaselineStats]


def _load_frames(payload: MutableMapping[str, object]) -> List[FrameSample]:
    frames: List[FrameSample] = []
    for frame in payload.get("frames", []):
        dispatches = [
            DispatchSample(
                name=str(entry.get("name", "")),
                category=str(entry.get("category", "")),
                duration_ms=float(entry.get("duration_ms", 0.0)),
                queue=str(entry.get("queue", "queue-0")),
            )
            for entry in frame.get("dispatches", [])
        ]
        category_totals = {
            str(entry.get("category", "")): float(entry.get("duration_ms", 0.0))
            for entry in frame.get("category_totals_ms", [])
        }
        queue_totals = {
            str(entry.get("queue", "queue-0")): float(entry.get("duration_ms", 0.0))
            for entry in frame.get("queue_totals_ms", [])
        }
        frames.append(
            FrameSample(
                index=int(frame.get("index", len(frames))),
                simulation_time=float(frame.get("simulation_time", 0.0)),
                timestep=float(frame.get("timestep", 0.0)),
                total_ms=float(frame.get("total_ms", 0.0)),
                dispatches=dispatches,
                category_totals_ms=category_totals,
                queue_totals_ms=queue_totals,
            )
        )
    return frames


def _compute_stats(values: Sequence[float]) -> SummaryStats:
    if not values:
        return SummaryStats(0, 0.0, 0.0, 0.0, 0.0, 0.0)
    mean = statistics.fmean(values)
    deviation = statistics.pstdev(values) if len(values) > 1 else 0.0
    return SummaryStats(
        samples=len(values),
        mean_ms=mean,
        min_ms=min(values),
        max_ms=max(values),
        stddev_ms=deviation,
        total_ms=sum(values),
    )


def _compute_summary(frames: Sequence[FrameSample]) -> ReportSummary:
    dispatch_samples: Dict[str, List[float]] = {}
    category_samples: Dict[str, List[float]] = {}
    queue_samples: Dict[str, List[float]] = {}
    frame_totals = [frame.total_ms for frame in frames]

    for frame in frames:
        for dispatch in frame.dispatches:
            dispatch_samples.setdefault(dispatch.name, []).append(dispatch.duration_ms)
            category_samples.setdefault(dispatch.category, []).append(dispatch.duration_ms)
            queue_samples.setdefault(dispatch.queue, []).append(dispatch.duration_ms)

    dispatch_stats = {name: _compute_stats(values) for name, values in dispatch_samples.items()}
    category_stats = {name: _compute_stats(values) for name, values in category_samples.items()}
    queue_stats = {name: _compute_stats(values) for name, values in queue_samples.items()}
    frame_stats = _compute_stats(frame_totals)
    return ReportSummary(
        dispatches=dispatch_stats,
        categories=category_stats,
        queues=queue_stats,
        frame_totals=frame_stats,
        queue_dependencies=[],
        queue_transitions=[],
    )


def load_report(path: Path) -> ReportPayload:
    payload = json.loads(path.read_text(encoding="utf-8"))
    metadata = dict(payload.get("metadata", {}))
    frames = _load_frames(payload)
    summary = _compute_summary(frames)
    summary_payload = payload.get("summary", {})
    stage_timings = list(summary_payload.get("stage_timings", []))
    if isinstance(summary_payload.get("queue_dependencies"), list):
        parsed_dependencies: List[MutableMapping[str, object]] = []
        for entry in summary_payload.get("queue_dependencies", []):
            if isinstance(entry, MutableMapping):
                parsed_dependencies.append(
                    {
                        "from_queue": str(entry.get("from_queue", "")),
                        "to_queue": str(entry.get("to_queue", "")),
                        "edge_count": int(entry.get("edge_count", 0)),
                        "consumer_kernels": [str(name) for name in entry.get("consumer_kernels", [])],
                    }
                )
        summary.queue_dependencies = parsed_dependencies
    if isinstance(summary_payload.get("queue_transitions"), list):
        parsed_transitions: List[MutableMapping[str, object]] = []
        for entry in summary_payload.get("queue_transitions", []):
            if isinstance(entry, MutableMapping):
                parsed_transitions.append(
                    {
                        "producer": str(entry.get("producer", "")),
                        "consumer": str(entry.get("consumer", "")),
                        "from_queue": str(entry.get("from_queue", "")),
                        "to_queue": str(entry.get("to_queue", "")),
                    }
                )
        summary.queue_transitions = parsed_transitions
    baseline_payload = payload.get("baseline")
    baseline_stats: Optional[BaselineStats] = None
    if isinstance(baseline_payload, dict):
        frames_count = int(baseline_payload.get("frames", 0))
        queue_count = int(baseline_payload.get("queue_count", 0))
        average_frame_ms = float(baseline_payload.get("average_frame_ms", 0.0))
        min_frame_ms = float(baseline_payload.get("min_frame_ms", average_frame_ms))
        max_frame_ms = float(baseline_payload.get("max_frame_ms", average_frame_ms))
        stddev_frame_ms = float(baseline_payload.get("stddev_frame_ms", 0.0))
        jitter_percent = float(baseline_payload.get("jitter_percent", 0.0))
        speedup = float(baseline_payload.get("speedup", 0.0))
        target_speedup = float(baseline_payload.get("target_speedup", 0.0))
        baseline_stats = BaselineStats(
            frames=frames_count,
            queue_count=queue_count,
            average_frame_ms=average_frame_ms,
            min_frame_ms=min_frame_ms,
            max_frame_ms=max_frame_ms,
            stddev_frame_ms=stddev_frame_ms,
            jitter_percent=jitter_percent,
            speedup=speedup,
            target_speedup=target_speedup,
        )
    return ReportPayload(
        metadata=metadata,
        frames=frames,
        summary=summary,
        stage_timings=stage_timings,
        baseline=baseline_stats,
    )


def _format_stats(name: str, stats: SummaryStats, jitter_threshold: Optional[float]) -> str:
    jitter = stats.jitter_percent
    threshold = jitter_threshold if jitter_threshold is not None else 5.0
    jitter_text = (
        f" (jitter {jitter:.2f}%" + (" ⚠" if jitter_threshold is not None and jitter > threshold else "") + ")"
        if stats.samples
        else ""
    )
    return f"  - {name}: {stats.mean_ms:.3f} ms (min {stats.min_ms:.3f}, max {stats.max_ms:.3f}, σ {stats.stddev_ms:.3f}){jitter_text}"


def _render_summary(payload: ReportPayload, top: int, jitter_threshold: Optional[float]) -> str:
    lines: List[str] = []
    clock = payload.metadata.get("clock", {}) if isinstance(payload.metadata.get("clock"), dict) else {}
    clock_name = clock.get("name", "steady_clock") if isinstance(clock, dict) else "steady_clock"
    clock_domain = clock.get("domain", "cpu") if isinstance(clock, dict) else "cpu"
    timestep = payload.metadata.get("timestep", payload.frames[0].timestep if payload.frames else 0.0)
    requested_frames = int(payload.metadata.get("requested_frames", payload.summary.frame_totals.samples))
    workload = str(payload.metadata.get("workload", "balanced"))
    queue_names_raw = payload.metadata.get("queues", [])
    queue_names = [str(name) for name in queue_names_raw] if isinstance(queue_names_raw, list) else []
    queue_count_meta = payload.metadata.get("queue_count", None)
    queue_count = int(queue_count_meta) if isinstance(queue_count_meta, (int, float)) else (
        len(queue_names) if queue_names else max(len(payload.summary.queues), 1)
    )

    lines.append("Compute Dispatcher Report")
    lines.append(f"Frames: {payload.summary.frame_totals.samples}")
    lines.append(f"Requested frames: {requested_frames}")
    lines.append(f"Timestep: {timestep:.6f} s")
    lines.append(f"Clock: {clock_name} ({clock_domain})")
    lines.append(f"Workload: {workload}")
    if queue_names:
        lines.append(f"Queues: {queue_count} ({', '.join(queue_names)})")
    else:
        lines.append(f"Queues: {queue_count}")
    assignments_raw = payload.metadata.get("queue_assignments", [])
    assignments: List[str] = []
    if isinstance(assignments_raw, list):
        for entry in assignments_raw:
            if isinstance(entry, dict):
                category = str(entry.get("category", "")).strip()
                queue_name = str(entry.get("queue", "")).strip()
                if category and queue_name:
                    assignments.append(f"{category}→{queue_name}")
    if assignments:
        lines.append(f"Queue assignments: {', '.join(assignments)}")
    lines.append(f"Average frame dispatch time: {payload.summary.frame_totals.mean_ms:.3f} ms")

    if payload.baseline is not None:
        baseline = payload.baseline
        lines.append("")
        queue_label = "queue" if baseline.queue_count == 1 else "queues"
        lines.append(
            (
                f"Baseline frame time ({baseline.queue_count} {queue_label}): "
                f"{baseline.average_frame_ms:.3f} ms (min {baseline.min_frame_ms:.3f}, max {baseline.max_frame_ms:.3f}, "
                f"σ {baseline.stddev_frame_ms:.3f}, jitter {baseline.jitter_percent:.2f}%)"
            )
        )
        lines.append(
            f"Speed-up vs baseline: {baseline.speedup:.3f}x (target {baseline.target_speedup:.2f}x)"
        )
        if baseline.speedup < baseline.target_speedup:
            lines.append("WARNING: Speed-up below performance target")

    if payload.summary.dispatches:
        lines.append("")
        lines.append("Top kernels by mean duration:")
        ordered = sorted(
            payload.summary.dispatches.items(),
            key=lambda item: item[1].mean_ms,
            reverse=True,
        )
        for name, stats in ordered[:top]:
            lines.append(_format_stats(name, stats, jitter_threshold))

    if payload.summary.categories:
        lines.append("")
        lines.append("Category totals:")
        for name, stats in sorted(
            payload.summary.categories.items(),
            key=lambda item: item[1].total_ms,
            reverse=True,
        ):
            lines.append(
                f"  - {name}: total {stats.total_ms:.3f} ms across {stats.samples} samples"
            )

    if payload.summary.queues:
        lines.append("")
        lines.append("Queue totals:")
        for name, stats in sorted(
            payload.summary.queues.items(),
            key=lambda item: item[1].total_ms,
            reverse=True,
        ):
            lines.append(
                f"  - {name}: total {stats.total_ms:.3f} ms across {stats.samples} samples"
            )

    if payload.summary.queue_dependencies:
        lines.append("")
        lines.append("Cross-queue synchronization:")
        for entry in payload.summary.queue_dependencies:
            from_queue = str(entry.get("from_queue", ""))
            to_queue = str(entry.get("to_queue", ""))
            edge_count = int(entry.get("edge_count", 0))
            consumers = [str(name) for name in entry.get("consumer_kernels", [])]
            consumer_text = f" (consumers: {', '.join(consumers)})" if consumers else ""
            lines.append(
                f"  - {from_queue} -> {to_queue}: {edge_count} dependencies{consumer_text}"
            )

    if payload.stage_timings:
        lines.append("")
        lines.append("Runtime stage timings:")
        for entry in payload.stage_timings:
            name = str(entry.get("name", "stage"))
            last_ms = float(entry.get("last_ms", 0.0))
            avg_ms = float(entry.get("average_ms", 0.0))
            max_ms = float(entry.get("max_ms", 0.0))
            samples = int(entry.get("samples", 0))
            lines.append(
                f"  - {name}: last {last_ms:.3f} ms, avg {avg_ms:.3f} ms, max {max_ms:.3f} ms ({samples} samples)"
            )

    if jitter_threshold is not None:
        exceeding = [
            (name, stats)
            for name, stats in payload.summary.dispatches.items()
            if stats.jitter_percent > jitter_threshold
        ]
        if exceeding:
            lines.append("")
            lines.append(
                f"WARNING: {len(exceeding)} kernels exceed jitter threshold {jitter_threshold:.2f}%"
            )
            for name, stats in sorted(exceeding, key=lambda item: item[1].jitter_percent, reverse=True):
                lines.append(
                    f"  - {name}: jitter {stats.jitter_percent:.2f}% (σ {stats.stddev_ms:.3f} ms)"
                )

    return "\n".join(lines)


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Summarise compute dispatcher telemetry captured by the runtime sample."
    )
    parser.add_argument("--input", type=Path, required=True, help="Path to the JSON payload")
    parser.add_argument(
        "--top", type=int, default=5, help="Number of kernels to list when reporting averages"
    )
    parser.add_argument(
        "--jitter-threshold",
        type=float,
        default=None,
        help="Warn when jitter exceeds the specified percent (default: disabled)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional file to write the rendered report",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> None:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    payload = load_report(args.input)
    report = _render_summary(payload, top=max(args.top, 1), jitter_threshold=args.jitter_threshold)

    print(report)
    if args.output:
        args.output.write_text(report + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
