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
persist the structured telemetry as JSON for later comparison.
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
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


class RuntimeBindings:
    """Thin ctypes wrapper around the runtime C API."""

    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
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
        lib.engine_runtime_simulation_time.restype = ctypes.c_double

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
        return float(self._lib.engine_runtime_simulation_time())


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
    for frame_index in range(frames):
        bindings.tick(dt)
        simulation_time = bindings.simulation_time()
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


def _samples_to_dict(samples: Sequence[FrameSample]) -> Dict[str, object]:
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
    }


def _print_summary(samples: Sequence[FrameSample], verbose: bool) -> None:
    data = summarise(samples)
    print("Aggregate category totals (ms):")
    for key, value in data.items():
        if key.startswith("category:"):
            category = key.split(":", maxsplit=1)[1]
            print(f"  {category:>10}: {value:8.4f}")
    print(f"Total recorded frame time: {data['total_ms']:.4f} ms")
    print(f"Physics→Geometry hand-off: {data['handoff_ms']:.4f} ms")
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
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    bindings = RuntimeBindings.load(args.library_name, args.library_dir)
    bindings.configure_default_modules()
    bindings.initialize()
    try:
        samples = capture_frames(bindings, args.frames, args.dt)
    finally:
        bindings.shutdown()

    _print_summary(samples, args.verbose)

    if args.output is not None:
        payload = _samples_to_dict(samples)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
