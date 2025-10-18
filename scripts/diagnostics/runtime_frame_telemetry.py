#!/usr/bin/env python3
"""Runtime telemetry capture for animation/physics ↔ rendering hand-off.

This utility loads the runtime shared library produced by the engine build and
collects per-kernel execution timings from the dispatcher that drives the
animation → physics → geometry chain required by the rendering vertical slice.
The metrics are intended for regression tracking of the `AI-003` / `RT-003`
vertical slice that links RuntimeHost to the rendering frame graph.

Example usage (after building the engine with a shared runtime library)::

    python scripts/diagnostics/runtime_frame_telemetry.py \
        --library-dir build/linux-clang-debug

Set ``--frames`` to record multiple consecutive frames and ``--output`` to
persist the structured telemetry as JSON for later comparison. ``--window-backend``
can be used to force the mock window system in headless environments, while
``--variance-check geometry.deform:5`` asserts that the per-frame skinning cost
remains stable (≤5% coefficient of variation in this example). Combine with
``--variance-trim 0.1`` to discard the lowest and highest 10% of samples when
computing variance, which is useful for ignoring warm-up transients.
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
import os
import statistics
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, MutableMapping, Optional, Sequence


@dataclass
class DispatchSample:
    """Single dispatcher kernel sample captured from the runtime."""

    name: str
    duration_ms: float
    category: str


@dataclass
class FrameSample:
    """Telemetry captured for a single frame."""

    index: int
    simulation_time: float
    timestep: float
    dispatches: List[DispatchSample]
    category_totals_ms: Dict[str, float]
    frame_total_ms: float


@dataclass(frozen=True)
class VarianceCheck:
    """Specification for verifying per-dispatch timing stability."""

    dispatch_name: str
    max_percent: float
    trim_fraction: float = 0.0


@dataclass(frozen=True)
class VarianceResult:
    """Outcome of a variance check including descriptive statistics."""

    check: VarianceCheck
    durations_ms: Sequence[float]
    mean_ms: float
    stdev_ms: float
    percent: float
    total_samples: int

    @property
    def passed(self) -> bool:
        return self.percent <= self.check.max_percent


@dataclass
class RuntimeStageMetric:
    """Lifecycle telemetry for a dispatcher stage captured via the runtime C API."""

    name: str
    last_ms: float
    average_ms: float
    max_ms: float
    sample_count: int


@dataclass
class RuntimeSubsystemMetric:
    """Lifecycle timings for a subsystem plugin during initialize/tick/shutdown."""

    name: str
    last_initialize_ms: float
    last_tick_ms: float
    last_shutdown_ms: float
    max_initialize_ms: float
    max_tick_ms: float
    max_shutdown_ms: float
    initialize_count: int
    tick_count: int
    shutdown_count: int


@dataclass
class RuntimeDiagnosticsSnapshot:
    """Aggregated runtime lifecycle diagnostics exposed through the C ABI."""

    initialize_count: int
    shutdown_count: int
    tick_count: int
    last_initialize_ms: float
    last_shutdown_ms: float
    last_tick_ms: float
    average_tick_ms: float
    max_tick_ms: float
    stages: List[RuntimeStageMetric]
    subsystems: List[RuntimeSubsystemMetric]


class RuntimeBindings:
    """Thin ctypes wrapper around the runtime C API."""

    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
        self._has_simulation_time = False
        self._has_diagnostics = False
        self._configure_signatures()

    @staticmethod
    def load(name: str, directory: Optional[Path]) -> "RuntimeBindings":
        """Load the runtime shared library from ``directory`` if provided."""

        candidates = tuple(_candidate_names(name))
        errors: List[Exception] = []
        search_paths: Sequence[Path]
        if directory is not None:
            search_paths = [directory]
        else:
            search_paths = [Path.cwd()]

        for base in search_paths:
            for candidate in candidates:
                try:
                    library = ctypes.CDLL(str(base / candidate))
                    return RuntimeBindings(library)
                except OSError as exc:  # pragma: no cover - exercised at runtime
                    errors.append(exc)

        # Fall back to system lookup if directory did not resolve.
        try:
            resolved = ctypes.util.find_library(name)  # type: ignore[attr-defined]
        except AttributeError as exc:  # pragma: no cover - Python <3.8 compat path
            errors.append(exc)
        else:
            if resolved is not None:
                try:
                    return RuntimeBindings(ctypes.CDLL(resolved))
                except OSError as exc:  # pragma: no cover - exercised at runtime
                    errors.append(exc)

        message = [
            f"Unable to load runtime library '{name}'. Tried candidates: {candidates}",
        ]
        if directory is not None:
            message.append(f" within directory '{directory}'.")
        for err in errors:
            message.append(f"\n- {err}")
        raise RuntimeError("".join(message))

    def _configure_signatures(self) -> None:
        lib = self._lib
        lib.engine_runtime_configure_with_default_modules.restype = None
        lib.engine_runtime_initialize.restype = None
        lib.engine_runtime_shutdown.restype = None
        lib.engine_runtime_tick.restype = None
        lib.engine_runtime_tick.argtypes = [ctypes.c_double]
        lib.engine_runtime_dispatch_count.restype = ctypes.c_size_t
        lib.engine_runtime_dispatch_count.argtypes = []
        lib.engine_runtime_dispatch_name.restype = ctypes.c_char_p
        lib.engine_runtime_dispatch_name.argtypes = [ctypes.c_size_t]
        lib.engine_runtime_dispatch_duration.restype = ctypes.c_double
        lib.engine_runtime_dispatch_duration.argtypes = [ctypes.c_size_t]
        lib.engine_runtime_scene_node_count.restype = ctypes.c_size_t
        lib.engine_runtime_scene_node_name.restype = ctypes.c_char_p
        lib.engine_runtime_scene_node_name.argtypes = [ctypes.c_size_t]
        try:
            lib.engine_runtime_simulation_time.restype = ctypes.c_double
        except AttributeError:
            self._has_simulation_time = False
        else:
            self._has_simulation_time = True
        try:
            lib.engine_runtime_diagnostic_initialize_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_initialize_count.argtypes = []
            lib.engine_runtime_diagnostic_shutdown_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_shutdown_count.argtypes = []
            lib.engine_runtime_diagnostic_tick_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_tick_count.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_last_initialize_ms.argtypes = []
            lib.engine_runtime_diagnostic_last_shutdown_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_last_shutdown_ms.argtypes = []
            lib.engine_runtime_diagnostic_last_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_last_tick_ms.argtypes = []
            lib.engine_runtime_diagnostic_average_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_average_tick_ms.argtypes = []
            lib.engine_runtime_diagnostic_max_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_max_tick_ms.argtypes = []
            lib.engine_runtime_diagnostic_stage_count.restype = ctypes.c_size_t
            lib.engine_runtime_diagnostic_stage_count.argtypes = []
            lib.engine_runtime_diagnostic_stage_name.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_stage_name.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_stage_last_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_stage_last_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_stage_average_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_stage_average_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_stage_max_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_stage_max_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_stage_samples.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_stage_samples.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_count.restype = ctypes.c_size_t
            lib.engine_runtime_diagnostic_subsystem_count.argtypes = []
            lib.engine_runtime_diagnostic_subsystem_name.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_subsystem_name.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_initialize_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_last_initialize_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_last_tick_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_shutdown_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_last_shutdown_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_initialize_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_subsystem_initialize_count.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_tick_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_subsystem_tick_count.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_shutdown_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_subsystem_shutdown_count.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_initialize_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_initialize_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_tick_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_shutdown_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_shutdown_ms.argtypes = [ctypes.c_size_t]
        except AttributeError:
            self._has_diagnostics = False
        else:
            self._has_diagnostics = True

    def configure_default_modules(self) -> None:
        self._lib.engine_runtime_configure_with_default_modules()

    def initialize(self) -> None:
        self._lib.engine_runtime_initialize()

    def shutdown(self) -> None:
        self._lib.engine_runtime_shutdown()

    def tick(self, dt: float) -> None:
        self._lib.engine_runtime_tick(dt)

    def dispatch_count(self) -> int:
        return int(self._lib.engine_runtime_dispatch_count())

    def dispatch_name(self, index: int) -> str:
        raw = self._lib.engine_runtime_dispatch_name(index)
        return raw.decode("utf-8") if raw else ""

    def dispatch_duration_ms(self, index: int) -> float:
        return float(self._lib.engine_runtime_dispatch_duration(index) * 1000.0)

    def simulation_time(self) -> float:
        if not self._has_simulation_time:
            raise RuntimeError("Runtime library does not expose simulation_time().")
        return float(self._lib.engine_runtime_simulation_time())

    @property
    def has_simulation_time(self) -> bool:
        return self._has_simulation_time

    @property
    def has_diagnostics(self) -> bool:
        return self._has_diagnostics

    def diagnostics_snapshot(self) -> Optional[RuntimeDiagnosticsSnapshot]:
        if not self._has_diagnostics:
            return None
        return RuntimeDiagnosticsSnapshot(
            initialize_count=int(self._lib.engine_runtime_diagnostic_initialize_count()),
            shutdown_count=int(self._lib.engine_runtime_diagnostic_shutdown_count()),
            tick_count=int(self._lib.engine_runtime_diagnostic_tick_count()),
            last_initialize_ms=float(self._lib.engine_runtime_diagnostic_last_initialize_ms()),
            last_shutdown_ms=float(self._lib.engine_runtime_diagnostic_last_shutdown_ms()),
            last_tick_ms=float(self._lib.engine_runtime_diagnostic_last_tick_ms()),
            average_tick_ms=float(self._lib.engine_runtime_diagnostic_average_tick_ms()),
            max_tick_ms=float(self._lib.engine_runtime_diagnostic_max_tick_ms()),
            stages=self._collect_stage_metrics(),
            subsystems=self._collect_subsystem_metrics(),
        )

    def _collect_stage_metrics(self) -> List[RuntimeStageMetric]:
        metrics: List[RuntimeStageMetric] = []
        count = int(self._lib.engine_runtime_diagnostic_stage_count())
        for index in range(count):
            raw_name = self._lib.engine_runtime_diagnostic_stage_name(index)
            name = raw_name.decode("utf-8") if raw_name else ""
            metrics.append(
                RuntimeStageMetric(
                    name=name,
                    last_ms=float(self._lib.engine_runtime_diagnostic_stage_last_ms(index)),
                    average_ms=float(self._lib.engine_runtime_diagnostic_stage_average_ms(index)),
                    max_ms=float(self._lib.engine_runtime_diagnostic_stage_max_ms(index)),
                    sample_count=int(self._lib.engine_runtime_diagnostic_stage_samples(index)),
                )
            )
        return metrics

    def _collect_subsystem_metrics(self) -> List[RuntimeSubsystemMetric]:
        metrics: List[RuntimeSubsystemMetric] = []
        count = int(self._lib.engine_runtime_diagnostic_subsystem_count())
        for index in range(count):
            raw_name = self._lib.engine_runtime_diagnostic_subsystem_name(index)
            name = raw_name.decode("utf-8") if raw_name else ""
            metrics.append(
                RuntimeSubsystemMetric(
                    name=name,
                    last_initialize_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_last_initialize_ms(index)
                    ),
                    last_tick_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_last_tick_ms(index)
                    ),
                    last_shutdown_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_last_shutdown_ms(index)
                    ),
                    max_initialize_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_max_initialize_ms(index)
                    ),
                    max_tick_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_max_tick_ms(index)
                    ),
                    max_shutdown_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_max_shutdown_ms(index)
                    ),
                    initialize_count=int(
                        self._lib.engine_runtime_diagnostic_subsystem_initialize_count(index)
                    ),
                    tick_count=int(
                        self._lib.engine_runtime_diagnostic_subsystem_tick_count(index)
                    ),
                    shutdown_count=int(
                        self._lib.engine_runtime_diagnostic_subsystem_shutdown_count(index)
                    ),
                )
            )
        return metrics


def _candidate_names(base: str) -> Iterable[str]:
    if sys.platform == "win32":
        yield f"{base}.dll"
    elif sys.platform == "darwin":
        yield f"lib{base}.dylib"
        yield f"{base}.dylib"
    else:
        yield f"lib{base}.so"
        yield f"{base}.so"


def _categorise_dispatch(name: str) -> str:
    if not name:
        return "unknown"
    if "." in name:
        return name.split(".", maxsplit=1)[0]
    return name


def capture_frames(bindings: RuntimeBindings, frames: int, dt: float) -> Sequence[FrameSample]:
    samples: List[FrameSample] = []
    fallback_simulation_time = 0.0
    for frame_index in range(frames):
        bindings.tick(dt)
        if bindings.has_simulation_time:
            simulation_time = bindings.simulation_time()
        else:
            fallback_simulation_time += dt
            simulation_time = fallback_simulation_time
        dispatches: List[DispatchSample] = []
        category_totals: MutableMapping[str, float] = defaultdict(float)
        frame_total = 0.0

        for dispatch_index in range(bindings.dispatch_count()):
            name = bindings.dispatch_name(dispatch_index)
            duration_ms = bindings.dispatch_duration_ms(dispatch_index)
            category = _categorise_dispatch(name)
            category_totals[category] += duration_ms
            frame_total += duration_ms
            dispatches.append(
                DispatchSample(
                    name=name,
                    duration_ms=duration_ms,
                    category=category,
                )
            )

        samples.append(
            FrameSample(
                index=frame_index,
                simulation_time=simulation_time,
                timestep=dt,
                dispatches=dispatches,
                category_totals_ms=dict(sorted(category_totals.items())),
                frame_total_ms=frame_total,
            )
        )

    return samples


def _parse_variance_checks(
    values: Optional[Sequence[str]], trim_fraction: float
) -> Sequence[VarianceCheck]:
    checks: List[VarianceCheck] = []
    if not values:
        return checks
    if trim_fraction < 0.0 or trim_fraction >= 0.5:
        raise ValueError("Variance trim fraction must satisfy 0.0 <= value < 0.5.")
    for raw in values:
        if ":" not in raw:
            raise ValueError(
                "Variance check must use the form '<dispatch>:<max_percent>'."
            )
        dispatch, percent_str = raw.split(":", maxsplit=1)
        dispatch = dispatch.strip()
        percent_str = percent_str.strip()
        if not dispatch:
            raise ValueError("Dispatch name in variance check cannot be empty.")
        try:
            percent = float(percent_str)
        except ValueError as exc:  # pragma: no cover - defensive programming
            raise ValueError(
                f"Invalid percentage '{percent_str}' in variance check '{raw}'."
            ) from exc
        if percent < 0.0:
            raise ValueError("Variance percentage must be non-negative.")
        checks.append(
            VarianceCheck(
                dispatch_name=dispatch,
                max_percent=percent,
                trim_fraction=trim_fraction,
            )
        )
    return checks


def _durations_for_dispatch(
    samples: Sequence[FrameSample], dispatch_name: str
) -> tuple[List[float], bool]:
    durations: List[float] = []
    seen = False
    for frame in samples:
        total = 0.0
        matched = False
        for dispatch in frame.dispatches:
            if dispatch.name == dispatch_name:
                total += dispatch.duration_ms
                matched = True
        durations.append(total)
        seen = seen or matched
    return durations, seen


def evaluate_variance(
    samples: Sequence[FrameSample], check: VarianceCheck
) -> VarianceResult:
    durations, seen = _durations_for_dispatch(samples, check.dispatch_name)
    if not seen:
        raise ValueError(
            f"No dispatches matched '{check.dispatch_name}' for variance evaluation."
        )
    sorted_durations = sorted(durations)
    trim = int(len(sorted_durations) * check.trim_fraction)
    if trim * 2 >= len(sorted_durations):
        raise ValueError("Trim fraction removed all samples for variance evaluation.")
    if trim > 0:
        trimmed = sorted_durations[trim:-trim]
    else:
        trimmed = sorted_durations
    mean = statistics.fmean(trimmed)
    if len(trimmed) == 1:
        stdev = 0.0
    else:
        stdev = statistics.pstdev(trimmed)
    percent = 0.0 if mean == 0.0 else (stdev / mean) * 100.0
    return VarianceResult(
        check=check,
        durations_ms=trimmed,
        mean_ms=mean,
        stdev_ms=stdev,
        percent=percent,
        total_samples=len(durations),
    )


def summarise(samples: Sequence[FrameSample]) -> Dict[str, float]:
    category_totals: MutableMapping[str, float] = defaultdict(float)
    total_ms = 0.0
    for frame in samples:
        total_ms += frame.frame_total_ms
        for category, duration in frame.category_totals_ms.items():
            category_totals[category] += duration
    summary: Dict[str, float] = {f"category:{k}": v for k, v in sorted(category_totals.items())}
    summary["total_ms"] = total_ms
    summary["handoff_ms"] = category_totals.get("physics", 0.0) + category_totals.get("geometry", 0.0)
    return summary


def _diagnostics_to_dict(snapshot: RuntimeDiagnosticsSnapshot) -> Dict[str, object]:
    return {
        "initialize_count": snapshot.initialize_count,
        "shutdown_count": snapshot.shutdown_count,
        "tick_count": snapshot.tick_count,
        "last_initialize_ms": snapshot.last_initialize_ms,
        "last_shutdown_ms": snapshot.last_shutdown_ms,
        "last_tick_ms": snapshot.last_tick_ms,
        "average_tick_ms": snapshot.average_tick_ms,
        "max_tick_ms": snapshot.max_tick_ms,
        "stages": [
            {
                "name": stage.name,
                "last_ms": stage.last_ms,
                "average_ms": stage.average_ms,
                "max_ms": stage.max_ms,
                "sample_count": stage.sample_count,
            }
            for stage in snapshot.stages
        ],
        "subsystems": [
            {
                "name": subsystem.name,
                "last_initialize_ms": subsystem.last_initialize_ms,
                "last_tick_ms": subsystem.last_tick_ms,
                "last_shutdown_ms": subsystem.last_shutdown_ms,
                "max_initialize_ms": subsystem.max_initialize_ms,
                "max_tick_ms": subsystem.max_tick_ms,
                "max_shutdown_ms": subsystem.max_shutdown_ms,
                "initialize_count": subsystem.initialize_count,
                "tick_count": subsystem.tick_count,
                "shutdown_count": subsystem.shutdown_count,
            }
            for subsystem in snapshot.subsystems
        ],
    }


def _samples_to_dict(
    samples: Sequence[FrameSample], diagnostics: Optional[RuntimeDiagnosticsSnapshot]
) -> Dict[str, object]:
    return {
        "frames": [
            {
                "index": sample.index,
                "simulation_time": sample.simulation_time,
                "dt": sample.timestep,
                "frame_total_ms": sample.frame_total_ms,
                "category_totals_ms": sample.category_totals_ms,
                "dispatches": [
                    {
                        "name": dispatch.name,
                        "duration_ms": dispatch.duration_ms,
                        "category": dispatch.category,
                    }
                    for dispatch in sample.dispatches
                ],
            }
            for sample in samples
        ],
        "summary": summarise(samples),
        "runtime_diagnostics": _diagnostics_to_dict(diagnostics)
        if diagnostics is not None
        else None,
    }


def _print_summary(
    samples: Sequence[FrameSample],
    verbose: bool,
    diagnostics: Optional[RuntimeDiagnosticsSnapshot],
) -> None:
    data = summarise(samples)
    print("Aggregate category totals (ms):")
    for key, value in data.items():
        if key.startswith("category:"):
            category = key.split(":", maxsplit=1)[1]
            print(f"  {category:>10}: {value:8.4f} ms")
    print(f"Total recorded frame time: {data['total_ms']:.4f} ms")
    print(f"Physics→Geometry hand-off: {data['handoff_ms']:.4f} ms")
    if diagnostics is not None:
        print("\nRuntime diagnostics:")
        print(
            "  ticks: "
            f"count={diagnostics.tick_count} "
            f"last={diagnostics.last_tick_ms:.4f} ms "
            f"avg={diagnostics.average_tick_ms:.4f} ms "
            f"max={diagnostics.max_tick_ms:.4f} ms"
        )
        if diagnostics.subsystems:
            print("  subsystem ticks:")
            for subsystem in diagnostics.subsystems:
                if subsystem.tick_count == 0:
                    continue
                print(
                    "    "
                    f"{subsystem.name:<24} "
                    f"count={subsystem.tick_count:>3} "
                    f"last={subsystem.last_tick_ms:8.4f} ms "
                    f"max={subsystem.max_tick_ms:8.4f} ms"
                )
        if diagnostics.stages:
            print("  dispatcher stages:")
            for stage in diagnostics.stages:
                print(
                    "    "
                    f"{stage.name:<24} "
                    f"samples={stage.sample_count:>3} "
                    f"avg={stage.average_ms:8.4f} ms "
                    f"last={stage.last_ms:8.4f} ms"
                )
    if not verbose:
        return
    print("\nPer-frame dispatch timings:")
    for frame in samples:
        print(f"Frame {frame.index} (sim {frame.simulation_time:.6f}s)")
        for dispatch in frame.dispatches:
            print(
                f"  {dispatch.name:<24} {dispatch.category:<10} {dispatch.duration_ms:8.4f} ms"
            )
        print("  -- frame total --".ljust(40) + f"{frame.frame_total_ms:8.4f} ms")


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Capture runtime dispatcher telemetry for the animation/physics/geometry "
            "handoff that precedes rendering submissions."
        )
    )
    parser.add_argument(
        "--library-dir",
        type=Path,
        default=None,
        help=(
            "Directory containing libengine_runtime (falls back to current working "
            "directory and system library paths)."
        ),
    )
    parser.add_argument(
        "--library-name",
        default="engine_runtime",
        help="Base name of the runtime shared library to load.",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=1,
        help="Number of frames to record telemetry for (default: 1).",
    )
    parser.add_argument(
        "--dt",
        type=float,
        default=0.016,
        help="Simulation timestep in seconds to pass to RuntimeHost::tick (default: 0.016).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional JSON file to persist telemetry results.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print per-frame dispatch timing details in addition to the aggregate summary.",
    )
    parser.add_argument(
        "--window-backend",
        default="mock",
        help=(
            "Window backend hint passed via the ENGINE_PLATFORM_WINDOW_BACKEND environment "
            "variable (default: 'mock'). Use 'auto' to defer to runtime defaults."
        ),
    )
    parser.add_argument(
        "--variance-check",
        action="append",
        default=None,
        metavar="DISPATCH:PERCENT",
        help=(
            "Verify that the coefficient of variation for the named dispatch stays at or below "
            "the provided percentage. May be specified multiple times."
        ),
    )
    parser.add_argument(
        "--variance-trim",
        type=float,
        default=0.0,
        help=(
            "Symmetric fraction to trim from both ends of the sample set before computing variance "
            "(e.g., 0.1 trims 10% from the minimum and maximum tails)."
        ),
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    backend = args.window_backend.strip()
    if backend and backend.lower() != "auto":
        os.environ["ENGINE_PLATFORM_WINDOW_BACKEND"] = backend
    variance_checks = _parse_variance_checks(args.variance_check, args.variance_trim)
    bindings = RuntimeBindings.load(args.library_name, args.library_dir)
    bindings.configure_default_modules()
    bindings.initialize()
    diagnostics: Optional[RuntimeDiagnosticsSnapshot] = None
    try:
        samples = capture_frames(bindings, args.frames, args.dt)
        diagnostics = bindings.diagnostics_snapshot()
    finally:
        bindings.shutdown()

    _print_summary(samples, args.verbose, diagnostics)

    if variance_checks:
        for result in map(lambda check: evaluate_variance(samples, check), variance_checks):
            status = "PASS" if result.passed else "FAIL"
            print(
                "Variance check for dispatch '",
                result.check.dispatch_name,
                "': ",
                status,
                sep="",
            )
            if result.check.trim_fraction > 0.0:
                trimmed = len(result.durations_ms)
                print(
                    f"  trimmed {result.check.trim_fraction * 100:.1f}% -> "
                    f"{trimmed}/{result.total_samples} samples"
                )
            print(
                f"  mean={result.mean_ms:.6f} ms stdev={result.stdev_ms:.6f} ms "
                f"cov={result.percent:.3f}% (limit {result.check.max_percent:.3f}%)"
            )
            if not result.passed:
                return 1

    if args.output is not None:
        payload = _samples_to_dict(samples, diagnostics)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
