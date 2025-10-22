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
from typing import Dict, Iterable, List, MutableMapping, Optional, Sequence, Tuple, Type


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
    initialize_failure_count: int
    last_initialize_failure_ms: float
    last_initialize_failure_category: str
    last_initialize_failure_message: str


@dataclass
class RuntimeInitializationFailure:
    """Details about the most recent runtime initialization failure."""

    runtime: str
    subsystem: str
    category: str
    message: str
    duration_ms: float


@dataclass
class RuntimeStreamingMetrics:
    """Snapshot of runtime streaming telemetry exposed via the C ABI."""

    worker_count: int
    queue_capacity: int
    pending_tasks: int
    active_workers: int
    total_enqueued: int
    total_executed: int
    streaming_pending: int
    streaming_loading: int
    streaming_total_requests: int
    streaming_total_completed: int
    streaming_total_failed: int
    streaming_total_cancelled: int
    streaming_total_rejected: int
    geometry_failures_by_error: Dict[str, int]


@dataclass
class HotReloadMetrics:
    """Hot reload telemetry snapshot exported through the runtime diagnostics."""

    attempt_count: int
    failure_count: int
    cancelled_count: int
    rejected_count: int
    pending_count: int
    loading_count: int
    total_requests: int
    last_error: str
    error_hint: str


@dataclass
class SceneHierarchyIssue:
    """Single hierarchy validation issue emitted by the runtime."""

    entity: int
    related: int
    type: str
    message: str


@dataclass
class SceneValidationSnapshot:
    """Hierarchy validation metrics and issues exposed through the diagnostics bridge."""

    issue_count: int
    cycle_count: int
    dangling_parent_count: int
    missing_parent_hierarchy_count: int
    non_finite_transform_count: int
    transform_mismatch_count: int
    issues: List[SceneHierarchyIssue]


_METRIC_KIND = {
    0: "counter",
    1: "gauge",
    2: "histogram",
}

_METRIC_UNIT = {
    0: "none",
    1: "count",
    2: "milliseconds",
    3: "seconds",
    4: "bytes",
    5: "percentage",
}

_METRIC_UNIT_SUFFIX = {
    "none": "",
    "count": "",
    "milliseconds": " ms",
    "seconds": " s",
    "bytes": " bytes",
    "percentage": " %",
}


@dataclass
class RuntimeMetricDescriptor:
    """Schema metadata describing a single runtime metric."""

    name: str
    kind: str
    unit: str
    description: str
    labels: Dict[str, str]


@dataclass
class RuntimeMetricSample:
    """Metric value paired with its descriptor index."""

    descriptor_index: int
    is_integral: bool
    value: float
    int_value: int


@dataclass
class RuntimeMetricsSnapshot:
    """Complete metrics payload exposed through the runtime diagnostics API."""

    descriptors: List[RuntimeMetricDescriptor]
    samples: List[RuntimeMetricSample]


@dataclass
class RuntimeDiagnosticsSnapshot:
    """Aggregated runtime lifecycle diagnostics exposed through the C ABI."""

    initialize_count: int
    initialize_failure_count: int
    shutdown_count: int
    tick_count: int
    last_initialize_ms: float
    last_shutdown_ms: float
    last_tick_ms: float
    has_initialize_failure: bool
    last_initialize_failure: Optional[RuntimeInitializationFailure]
    average_tick_ms: float
    max_tick_ms: float
    stages: List[RuntimeStageMetric]
    subsystems: List[RuntimeSubsystemMetric]
    streaming: Optional[RuntimeStreamingMetrics] = None
    hot_reload: Optional[HotReloadMetrics] = None
    scene_validation: Optional[SceneValidationSnapshot] = None
    metrics: Optional[RuntimeMetricsSnapshot] = None


_STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK = 5


def _create_streaming_metrics_type(capacity: int) -> Type[ctypes.Structure]:
    """Build a ctypes struct matching the runtime streaming metrics layout."""

    if capacity <= 0:
        capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK

    return type(
        "_CStreamingMetrics",
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


class _CHotReloadMetrics(ctypes.Structure):
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


class RuntimeBindings:
    """Thin ctypes wrapper around the runtime C API."""

    def __init__(self, library: ctypes.CDLL) -> None:
        self._lib = library
        self._has_simulation_time = False
        self._has_diagnostics = False
        self._has_streaming_metrics = False
        self._has_hot_reload_metrics = False
        self._has_scene_validation = False
        self._has_metrics = False
        self._streaming_metrics_capacity = _STREAMING_GEOMETRY_ERROR_CAPACITY_FALLBACK
        self._streaming_metrics_type: Type[ctypes.Structure]
        self._streaming_metrics_type = _create_streaming_metrics_type(
            self._streaming_metrics_capacity
        )
        self._streaming_metrics_func = None
        self._hot_reload_metrics_func = None
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

        streaming_metrics_pointer = ctypes.POINTER(self._streaming_metrics_type)
        try:
            lib.engine_runtime_diagnostic_initialize_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_initialize_count.argtypes = []
            lib.engine_runtime_diagnostic_initialize_failure_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_initialize_failure_count.argtypes = []
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
            lib.engine_runtime_diagnostic_has_initialize_failure.restype = ctypes.c_bool
            lib.engine_runtime_diagnostic_has_initialize_failure.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_failure_runtime.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_last_initialize_failure_runtime.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_failure_subsystem.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_last_initialize_failure_subsystem.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_failure_category.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_last_initialize_failure_category.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_failure_message.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_last_initialize_failure_message.argtypes = []
            lib.engine_runtime_diagnostic_last_initialize_failure_duration_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_last_initialize_failure_duration_ms.argtypes = []
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
            lib.engine_runtime_diagnostic_subsystem_initialize_failure_count.restype = ctypes.c_uint64
            lib.engine_runtime_diagnostic_subsystem_initialize_failure_count.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_initialize_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_initialize_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_tick_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_tick_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_max_shutdown_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_max_shutdown_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_ms.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_ms.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_category.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_category.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_message.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_message.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_count.restype = ctypes.c_size_t
            lib.engine_runtime_diagnostic_metric_count.argtypes = []
            lib.engine_runtime_diagnostic_metric_name.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_metric_name.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_kind.restype = ctypes.c_int
            lib.engine_runtime_diagnostic_metric_kind.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_unit.restype = ctypes.c_int
            lib.engine_runtime_diagnostic_metric_unit.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_description.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_metric_description.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_label_count.restype = ctypes.c_size_t
            lib.engine_runtime_diagnostic_metric_label_count.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_label_key.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_metric_label_key.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_label_value.restype = ctypes.c_char_p
            lib.engine_runtime_diagnostic_metric_label_value.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_is_integral.restype = ctypes.c_bool
            lib.engine_runtime_diagnostic_metric_is_integral.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_value.restype = ctypes.c_double
            lib.engine_runtime_diagnostic_metric_value.argtypes = [ctypes.c_size_t]
            lib.engine_runtime_diagnostic_metric_value_int.restype = ctypes.c_int64
            lib.engine_runtime_diagnostic_metric_value_int.argtypes = [ctypes.c_size_t]
        except AttributeError:
            self._has_diagnostics = False
            self._has_scene_validation = False
            self._has_metrics = False
        else:
            self._has_diagnostics = True
            self._has_metrics = True

        try:
            lib.engine_runtime_diagnostic_streaming_metrics.restype = None
            lib.engine_runtime_diagnostic_streaming_metrics.argtypes = [
                streaming_metrics_pointer
            ]
        except AttributeError:
            try:
                lib.engine_runtime_streaming_metrics.restype = None
                lib.engine_runtime_streaming_metrics.argtypes = [streaming_metrics_pointer]
            except AttributeError:
                self._has_streaming_metrics = False
                self._streaming_metrics_func = None
            else:
                self._streaming_metrics_func = lib.engine_runtime_streaming_metrics
                self._has_streaming_metrics = True
        else:
            self._streaming_metrics_func = lib.engine_runtime_diagnostic_streaming_metrics
            self._has_streaming_metrics = True

        try:
            lib.engine_runtime_diagnostic_hot_reload_metrics.restype = None
            lib.engine_runtime_diagnostic_hot_reload_metrics.argtypes = [
                ctypes.POINTER(_CHotReloadMetrics)
            ]
        except AttributeError:
            self._has_hot_reload_metrics = False
            self._hot_reload_metrics_func = None
        else:
            self._has_hot_reload_metrics = True
            self._hot_reload_metrics_func = lib.engine_runtime_diagnostic_hot_reload_metrics

        if self._has_diagnostics:
            try:
                lib.engine_runtime_diagnostic_scene_issue_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_issue_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_cycle_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_cycle_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_dangling_parent_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_dangling_parent_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_missing_parent_hierarchy_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_missing_parent_hierarchy_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_non_finite_transform_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_non_finite_transform_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_transform_mismatch_count.restype = ctypes.c_uint64
                lib.engine_runtime_diagnostic_scene_transform_mismatch_count.argtypes = []
                lib.engine_runtime_diagnostic_scene_issue_total.restype = ctypes.c_size_t
                lib.engine_runtime_diagnostic_scene_issue_total.argtypes = []
                lib.engine_runtime_diagnostic_scene_issue_entity.restype = ctypes.c_uint32
                lib.engine_runtime_diagnostic_scene_issue_entity.argtypes = [ctypes.c_size_t]
                lib.engine_runtime_diagnostic_scene_issue_related.restype = ctypes.c_uint32
                lib.engine_runtime_diagnostic_scene_issue_related.argtypes = [ctypes.c_size_t]
                lib.engine_runtime_diagnostic_scene_issue_type_name.restype = ctypes.c_char_p
                lib.engine_runtime_diagnostic_scene_issue_type_name.argtypes = [ctypes.c_size_t]
                lib.engine_runtime_diagnostic_scene_issue_message.restype = ctypes.c_char_p
                lib.engine_runtime_diagnostic_scene_issue_message.argtypes = [ctypes.c_size_t]
            except AttributeError:
                self._has_scene_validation = False
            else:
                self._has_scene_validation = True

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

    @property
    def has_streaming_metrics(self) -> bool:
        return self._has_streaming_metrics

    @property
    def streaming_metrics_capacity(self) -> int:
        return self._streaming_metrics_capacity

    def streaming_metrics(self) -> Optional[RuntimeStreamingMetrics]:
        if not self._has_streaming_metrics or self._streaming_metrics_func is None:
            return None
        data = self._streaming_metrics_type()
        self._streaming_metrics_func(ctypes.byref(data))
        failures: Dict[str, int] = {}
        limit = min(int(data.geometry_failure_count), self._streaming_metrics_capacity)
        for index in range(limit):
            label_bytes = data.geometry_failure_labels[index]
            label = label_bytes.decode("utf-8") if label_bytes else f"error_{index}"
            failures[label] = int(data.geometry_failures[index])
        return RuntimeStreamingMetrics(
            worker_count=int(data.worker_count),
            queue_capacity=int(data.queue_capacity),
            pending_tasks=int(data.pending_tasks),
            active_workers=int(data.active_workers),
            total_enqueued=int(data.total_enqueued),
            total_executed=int(data.total_executed),
            streaming_pending=int(data.streaming_pending),
            streaming_loading=int(data.streaming_loading),
            streaming_total_requests=int(data.streaming_total_requests),
            streaming_total_completed=int(data.streaming_total_completed),
            streaming_total_failed=int(data.streaming_total_failed),
            streaming_total_cancelled=int(data.streaming_total_cancelled),
            streaming_total_rejected=int(data.streaming_total_rejected),
            geometry_failures_by_error=failures,
        )

    def _collect_hot_reload_metrics(self) -> Optional[HotReloadMetrics]:
        if not self._has_hot_reload_metrics or self._hot_reload_metrics_func is None:
            return None
        data = _CHotReloadMetrics()
        self._hot_reload_metrics_func(ctypes.byref(data))
        last_error = data.last_error.decode("utf-8") if data.last_error else ""
        error_hint = data.error_hint.decode("utf-8") if data.error_hint else ""
        return HotReloadMetrics(
            attempt_count=int(data.attempt_count),
            failure_count=int(data.failure_count),
            cancelled_count=int(data.cancelled_count),
            rejected_count=int(data.rejected_count),
            pending_count=int(data.pending_count),
            loading_count=int(data.loading_count),
            total_requests=int(data.total_requests),
            last_error=last_error,
            error_hint=error_hint,
        )

    def diagnostics_snapshot(self) -> Optional[RuntimeDiagnosticsSnapshot]:
        if not self._has_diagnostics:
            return None
        has_initialize_failure = bool(self._lib.engine_runtime_diagnostic_has_initialize_failure())
        return RuntimeDiagnosticsSnapshot(
            initialize_count=int(self._lib.engine_runtime_diagnostic_initialize_count()),
            initialize_failure_count=int(
                self._lib.engine_runtime_diagnostic_initialize_failure_count()
            ),
            shutdown_count=int(self._lib.engine_runtime_diagnostic_shutdown_count()),
            tick_count=int(self._lib.engine_runtime_diagnostic_tick_count()),
            last_initialize_ms=float(self._lib.engine_runtime_diagnostic_last_initialize_ms()),
            last_shutdown_ms=float(self._lib.engine_runtime_diagnostic_last_shutdown_ms()),
            last_tick_ms=float(self._lib.engine_runtime_diagnostic_last_tick_ms()),
            has_initialize_failure=has_initialize_failure,
            last_initialize_failure=self._collect_last_initialize_failure(has_initialize_failure),
            average_tick_ms=float(self._lib.engine_runtime_diagnostic_average_tick_ms()),
            max_tick_ms=float(self._lib.engine_runtime_diagnostic_max_tick_ms()),
            stages=self._collect_stage_metrics(),
            subsystems=self._collect_subsystem_metrics(),
            streaming=self.streaming_metrics(),
            hot_reload=self._collect_hot_reload_metrics(),
            scene_validation=self._collect_scene_validation(),
            metrics=self._collect_metrics(),
        )

    def _collect_last_initialize_failure(
        self, has_initialize_failure: bool
    ) -> Optional[RuntimeInitializationFailure]:
        if not has_initialize_failure:
            return None

        def _decode(value: Optional[bytes]) -> str:
            return value.decode("utf-8") if value else ""

        runtime = _decode(self._lib.engine_runtime_diagnostic_last_initialize_failure_runtime())
        subsystem = _decode(self._lib.engine_runtime_diagnostic_last_initialize_failure_subsystem())
        category = _decode(self._lib.engine_runtime_diagnostic_last_initialize_failure_category())
        message = _decode(self._lib.engine_runtime_diagnostic_last_initialize_failure_message())
        duration = float(self._lib.engine_runtime_diagnostic_last_initialize_failure_duration_ms())
        return RuntimeInitializationFailure(
            runtime=runtime,
            subsystem=subsystem,
            category=category,
            message=message,
            duration_ms=duration,
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
            failure_category = self._lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_category(index)
            failure_message = self._lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_message(index)
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
                    initialize_failure_count=int(
                        self._lib.engine_runtime_diagnostic_subsystem_initialize_failure_count(index)
                    ),
                    last_initialize_failure_ms=float(
                        self._lib.engine_runtime_diagnostic_subsystem_last_initialize_failure_ms(index)
                    ),
                    last_initialize_failure_category=failure_category.decode("utf-8")
                    if failure_category
                    else "",
                    last_initialize_failure_message=failure_message.decode("utf-8")
                    if failure_message
                    else "",
                )
            )
        return metrics

    def _collect_scene_validation(self) -> Optional[SceneValidationSnapshot]:
        if not self._has_diagnostics or not self._has_scene_validation:
            return None

        issue_count = int(self._lib.engine_runtime_diagnostic_scene_issue_count())
        cycle_count = int(self._lib.engine_runtime_diagnostic_scene_cycle_count())
        dangling_count = int(self._lib.engine_runtime_diagnostic_scene_dangling_parent_count())
        non_finite_count = int(self._lib.engine_runtime_diagnostic_scene_non_finite_transform_count())
        mismatch_count = int(self._lib.engine_runtime_diagnostic_scene_transform_mismatch_count())

        total = int(self._lib.engine_runtime_diagnostic_scene_issue_total())
        issues: List[SceneHierarchyIssue] = []
        for index in range(total):
            raw_type = self._lib.engine_runtime_diagnostic_scene_issue_type_name(index)
            type_name = raw_type.decode("utf-8") if raw_type else "unknown"
            raw_message = self._lib.engine_runtime_diagnostic_scene_issue_message(index)
            message = raw_message.decode("utf-8") if raw_message else ""
            issues.append(
                SceneHierarchyIssue(
                    entity=int(self._lib.engine_runtime_diagnostic_scene_issue_entity(index)),
                    related=int(self._lib.engine_runtime_diagnostic_scene_issue_related(index)),
                    type=type_name,
                    message=message,
                )
            )

        return SceneValidationSnapshot(
            issue_count=issue_count,
            cycle_count=cycle_count,
            dangling_parent_count=dangling_count,
            missing_parent_hierarchy_count=int(
                self._lib.engine_runtime_diagnostic_scene_missing_parent_hierarchy_count()
            ),
            non_finite_transform_count=non_finite_count,
            transform_mismatch_count=mismatch_count,
            issues=issues,
        )

    def _collect_metrics(self) -> Optional[RuntimeMetricsSnapshot]:
        if not self._has_diagnostics or not self._has_metrics:
            return None

        count = int(self._lib.engine_runtime_diagnostic_metric_count())
        descriptors: List[RuntimeMetricDescriptor] = []
        samples: List[RuntimeMetricSample] = []

        for index in range(count):
            raw_name = self._lib.engine_runtime_diagnostic_metric_name(index)
            name = raw_name.decode("utf-8") if raw_name else ""
            kind_value = int(self._lib.engine_runtime_diagnostic_metric_kind(index))
            unit_value = int(self._lib.engine_runtime_diagnostic_metric_unit(index))
            raw_description = self._lib.engine_runtime_diagnostic_metric_description(index)
            description = raw_description.decode("utf-8") if raw_description else ""
            label_count = int(self._lib.engine_runtime_diagnostic_metric_label_count(index))
            labels: Dict[str, str] = {}
            for label_index in range(label_count):
                raw_key = self._lib.engine_runtime_diagnostic_metric_label_key(index, label_index)
                raw_value = self._lib.engine_runtime_diagnostic_metric_label_value(index, label_index)
                key = raw_key.decode("utf-8") if raw_key else ""
                value = raw_value.decode("utf-8") if raw_value else ""
                if key:
                    labels[key] = value

            descriptors.append(
                RuntimeMetricDescriptor(
                    name=name,
                    kind=_METRIC_KIND.get(kind_value, f"unknown({kind_value})"),
                    unit=_METRIC_UNIT.get(unit_value, f"unknown({unit_value})"),
                    description=description,
                    labels=labels,
                )
            )

            is_integral = bool(self._lib.engine_runtime_diagnostic_metric_is_integral(index))
            value = float(self._lib.engine_runtime_diagnostic_metric_value(index))
            int_value = int(self._lib.engine_runtime_diagnostic_metric_value_int(index))
            samples.append(
                RuntimeMetricSample(
                    descriptor_index=index,
                    is_integral=is_integral,
                    value=value,
                    int_value=int_value,
                )
            )

        return RuntimeMetricsSnapshot(descriptors=descriptors, samples=samples)


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
        "initialize_failure_count": snapshot.initialize_failure_count,
        "shutdown_count": snapshot.shutdown_count,
        "tick_count": snapshot.tick_count,
        "last_initialize_ms": snapshot.last_initialize_ms,
        "last_shutdown_ms": snapshot.last_shutdown_ms,
        "last_tick_ms": snapshot.last_tick_ms,
        "has_initialize_failure": snapshot.has_initialize_failure,
        "last_initialize_failure": (
            {
                "runtime": snapshot.last_initialize_failure.runtime,
                "subsystem": snapshot.last_initialize_failure.subsystem,
                "category": snapshot.last_initialize_failure.category,
                "message": snapshot.last_initialize_failure.message,
                "duration_ms": snapshot.last_initialize_failure.duration_ms,
            }
            if snapshot.last_initialize_failure
            else None
        ),
        "average_tick_ms": snapshot.average_tick_ms,
        "max_tick_ms": snapshot.max_tick_ms,
        "streaming": _streaming_to_dict(snapshot.streaming),
        "hot_reload": _hot_reload_to_dict(snapshot.hot_reload),
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
                "initialize_failure_count": subsystem.initialize_failure_count,
                "last_initialize_failure_ms": subsystem.last_initialize_failure_ms,
                "last_initialize_failure_category": subsystem.last_initialize_failure_category,
                "last_initialize_failure_message": subsystem.last_initialize_failure_message,
            }
            for subsystem in snapshot.subsystems
        ],
        "scene_validation": _scene_validation_to_dict(snapshot.scene_validation),
        "metrics": _metrics_to_dict(snapshot.metrics),
    }


def _streaming_to_dict(
    metrics: Optional[RuntimeStreamingMetrics],
) -> Optional[Dict[str, object]]:
    if metrics is None:
        return None
    return {
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
        "geometry_failures_by_error": dict(metrics.geometry_failures_by_error),
    }


def _hot_reload_to_dict(metrics: Optional[HotReloadMetrics]) -> Optional[Dict[str, object]]:
    if metrics is None:
        return None
    return {
        "attempt_count": metrics.attempt_count,
        "failure_count": metrics.failure_count,
        "cancelled_count": metrics.cancelled_count,
        "rejected_count": metrics.rejected_count,
        "pending_count": metrics.pending_count,
        "loading_count": metrics.loading_count,
        "total_requests": metrics.total_requests,
        "last_error": metrics.last_error,
        "error_hint": metrics.error_hint,
    }


def _metrics_to_dict(
    snapshot: Optional[RuntimeMetricsSnapshot],
) -> Optional[Dict[str, object]]:
    if snapshot is None:
        return None

    return {
        "descriptors": [
            {
                "name": descriptor.name,
                "kind": descriptor.kind,
                "unit": descriptor.unit,
                "description": descriptor.description,
                "labels": descriptor.labels,
            }
            for descriptor in snapshot.descriptors
        ],
        "samples": [
            {
                "descriptor_index": sample.descriptor_index,
                "is_integral": sample.is_integral,
                "value": sample.value,
                "int_value": sample.int_value,
            }
            for sample in snapshot.samples
        ],
    }


def _scene_validation_to_dict(
    snapshot: Optional[SceneValidationSnapshot],
) -> Optional[Dict[str, object]]:
    if snapshot is None:
        return None

    return {
        "issue_count": snapshot.issue_count,
        "cycle_count": snapshot.cycle_count,
        "dangling_parent_count": snapshot.dangling_parent_count,
        "missing_parent_hierarchy_count": snapshot.missing_parent_hierarchy_count,
        "non_finite_transform_count": snapshot.non_finite_transform_count,
        "transform_mismatch_count": snapshot.transform_mismatch_count,
        "issues": [
            {
                "entity": issue.entity,
                "related": issue.related,
                "type": issue.type,
                "message": issue.message,
            }
            for issue in snapshot.issues
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


def build_profile_trace(
    samples: Sequence[FrameSample], title: str = "Runtime Dispatch Profiling"
) -> Dict[str, object]:
    """Transform captured frame samples into a Chrome trace payload.

    The trace groups dispatch kernels by frame using individual threads so
    external profilers can reconstruct the frame sequence while preserving the
    per-dispatch ordering captured by the runtime dispatcher.
    """

    events: List[Dict[str, object]] = []
    process_id = 1
    events.append(
        {
            "name": "process_name",
            "ph": "M",
            "pid": process_id,
            "args": {"name": title},
        }
    )

    for sample in samples:
        thread_id = sample.index
        events.append(
            {
                "name": "thread_name",
                "ph": "M",
                "pid": process_id,
                "tid": thread_id,
                "args": {"name": f"frame_{sample.index}"},
            }
        )

        frame_start = sample.simulation_time - sample.timestep
        if frame_start < 0.0:
            frame_start = 0.0
        frame_start_us = frame_start * 1_000_000.0
        cursor_us = frame_start_us

        for dispatch in sample.dispatches:
            duration_us = max(dispatch.duration_ms, 0.0) * 1000.0
            events.append(
                {
                    "name": dispatch.name,
                    "cat": dispatch.category,
                    "ph": "X",
                    "ts": cursor_us,
                    "dur": duration_us,
                    "pid": process_id,
                    "tid": thread_id,
                    "args": {
                        "frame_index": sample.index,
                        "simulation_time": sample.simulation_time,
                        "dt": sample.timestep,
                    },
                }
            )
            cursor_us += duration_us

        frame_duration_us = max(sample.frame_total_ms, 0.0) * 1000.0
        events.append(
            {
                "name": "frame.total",
                "cat": "frame",
                "ph": "X",
                "ts": frame_start_us,
                "dur": frame_duration_us,
                "pid": process_id,
                "tid": thread_id,
                "args": {
                    "frame_index": sample.index,
                    "simulation_time": sample.simulation_time,
                    "dt": sample.timestep,
                },
            }
        )

    metadata: Dict[str, object] = {
        "title": title,
        "frameCount": len(samples),
    }

    return {
        "traceEvents": events,
        "displayTimeUnit": "ms",
        "metadata": metadata,
    }


def write_profile_trace(
    samples: Sequence[FrameSample],
    path: Path,
    title: str = "Runtime Dispatch Profiling",
) -> None:
    """Persist a Chrome trace payload for captured frame samples."""

    payload = build_profile_trace(samples, title)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _metric_matches_prefix(name: str, prefixes: Optional[Sequence[str]]) -> bool:
    if prefixes is None:
        return True
    return any(name.startswith(prefix) for prefix in prefixes)


def _select_metrics(
    snapshot: RuntimeMetricsSnapshot,
    prefixes: Optional[Sequence[str]],
) -> List[Tuple[RuntimeMetricDescriptor, RuntimeMetricSample]]:
    entries: List[Tuple[RuntimeMetricDescriptor, RuntimeMetricSample, Tuple[Tuple[str, str], ...]]] = []
    descriptor_count = len(snapshot.descriptors)
    for sample in snapshot.samples:
        index = sample.descriptor_index
        if index >= descriptor_count:
            continue
        descriptor = snapshot.descriptors[index]
        if not _metric_matches_prefix(descriptor.name, prefixes):
            continue
        label_key = tuple(sorted(descriptor.labels.items()))
        entries.append((descriptor, sample, label_key))

    entries.sort(key=lambda entry: (entry[0].name, entry[2]))
    return [(descriptor, sample) for descriptor, sample, _ in entries]


def _format_metric_value(sample: RuntimeMetricSample) -> str:
    if sample.is_integral:
        return f"{sample.int_value}"
    return f"{sample.value:.4f}"


def _format_metric_unit(unit: str) -> str:
    return _METRIC_UNIT_SUFFIX.get(unit, f" {unit}" if unit else "")


def _format_metric_labels(labels: Dict[str, str]) -> str:
    if not labels:
        return ""
    parts = ", ".join(f"{key}={value}" for key, value in sorted(labels.items()))
    return f" [{parts}]"


def _print_metric_summary(
    snapshot: RuntimeMetricsSnapshot,
    prefixes: Optional[Sequence[str]],
) -> None:
    filtered = _select_metrics(snapshot, prefixes)
    if prefixes is None:
        prefix_info = ""
    else:
        joined = ", ".join(prefixes)
        prefix_info = f" (filter: {joined})" if joined else ""

    print(f"  metrics{prefix_info}:")
    if not filtered:
        if prefixes is None:
            print("    (no metrics available)")
        else:
            print("    (no metrics matched the provided prefix filters)")
        return

    current_group: Optional[str] = None
    for descriptor, sample in filtered:
        if "." in descriptor.name:
            group, leaf = descriptor.name.rsplit(".", maxsplit=1)
        else:
            group, leaf = "metrics", descriptor.name

        if group != current_group:
            print(f"    [{group}]")
            current_group = group

        value = _format_metric_value(sample)
        unit = _format_metric_unit(descriptor.unit)
        labels = _format_metric_labels(descriptor.labels)
        print(f"      {leaf:<32} {value}{unit}{labels}")


def _print_summary(
    samples: Sequence[FrameSample],
    verbose: bool,
    diagnostics: Optional[RuntimeDiagnosticsSnapshot],
    metric_prefixes: Optional[Sequence[str]],
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
        if diagnostics.metrics is not None:
            _print_metric_summary(diagnostics.metrics, metric_prefixes)
        if diagnostics.scene_validation is not None:
            scene = diagnostics.scene_validation
            print(
                "  hierarchy: "
                f"issues={scene.issue_count} cycles={scene.cycle_count} "
                f"dangling={scene.dangling_parent_count} "
                f"missing_parent={scene.missing_parent_hierarchy_count} "
                f"non_finite={scene.non_finite_transform_count} "
                f"mismatch={scene.transform_mismatch_count}"
            )
            if scene.issue_count:
                limit = min(len(scene.issues), 5)
                for issue in scene.issues[:limit]:
                    print(
                        "    "
                        f"[{issue.type}] entity={issue.entity} related={issue.related} message={issue.message}"
                    )
                if len(scene.issues) > limit:
                    print(f"    … {len(scene.issues) - limit} additional issue(s)")
        if diagnostics.streaming is not None:
            streaming = diagnostics.streaming
            print("  streaming metrics:")
            print(
                "    "
                f"workers={streaming.worker_count} "
                f"active={streaming.active_workers} "
                f"pending_tasks={streaming.pending_tasks}"
            )
            print(
                "    scheduler totals: "
                f"enqueued={streaming.total_enqueued} "
                f"executed={streaming.total_executed}"
            )
            print(
                "    queue states: "
                f"pending={streaming.streaming_pending} "
                f"loading={streaming.streaming_loading} "
                f"completed={streaming.streaming_total_completed} "
                f"failed={streaming.streaming_total_failed} "
                f"cancelled={streaming.streaming_total_cancelled} "
                f"rejected={streaming.streaming_total_rejected}"
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
    parser.add_argument(
        "--metric-prefix",
        action="append",
        default=None,
        metavar="PREFIX",
        help=(
            "Print metrics whose fully-qualified names start with PREFIX. May be provided multiple times. "
            "Defaults to 'runtime.streaming.' when no prefixes are specified."
        ),
    )
    parser.add_argument(
        "--metrics-all",
        action="store_true",
        help=(
            "Ignore prefix filtering and display every metric in the runtime telemetry snapshot."
        ),
    )
    parser.add_argument(
        "--profile-trace",
        type=Path,
        default=None,
        help=(
            "Optional Chrome trace JSON file capturing per-dispatch profiling data for external tooling."
        ),
    )
    parser.add_argument(
        "--profile-trace-title",
        default="Runtime Dispatch Profiling",
        help=(
            "Label to embed in the exported Chrome trace metadata (default: 'Runtime Dispatch Profiling')."
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

    if args.metrics_all:
        metric_prefixes: Optional[Sequence[str]] = None
    else:
        prefixes = tuple(args.metric_prefix or ())
        metric_prefixes = prefixes if prefixes else ("runtime.streaming.",)

    _print_summary(samples, args.verbose, diagnostics, metric_prefixes)

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
    if args.profile_trace is not None:
        write_profile_trace(samples, args.profile_trace, args.profile_trace_title)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
