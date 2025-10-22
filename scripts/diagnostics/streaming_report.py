#!/usr/bin/env python3
"""Report runtime streaming metrics via the C API."""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
from pathlib import Path
from typing import Optional, Sequence, Type


_STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK = 5


def _create_streaming_metrics_type(capacity: int) -> Type[ctypes.Structure]:
    if capacity <= 0:
        capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK

    return type(
        "StreamingMetrics",
        (ctypes.Structure,),
        {
            "_fields_": [
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
                ("geometry_failure_count", ctypes.c_uint32),
                ("geometry_failures", ctypes.c_uint64 * capacity),
                ("geometry_failure_labels", ctypes.c_char_p * capacity),
            ]
        },
    )


StreamingMetrics = _create_streaming_metrics_type(
    _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
)


class RuntimeStreamingBindings:
    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
        self._streaming_metrics_capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
        self._streaming_metrics_type: Type[ctypes.Structure]
        self._streaming_metrics_type = _create_streaming_metrics_type(
            self._streaming_metrics_capacity
        )
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
        capacity = self._streaming_metrics_capacity
        try:
            lib.engine_runtime_streaming_geometry_failure_capacity_value.restype = ctypes.c_size_t
            lib.engine_runtime_streaming_geometry_failure_capacity_value.argtypes = []
        except AttributeError:
            pass
        else:
            try:
                capacity = int(lib.engine_runtime_streaming_geometry_failure_capacity_value())
            except Exception:  # pragma: no cover - depends on runtime ABI availability
                capacity = self._streaming_metrics_capacity
        if capacity <= 0:
            capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
        if capacity != self._streaming_metrics_capacity:
            self._streaming_metrics_capacity = capacity
            self._streaming_metrics_type = _create_streaming_metrics_type(capacity)

        metrics_pointer = ctypes.POINTER(self._streaming_metrics_type)
        lib.engine_runtime_streaming_metrics.restype = None
        lib.engine_runtime_streaming_metrics.argtypes = [metrics_pointer]

    def metrics(self) -> StreamingMetrics:
        data = self._streaming_metrics_type()
        self._lib.engine_runtime_streaming_metrics(ctypes.byref(data))
        return data

    @property
    def streaming_metrics_capacity(self) -> int:
        return self._streaming_metrics_capacity


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
    limit = min(int(metrics.geometry_failure_count), bindings.streaming_metrics_capacity)
    failure_counts = {}
    for index in range(limit):
        label_bytes = metrics.geometry_failure_labels[index]
        label = label_bytes.decode("utf-8") if label_bytes else f"error_{index}"
        failure_counts[label] = metrics.geometry_failures[index]
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
        "geometry_failures_by_error": failure_counts,
    }

    text = json.dumps(payload, indent=2, sort_keys=True)
    print(text)
    if args.output is not None:
        args.output.write_text(text + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

