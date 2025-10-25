"""Automate runtime dispatcher benchmark captures and variance analysis.

The runtime integration sample (`engine_compute_runtime_sample`) already
emits detailed telemetry and exposes `--repeat` to record multiple captures.
This helper orchestrates those captures (or consumes previously recorded
payloads), computes run-to-run variance, and surfaces warnings whenever the
observed variance or jitter budgets drift beyond their targets.  It is
designed to close the remaining acceptance criteria for `CO-170` by making
the ≤2 % variance requirement actionable in CI and local workflows.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import statistics
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Mapping, Optional, Sequence


class BenchmarkError(Exception):
    """Custom error to keep failure handling predictable for tests."""


@dataclass(frozen=True)
class RunSummary:
    source: Path
    mean_frame_ms: float
    frame_stddev_ms: float
    frame_count: int
    jitter_ms: Optional[float]
    jitter_budget_ms: Optional[float]
    jitter_exceeded: bool
    baseline_speedup: Optional[float]
    baseline_target: Optional[float]
    baseline_jitter_ms: Optional[float]
    baseline_jitter_budget_ms: Optional[float]
    baseline_jitter_exceeded: bool


def _parse_queue_map(values: Iterable[str]) -> List[str]:
    mappings: List[str] = []
    for value in values:
        if "=" not in value:
            raise BenchmarkError(f"queue map must use category=queue syntax: {value}")
        mappings.append(value)
    return mappings


def _parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture runtime dispatcher telemetry and compute variance statistics."
    )
    parser.add_argument(
        "--sample",
        type=Path,
        default=None,
        help="Path to engine_compute_runtime_sample. When omitted, --input must be provided.",
    )
    parser.add_argument(
        "--input",
        type=Path,
        nargs="+",
        default=None,
        help="Existing telemetry payload(s) to analyse instead of running the sample.",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=3,
        help="Number of captures to record when executing the sample (default: 3).",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=1024,
        help="Frame count per capture when executing the sample (default: 1024).",
    )
    parser.add_argument(
        "--dt",
        type=float,
        default=0.016,
        help="Timestep in seconds per frame (default: 0.016).",
    )
    parser.add_argument(
        "--workload",
        default="balanced",
        help="Workload profile passed to the sample (default: balanced).",
    )
    parser.add_argument(
        "--queues",
        type=int,
        default=3,
        help="Logical queue count supplied to the sample (default: 3).",
    )
    parser.add_argument(
        "--dispatcher-backend",
        default="cpu",
        help="Dispatcher backend to request (default: cpu).",
    )
    parser.add_argument(
        "--jitter-budget-ms",
        type=float,
        default=None,
        help="Override the jitter budget forwarded to the sample (optional).",
    )
    parser.add_argument(
        "--queue-names",
        default=None,
        help="Comma separated queue names forwarded to the sample (optional).",
    )
    parser.add_argument(
        "--queue-map",
        action="append",
        default=None,
        help="Repeatable category=queue overrides forwarded to the sample.",
    )
    parser.add_argument(
        "--baseline",
        action="store_true",
        help="Request the sample to capture a single-queue baseline as well.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("telemetry"),
        help="Directory used to store telemetry captures (default: ./telemetry).",
    )
    parser.add_argument(
        "--prefix",
        default="compute_dispatch",
        help="Filename prefix for captured telemetry (default: compute_dispatch).",
    )
    parser.add_argument(
        "--variance-threshold",
        type=float,
        default=2.0,
        help="Warn when run-to-run variance exceeds this percent (default: 2.0).",
    )
    parser.add_argument(
        "--exit-on-regression",
        action="store_true",
        help="Return a non-zero exit status when warnings are emitted.",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=None,
        help="Optional file to persist the rendered report.",
    )
    parser.add_argument(
        "--env",
        action="append",
        default=None,
        help="Optional KEY=VALUE environment overrides when executing the sample.",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=None,
        help="Optional timeout (seconds) for each sample execution.",
    )
    return parser.parse_args(argv)


def _load_payload(path: Path) -> Mapping[str, object]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:  # pragma: no cover - exercised in integration usage
        raise BenchmarkError(f"telemetry payload not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise BenchmarkError(f"telemetry payload is not valid JSON: {path}") from exc

    if not isinstance(data, dict):
        raise BenchmarkError(f"telemetry payload must be a JSON object: {path}")
    if "frames" not in data or not isinstance(data["frames"], list):
        raise BenchmarkError(f"telemetry payload missing 'frames' array: {path}")
    return data


def _summarise_run(path: Path) -> RunSummary:
    payload = _load_payload(path)
    frames = payload.get("frames", [])
    if not frames:
        raise BenchmarkError(f"telemetry payload contains no frames: {path}")

    totals: List[float] = []
    for frame in frames:
        if not isinstance(frame, Mapping) or "total_ms" not in frame:
            raise BenchmarkError(f"frame record missing 'total_ms': {path}")
        totals.append(float(frame["total_ms"]))

    mean = statistics.fmean(totals)
    stddev = statistics.pstdev(totals, mu=mean) if len(totals) > 1 else 0.0

    metadata = payload.get("metadata", {})
    jitter_ms = None
    jitter_budget_ms = None
    jitter_exceeded = False
    if isinstance(metadata, Mapping):
        jitter_ms = float(metadata.get("frame_jitter_ms")) if metadata.get("frame_jitter_ms") is not None else None
        jitter_budget_ms = (
            float(metadata.get("frame_jitter_budget_ms")) if metadata.get("frame_jitter_budget_ms") is not None else None
        )
        jitter_exceeded = bool(metadata.get("frame_jitter_exceeds_budget", False))

    baseline = payload.get("baseline")
    baseline_speedup = None
    baseline_target = None
    baseline_jitter_ms = None
    baseline_jitter_budget_ms = None
    baseline_jitter_exceeded = False
    if isinstance(baseline, Mapping):
        baseline_speedup = float(baseline.get("speedup")) if baseline.get("speedup") is not None else None
        baseline_target = float(baseline.get("target_speedup")) if baseline.get("target_speedup") is not None else None
        baseline_jitter_ms = (
            float(baseline.get("stddev_frame_ms")) if baseline.get("stddev_frame_ms") is not None else None
        )
        baseline_jitter_budget_ms = (
            float(baseline.get("jitter_budget_ms")) if baseline.get("jitter_budget_ms") is not None else None
        )
        baseline_jitter_exceeded = bool(baseline.get("jitter_exceeds_budget", False))

    return RunSummary(
        source=path,
        mean_frame_ms=mean,
        frame_stddev_ms=stddev,
        frame_count=len(totals),
        jitter_ms=jitter_ms,
        jitter_budget_ms=jitter_budget_ms,
        jitter_exceeded=jitter_exceeded,
        baseline_speedup=baseline_speedup,
        baseline_target=baseline_target,
        baseline_jitter_ms=baseline_jitter_ms,
        baseline_jitter_budget_ms=baseline_jitter_budget_ms,
        baseline_jitter_exceeded=baseline_jitter_exceeded,
    )


def _execute_sample(
    *,
    sample: Path,
    runs: int,
    output_dir: Path,
    prefix: str,
    frames: int,
    dt: float,
    workload: str,
    queues: int,
    dispatcher_backend: str,
    jitter_budget_ms: Optional[float],
    queue_names: Optional[str],
    queue_map: Optional[List[str]],
    baseline: bool,
    env_overrides: Optional[List[str]],
    timeout: Optional[float],
) -> List[Path]:
    if runs <= 0:
        raise BenchmarkError("--runs must be greater than zero when executing the sample")

    output_dir.mkdir(parents=True, exist_ok=True)

    environment = os.environ.copy()
    if env_overrides:
        for entry in env_overrides:
            if "=" not in entry:
                raise BenchmarkError(f"environment overrides must be KEY=VALUE: {entry}")
            key, value = entry.split("=", maxsplit=1)
            environment[key] = value

    queue_map_flags = _parse_queue_map(queue_map or [])

    outputs: List[Path] = []
    for index in range(1, runs + 1):
        output_path = output_dir / f"{prefix}-run{index:02d}.json"
        cmd: List[str] = [
            str(sample),
            "--frames",
            str(frames),
            "--dt",
            f"{dt:.6f}",
            "--workload",
            workload,
            "--queues",
            str(queues),
            "--dispatcher-backend",
            dispatcher_backend,
            "--output",
            str(output_path),
            "--pretty",
        ]
        if jitter_budget_ms is not None:
            cmd.extend(["--jitter-budget-ms", f"{jitter_budget_ms:.6f}"])
        if queue_names:
            cmd.extend(["--queue-names", queue_names])
        for mapping in queue_map_flags:
            cmd.extend(["--queue-map", mapping])
        if baseline:
            cmd.append("--baseline")

        try:
            subprocess.run(cmd, check=True, env=environment, timeout=timeout)
        except FileNotFoundError as exc:
            raise BenchmarkError(f"sample executable not found: {sample}") from exc
        except subprocess.CalledProcessError as exc:
            raise BenchmarkError(f"sample execution failed with exit code {exc.returncode}") from exc

        outputs.append(output_path)

    return outputs


def _render_summary(runs: Sequence[RunSummary], variance_threshold: float) -> (str, List[str]):
    if not runs:
        raise BenchmarkError("no telemetry payloads were provided")

    means = [run.mean_frame_ms for run in runs]
    mean_of_means = statistics.fmean(means)
    stddev_between = statistics.pstdev(means, mu=mean_of_means) if len(means) > 1 else 0.0
    variance_percent = (stddev_between / mean_of_means * 100.0) if mean_of_means > 0.0 else 0.0

    lines = [
        "Compute Dispatcher Benchmark",
        "============================",
        f"Runs analysed: {len(runs)}",
        f"Mean frame time across runs: {mean_of_means:.3f} ms",
        f"Run-to-run standard deviation: {stddev_between:.3f} ms ({variance_percent:.2f}%)",
    ]
    if variance_threshold > 0.0:
        lines.append(f"Variance threshold: {variance_threshold:.2f}%")

    frame_counts = {run.frame_count for run in runs}
    if len(frame_counts) == 1:
        lines.append(f"Frames per run: {next(iter(frame_counts))}")

    if any(run.baseline_speedup is not None for run in runs):
        speeds = [run.baseline_speedup for run in runs if run.baseline_speedup is not None]
        if speeds:
            lines.append(
                "Average speed-up vs baseline: "
                f"{statistics.fmean(speeds):.3f}x (min {min(speeds):.3f}x, max {max(speeds):.3f}x)"
            )

    if any(run.jitter_ms is not None for run in runs):
        jitters = [run.jitter_ms for run in runs if run.jitter_ms is not None]
        lines.append(
            "Average frame jitter σ: "
            f"{statistics.fmean(jitters):.3f} ms (max {max(jitters):.3f} ms)"
        )

    lines.append("")
    lines.append("Per-run details:")
    lines.append("Run  Mean (ms)  σ within run (ms)  Jitter (ms)  Speed-up  Source")
    for index, run in enumerate(runs, start=1):
        jitter_str = "-" if run.jitter_ms is None else f"{run.jitter_ms:.3f}"
        speedup_str = "-" if run.baseline_speedup is None else f"{run.baseline_speedup:.3f}"
        lines.append(
            f"{index:>3}  {run.mean_frame_ms:9.3f}  {run.frame_stddev_ms:17.3f}  "
            f"{jitter_str:>10}  {speedup_str:>7}  {run.source}"
        )

    warnings: List[str] = []
    if variance_threshold > 0.0 and variance_percent > variance_threshold + 1e-6:
        warnings.append(
            f"WARNING: Run-to-run variance {variance_percent:.2f}% exceeds threshold {variance_threshold:.2f}%"
        )

    for index, run in enumerate(runs, start=1):
        if run.jitter_exceeded and run.jitter_ms is not None and run.jitter_budget_ms is not None:
            warnings.append(
                f"WARNING: Run {index} frame jitter {run.jitter_ms:.3f} ms exceeds budget {run.jitter_budget_ms:.3f} ms"
            )
        if (
            run.baseline_speedup is not None
            and run.baseline_target is not None
            and run.baseline_speedup + 1e-6 < run.baseline_target
        ):
            warnings.append(
                f"WARNING: Run {index} baseline speed-up {run.baseline_speedup:.3f}x falls below target {run.baseline_target:.3f}x"
            )
        if (
            run.baseline_jitter_exceeded
            and run.baseline_jitter_ms is not None
            and run.baseline_jitter_budget_ms is not None
        ):
            warnings.append(
                f"WARNING: Run {index} baseline jitter {run.baseline_jitter_ms:.3f} ms exceeds budget "
                f"{run.baseline_jitter_budget_ms:.3f} ms"
            )

    if warnings:
        lines.append("")
        lines.extend(warnings)

    return "\n".join(lines) + "\n", warnings


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parse_args(argv)

    try:
        captured: List[Path] = []
        if args.input:
            captured.extend(args.input)
        if args.sample is not None:
            captured.extend(
                _execute_sample(
                    sample=args.sample,
                    runs=args.runs,
                    output_dir=args.output_dir,
                    prefix=args.prefix,
                    frames=args.frames,
                    dt=args.dt,
                    workload=args.workload,
                    queues=args.queues,
                    dispatcher_backend=args.dispatcher_backend,
                    jitter_budget_ms=args.jitter_budget_ms,
                    queue_names=args.queue_names,
                    queue_map=args.queue_map,
                    baseline=args.baseline,
                    env_overrides=args.env,
                    timeout=args.timeout,
                )
            )
        if not captured:
            raise BenchmarkError("provide --sample and/or --input telemetry to analyse")

        runs = [_summarise_run(path) for path in captured]
        report, warnings = _render_summary(runs, args.variance_threshold)
    except BenchmarkError as exc:
        print(exc, flush=True)
        return 1

    print(report, end="")
    if args.report:
        args.report.write_text(report, encoding="utf-8")

    if args.exit_on_regression and warnings:
        return 2
    return 0


if __name__ == "__main__":  # pragma: no cover - manual invocation
    raise SystemExit(main())
