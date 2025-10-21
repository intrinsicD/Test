"""Render summary tables for geometry normal recomputation benchmarks."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Optional, Sequence


_METRIC_LABELS: Mapping[str, str] = {
    "duration_seconds": "Duration (s)",
    "iterations_per_second": "Iterations / second",
    "vertices_per_second": "Vertices / second",
    "triangles_per_second": "Triangles / second",
    "normal_checksum": "Normal checksum",
}


@dataclass(frozen=True)
class BenchmarkRun:
    label: str
    source: Path
    metrics: Mapping[str, float]
    config: Mapping[str, float]


class BenchmarkError(Exception):
    """Custom exit used for predictable error handling in tests."""


def _parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render summary tables for geometry normal recompute benchmark telemetry.",
    )
    parser.add_argument(
        "--current",
        type=Path,
        required=True,
        help="Path to the latest geometry_normals_benchmark JSON payload.",
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        help="Optional baseline JSON payload for comparison.",
    )
    parser.add_argument(
        "--current-label",
        help="Override label used when printing the current benchmark run.",
    )
    parser.add_argument(
        "--baseline-label",
        help="Override label used when printing the baseline benchmark run.",
    )
    return parser.parse_args(argv)


def _load_json(path: Path) -> Mapping[str, object]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:  # pragma: no cover - exercised via CLI contract
        raise BenchmarkError(f"error: benchmark file not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise BenchmarkError(f"error: failed to parse JSON payload: {exc}") from exc
    if not isinstance(raw, Mapping):
        raise BenchmarkError("error: benchmark payload must be a JSON object")
    return raw


def _coerce_mapping(payload: Mapping[str, object], key: str) -> Mapping[str, float]:
    value = payload.get(key)
    if not isinstance(value, Mapping):
        raise BenchmarkError(f"error: benchmark payload missing '{key}' object")
    coerced: dict[str, float] = {}
    for entry_key, entry_value in value.items():
        if isinstance(entry_value, (int, float)):
            coerced[str(entry_key)] = float(entry_value)
            continue
        try:
            coerced[str(entry_key)] = float(entry_value)
        except (TypeError, ValueError) as exc:
            raise BenchmarkError(
                f"error: expected numeric value for '{key}.{entry_key}'",
            ) from exc
    return coerced


def _load_run(path: Path, label_override: Optional[str]) -> BenchmarkRun:
    payload = _load_json(path)
    benchmark_name = str(payload.get("benchmark", ""))
    if benchmark_name and benchmark_name != "geometry_normal_recompute":
        raise BenchmarkError(
            "error: benchmark payload must describe 'geometry_normal_recompute'",
        )
    metrics = _coerce_mapping(payload, "metrics")
    config = _coerce_mapping(payload, "config")
    label = label_override or path.stem
    return BenchmarkRun(label=label, source=path, metrics=metrics, config=config)


def _format_config(config: Mapping[str, float]) -> str:
    order = ["resolution", "iterations", "vertex_count", "triangle_count"]
    lines = ["Configuration", "--------------"]
    for key in order:
        if key not in config:
            continue
        value = config[key]
        if key in {"vertex_count", "triangle_count"}:
            lines.append(f"{key.replace('_', ' ').title()}: {value:.0f}")
        else:
            lines.append(f"{key.replace('_', ' ').title()}: {value:.0f}")
    extras = sorted(set(config) - set(order))
    for key in extras:
        lines.append(f"{key}: {config[key]}")
    return "\n".join(lines)


def _format_metrics(current: BenchmarkRun, baseline: Optional[BenchmarkRun]) -> str:
    lines = ["Metrics", "-------"]
    for key, label in _METRIC_LABELS.items():
        if key not in current.metrics:
            continue
        current_value = current.metrics[key]
        line = f"{label}: {current_value:.4f}"
        if baseline is not None and key in baseline.metrics:
            baseline_value = baseline.metrics[key]
            delta = current_value - baseline_value
            if baseline_value != 0.0:
                percent = (delta / baseline_value) * 100.0
                line += f" (Δ {delta:+.4f}, {percent:+.2f}%)"
            else:
                line += f" (Δ {delta:+.4f})"
        lines.append(line)
    missing = sorted(set(current.metrics) - set(_METRIC_LABELS))
    for key in missing:
        current_value = current.metrics[key]
        line = f"{key}: {current_value:.4f}"
        if baseline is not None and key in baseline.metrics:
            baseline_value = baseline.metrics[key]
            delta = current_value - baseline_value
            if baseline_value != 0.0:
                percent = (delta / baseline_value) * 100.0
                line += f" (Δ {delta:+.4f}, {percent:+.2f}%)"
            else:
                line += f" (Δ {delta:+.4f})"
        lines.append(line)
    return "\n".join(lines)


def _render_report(current: BenchmarkRun, baseline: Optional[BenchmarkRun]) -> str:
    header = [
        "Geometry Normals Benchmark Report",
        "==================================",
        f"Current run: {current.label} ({current.source})",
    ]
    if baseline is not None:
        header.append(f"Baseline: {baseline.label} ({baseline.source})")
    header.append("")
    body = [
        _format_config(current.config),
        "",
        _format_metrics(current, baseline),
    ]
    return "\n".join(header + body) + "\n"


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parse_args(argv)
    try:
        current = _load_run(args.current, args.current_label)
        baseline = _load_run(args.baseline, args.baseline_label) if args.baseline else None
    except BenchmarkError as exc:
        print(exc, flush=True)
        return 1
    report = _render_report(current, baseline)
    print(report, end="")
    return 0


if __name__ == "__main__":  # pragma: no cover - manual invocation
    raise SystemExit(main())
