#!/usr/bin/env python3
"""Report runtime streaming metrics via the C API."""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
from pathlib import Path
from typing import Optional, Sequence


class StreamingMetrics(ctypes.Structure):
    _fields_ = [
        ("worker_count", ctypes.c_size_t),
        ("queue_capacity", ctypes.c_size_t),
        ("pending_tasks", ctypes.c_size_t),
        ("active_workers", ctypes.c_size_t),
        ("total_enqueued", ctypes.c_uint64),
        ("total_executed", ctypes.c_uint64),
        ("streaming_pending", ctypes.c_uint64),
        ("streaming_loading", ctypes.c_uint64),
        ("streaming_total_requests", ctypes.c_uint64),
        ("streaming_total_completed", ctypes.c_uint64),
        ("streaming_total_failed", ctypes.c_uint64),
        ("streaming_total_cancelled", ctypes.c_uint64),
        ("streaming_total_rejected", ctypes.c_uint64),
    ]


class RuntimeStreamingBindings:
    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
        self._configure_signatures()

    @staticmethod
    def load(name: str, directory: Optional[Path]) -> "RuntimeStreamingBindings":
        candidates = tuple(_candidate_names(name))
        errors = []
        search_paths: Sequence[Path]
        if directory is not None:
            search_paths = [directory]
        else:
            search_paths = [Path.cwd()]

        for base in search_paths:
            for candidate in candidates:
                try:
                    library = ctypes.CDLL(str(base / candidate))
                    return RuntimeStreamingBindings(library)
                except OSError as exc:
                    errors.append(exc)

        resolved = ctypes.util.find_library(name)
        if resolved is not None:
            try:
                return RuntimeStreamingBindings(ctypes.CDLL(resolved))
            except OSError as exc:
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
        lib.engine_runtime_streaming_metrics.restype = None
        lib.engine_runtime_streaming_metrics.argtypes = [ctypes.POINTER(StreamingMetrics)]

    def metrics(self) -> StreamingMetrics:
        data = StreamingMetrics()
        self._lib.engine_runtime_streaming_metrics(ctypes.byref(data))
        return data


def _candidate_names(base: str):
    if ctypes.sizeof(ctypes.c_void_p) == 8 and Path("lib" + base + ".so").exists():
        yield f"lib{base}.so"
    if Path(base + ".so").exists():
        yield f"{base}.so"
    if Path(base + ".dll").exists():
        yield f"{base}.dll"
    if Path("lib" + base + ".dylib").exists():
        yield f"lib{base}.dylib"
    if Path(base + ".dylib").exists():
        yield f"{base}.dylib"
    yield f"lib{base}.so"
    yield f"{base}.so"
    yield f"{base}.dll"
    yield f"lib{base}.dylib"
    yield f"{base}.dylib"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library-dir", type=Path, default=None,
                        help="Directory containing the runtime shared library.")
    parser.add_argument("--output", type=Path, default=None,
                        help="Optional path to write the metrics JSON.")
    args = parser.parse_args()

    bindings = RuntimeStreamingBindings.load("engine_runtime", args.library_dir)
    metrics = bindings.metrics()
    payload = {
        "worker_count": metrics.worker_count,
        "queue_capacity": metrics.queue_capacity,
        "pending_tasks": metrics.pending_tasks,
        "active_workers": metrics.active_workers,
        "total_enqueued": metrics.total_enqueued,
        "total_executed": metrics.total_executed,
        "streaming_pending": metrics.streaming_pending,
        "streaming_loading": metrics.streaming_loading,
        "streaming_total_requests": metrics.streaming_total_requests,
        "streaming_total_completed": metrics.streaming_total_completed,
        "streaming_total_failed": metrics.streaming_total_failed,
        "streaming_total_cancelled": metrics.streaming_total_cancelled,
        "streaming_total_rejected": metrics.streaming_total_rejected,
    }

    text = json.dumps(payload, indent=2, sort_keys=True)
    print(text)
    if args.output is not None:
        args.output.write_text(text + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

