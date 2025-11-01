#!/usr/bin/env python3
"""Report runtime streaming metrics via the C API."""

from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
from pathlib import Path
from typing import Mapping, Optional, Sequence, Type


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
                (
                    "geometry_failure_labels",
                    ctypes.POINTER(ctypes.c_char) * capacity,
                ),
            ]
        },
    )


StreamingMetrics = _create_streaming_metrics_type(
    _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
)


class HotReloadMetrics(ctypes.Structure):
    _fields_ = [
        ("attempt_count", ctypes.c_uint64),
        ("failure_count", ctypes.c_uint64),
        ("cancelled_count", ctypes.c_uint64),
        ("rejected_count", ctypes.c_uint64),
        ("pending_count", ctypes.c_uint64),
        ("loading_count", ctypes.c_uint64),
        ("total_requests", ctypes.c_uint64),
        ("last_error", ctypes.c_char_p),
        ("error_hint", ctypes.c_char_p),
    ]


class RuntimeStreamingBindings:
    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
        self._streaming_metrics_capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
        self._streaming_metrics_type: Type[ctypes.Structure]
        self._streaming_metrics_type = _create_streaming_metrics_type(
            self._streaming_metrics_capacity
        )
        self._has_hot_reload_metrics = False
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
            global StreamingMetrics
            StreamingMetrics = self._streaming_metrics_type

        metrics_pointer = ctypes.POINTER(self._streaming_metrics_type)
        lib.engine_runtime_streaming_metrics.restype = None
        lib.engine_runtime_streaming_metrics.argtypes = [metrics_pointer]

        try:
            lib.engine_runtime_diagnostic_hot_reload_metrics.restype = None
            lib.engine_runtime_diagnostic_hot_reload_metrics.argtypes = [
                ctypes.POINTER(HotReloadMetrics)
            ]
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_count.restype = (
                ctypes.c_uint32
            )
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_count.argtypes = []
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_identifier.restype = (
                ctypes.c_char_p
            )
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_identifier.argtypes = [
                ctypes.c_uint32
            ]
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_error.restype = (
                ctypes.c_char_p
            )
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_error.argtypes = [
                ctypes.c_uint32
            ]
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_hint.restype = (
                ctypes.c_char_p
            )
            lib.engine_runtime_diagnostic_hot_reload_recent_failure_hint.argtypes = [
                ctypes.c_uint32
            ]
        except AttributeError:
            self._has_hot_reload_metrics = False
        else:
            self._has_hot_reload_metrics = True

    def metrics(self) -> StreamingMetrics:
        data = self._streaming_metrics_type()
        self._lib.engine_runtime_streaming_metrics(ctypes.byref(data))
        return data

    @property
    def streaming_metrics_capacity(self) -> int:
        return self._streaming_metrics_capacity

    def hot_reload_metrics(self) -> Optional[tuple[HotReloadMetrics, list[dict[str, str]]]]:
        if not self._has_hot_reload_metrics:
            return None

        metrics = HotReloadMetrics()
        self._lib.engine_runtime_diagnostic_hot_reload_metrics(ctypes.byref(metrics))

        count = int(
            self._lib.engine_runtime_diagnostic_hot_reload_recent_failure_count()
        )
        failures: list[dict[str, str]] = []
        for index in range(count):
            identifier = self._lib.engine_runtime_diagnostic_hot_reload_recent_failure_identifier(  # type: ignore[attr-defined]
                ctypes.c_uint32(index)
            )
            error = self._lib.engine_runtime_diagnostic_hot_reload_recent_failure_error(  # type: ignore[attr-defined]
                ctypes.c_uint32(index)
            )
            hint = self._lib.engine_runtime_diagnostic_hot_reload_recent_failure_hint(  # type: ignore[attr-defined]
                ctypes.c_uint32(index)
            )
            failures.append(
                {
                    "identifier": (identifier or b"").decode("utf-8"),
                    "error": (error or b"").decode("utf-8"),
                    "hint": (hint or b"").decode("utf-8"),
                }
            )

        return metrics, failures


def _render_text_dashboard(payload: Mapping[str, object]) -> str:
    lines: list[str] = []

    def section(title: str) -> None:
        if lines:
            lines.append("")
        lines.append(title)
        lines.append("-" * len(title))

    section("Streaming Queue")
    lines.append(f"Workers: {int(payload.get('worker_count', 0))}")
    lines.append(f"Queue capacity: {int(payload.get('queue_capacity', 0))}")
    lines.append(f"Pending tasks: {int(payload.get('pending_tasks', 0))}")
    lines.append(f"Active workers: {int(payload.get('active_workers', 0))}")

    section("Streaming Totals")
    lines.append(f"Requests observed: {int(payload.get('streaming_total_requests', 0))}")
    lines.append(f"Completed: {int(payload.get('streaming_total_completed', 0))}")
    lines.append(f"Failed: {int(payload.get('streaming_total_failed', 0))}")
    lines.append(f"Cancelled: {int(payload.get('streaming_total_cancelled', 0))}")
    lines.append(f"Rejected: {int(payload.get('streaming_total_rejected', 0))}")
    lines.append(f"In-flight — pending: {int(payload.get('streaming_pending', 0))}")
    lines.append(f"In-flight — loading: {int(payload.get('streaming_loading', 0))}")

    failures = payload.get("geometry_failures_by_error")
    if isinstance(failures, Mapping) and failures:
        section("Geometry Failure Attribution")
        for label, count in sorted((str(key), int(value)) for key, value in failures.items()):
            lines.append(f"{label}: {count}")

    hot_reload = payload.get("hot_reload")
    if isinstance(hot_reload, Mapping):
        section("Hot Reload Summary")
        lines.append(f"Attempts: {int(hot_reload.get('attempt_count', 0))}")
        lines.append(f"Failures: {int(hot_reload.get('failure_count', 0))}")
        lines.append(f"Cancelled: {int(hot_reload.get('cancelled_count', 0))}")
        lines.append(f"Rejected: {int(hot_reload.get('rejected_count', 0))}")
        lines.append(f"Pending: {int(hot_reload.get('pending_count', 0))}")
        lines.append(f"Loading: {int(hot_reload.get('loading_count', 0))}")
        last_error = str(hot_reload.get("last_error", "")).strip()
        error_hint = str(hot_reload.get("error_hint", "")).strip()
        if last_error:
            lines.append(f"Last error: {last_error}")
        if error_hint:
            lines.append(f"Hint: {error_hint}")
        recent = hot_reload.get("recent_failures")
        if isinstance(recent, Sequence) and recent:
            lines.append("Recent failures:")
            for entry in recent:
                if not isinstance(entry, Mapping):
                    continue
                identifier = str(entry.get("identifier", "")).strip() or "<unknown>"
                error = str(entry.get("error", "")).strip()
                hint = str(entry.get("hint", "")).strip()
                detail = f"  • {identifier}"
                if error:
                    detail += f": {error}"
                lines.append(detail)
                if hint:
                    lines.append(f"    Hint: {hint}")

    return "\n".join(lines)


def _build_chrome_trace(payload: Mapping[str, object], label: str) -> dict[str, object]:
    events: list[dict[str, object]] = []
    events.append({"name": "process_name", "ph": "M", "pid": 0, "args": {"name": label}})
    events.append({"name": "thread_name", "ph": "M", "pid": 0, "tid": 0, "args": {"name": "Streaming"}})

    streaming_args = {
        "worker_count": int(payload.get("worker_count", 0)),
        "queue_capacity": int(payload.get("queue_capacity", 0)),
        "pending_tasks": int(payload.get("pending_tasks", 0)),
        "active_workers": int(payload.get("active_workers", 0)),
        "streaming_pending": int(payload.get("streaming_pending", 0)),
        "streaming_loading": int(payload.get("streaming_loading", 0)),
        "streaming_total_requests": int(payload.get("streaming_total_requests", 0)),
        "streaming_total_completed": int(payload.get("streaming_total_completed", 0)),
        "streaming_total_failed": int(payload.get("streaming_total_failed", 0)),
        "streaming_total_cancelled": int(payload.get("streaming_total_cancelled", 0)),
        "streaming_total_rejected": int(payload.get("streaming_total_rejected", 0)),
        "total_enqueued": int(payload.get("total_enqueued", 0)),
        "total_executed": int(payload.get("total_executed", 0)),
    }
    events.append(
        {
            "name": "Streaming Counters",
            "cat": "assets.streaming",
            "ph": "C",
            "pid": 0,
            "tid": 0,
            "ts": 0,
            "args": streaming_args,
        }
    )

    failures = payload.get("geometry_failures_by_error")
    if isinstance(failures, Mapping) and failures:
        failure_args = {str(key): int(value) for key, value in failures.items()}
        events.append(
            {
                "name": "Geometry Failures",
                "cat": "assets.streaming",
                "ph": "C",
                "pid": 0,
                "tid": 0,
                "ts": 0,
                "args": failure_args,
            }
        )

    hot_reload = payload.get("hot_reload")
    if isinstance(hot_reload, Mapping):
        hot_reload_args = {
            "attempt_count": int(hot_reload.get("attempt_count", 0)),
            "failure_count": int(hot_reload.get("failure_count", 0)),
            "cancelled_count": int(hot_reload.get("cancelled_count", 0)),
            "rejected_count": int(hot_reload.get("rejected_count", 0)),
            "pending_count": int(hot_reload.get("pending_count", 0)),
            "loading_count": int(hot_reload.get("loading_count", 0)),
            "total_requests": int(hot_reload.get("total_requests", 0)),
        }
        last_error = str(hot_reload.get("last_error", "")).strip()
        error_hint = str(hot_reload.get("error_hint", "")).strip()
        if last_error:
            hot_reload_args["last_error"] = last_error
        if error_hint:
            hot_reload_args["error_hint"] = error_hint
        recent = hot_reload.get("recent_failures")
        if isinstance(recent, Sequence) and recent:
            identifiers = [
                str(entry.get("identifier", "")).strip() or "<unknown>"
                for entry in recent
                if isinstance(entry, Mapping)
            ]
            if identifiers:
                hot_reload_args["recent_failures"] = ", ".join(identifiers)
        events.append(
            {
                "name": "Hot Reload Counters",
                "cat": "assets.hot_reload",
                "ph": "C",
                "pid": 0,
                "tid": 0,
                "ts": 0,
                "args": hot_reload_args,
            }
        )

    return {"traceEvents": events, "displayTimeUnit": "ms"}


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
    parser.add_argument(
        "--library-dir",
        type=Path,
        default=None,
        help="Directory containing the runtime shared library.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional path to write the rendered report.",
    )
    parser.add_argument(
        "--format",
        choices=("json", "text"),
        default="json",
        help="Output format: JSON (default) or a human-readable text dashboard.",
    )
    parser.add_argument(
        "--chrome-trace",
        type=Path,
        default=None,
        help="Optional path to persist a Chrome trace counter snapshot for asset telemetry.",
    )
    parser.add_argument(
        "--chrome-trace-label",
        default="Asset Streaming Telemetry",
        help="Label embedded in the generated Chrome trace metadata (default: 'Asset Streaming Telemetry').",
    )
    args = parser.parse_args()

    bindings = RuntimeStreamingBindings.load("engine_runtime", args.library_dir)
    metrics = bindings.metrics()
    limit = min(int(metrics.geometry_failure_count), bindings.streaming_metrics_capacity)
    failure_counts = {}
    for index in range(limit):
        label_pointer = metrics.geometry_failure_labels[index]
        if bool(label_pointer):
            label = ctypes.string_at(label_pointer).decode("utf-8", errors="replace")
        else:
            label = f"error_{index}"
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

    hot_reload = bindings.hot_reload_metrics()
    if hot_reload is not None:
        metrics_struct, recent_failures = hot_reload
        payload["hot_reload"] = {
            "attempt_count": metrics_struct.attempt_count,
            "failure_count": metrics_struct.failure_count,
            "cancelled_count": metrics_struct.cancelled_count,
            "rejected_count": metrics_struct.rejected_count,
            "pending_count": metrics_struct.pending_count,
            "loading_count": metrics_struct.loading_count,
            "total_requests": metrics_struct.total_requests,
            "last_error": (metrics_struct.last_error or b"").decode("utf-8"),
            "error_hint": (metrics_struct.error_hint or b"").decode("utf-8"),
            "recent_failures": recent_failures,
        }
    else:
        payload["hot_reload"] = None

    if args.chrome_trace is not None:
        trace_payload = _build_chrome_trace(payload, args.chrome_trace_label)
        args.chrome_trace.write_text(json.dumps(trace_payload, indent=2) + "\n", encoding="utf-8")

    if args.format == "text":
        rendered = _render_text_dashboard(payload)
    else:
        rendered = json.dumps(payload, indent=2, sort_keys=True)

    print(rendered)
    if args.output is not None:
        args.output.write_text(rendered + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

