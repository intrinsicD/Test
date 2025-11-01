import ctypes
import importlib.util
import json
import sys
from pathlib import Path
from typing import Optional

import pytest


MODULE_PATH = Path(__file__).resolve().parents[1] / "diagnostics" / "streaming_report.py"
SPEC = importlib.util.spec_from_file_location("streaming_report", MODULE_PATH)
assert SPEC and SPEC.loader  # narrow type check for mypy/linters
report = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = report
SPEC.loader.exec_module(report)


class StubFunction:
    def __init__(self, func):
        self._func = func
        self.restype = None
        self.argtypes = None

    def __call__(self, *args):
        return self._func(*args)


class StubRuntimeLibrary:
    def __init__(self) -> None:
        self._streaming_capacity = 2
        self.engine_runtime_streaming_geometry_failure_capacity_value = StubFunction(
            lambda: self._streaming_capacity
        )
        self.engine_runtime_streaming_metrics = StubFunction(self._fill_streaming_metrics)
        self.engine_runtime_diagnostic_hot_reload_metrics = StubFunction(
            self._fill_hot_reload_metrics
        )
        self.engine_runtime_diagnostic_hot_reload_recent_failure_count = StubFunction(
            lambda: len(self._recent_failures)
        )
        def _resolve(index: object) -> int:
            if isinstance(index, ctypes.c_uint):
                return int(index.value)
            return int(index)

        self.engine_runtime_diagnostic_hot_reload_recent_failure_identifier = StubFunction(
            lambda index: self._recent_failures[_resolve(index)]["identifier"].encode("utf-8")
        )
        self.engine_runtime_diagnostic_hot_reload_recent_failure_error = StubFunction(
            lambda index: self._recent_failures[_resolve(index)]["error"].encode("utf-8")
        )
        self.engine_runtime_diagnostic_hot_reload_recent_failure_hint = StubFunction(
            lambda index: self._recent_failures[_resolve(index)]["hint"].encode("utf-8")
        )

        self._recent_failures = [
            {
                "identifier": "mesh/hero",
                "error": "Decode failed",
                "hint": "Re-export asset",
            },
            {
                "identifier": "mesh/tree",
                "error": "IO failure",
                "hint": "Check watcher permissions",
            },
        ]
        self._failure_labels: list[ctypes.Array[ctypes.c_char]] = []

    def _fill_streaming_metrics(self, out_ptr) -> None:
        metrics = ctypes.cast(out_ptr, ctypes.POINTER(report.StreamingMetrics)).contents
        metrics.worker_count = 4
        metrics.queue_capacity = 16
        metrics.pending_tasks = 3
        metrics.active_workers = 2
        metrics.total_enqueued = 10
        metrics.total_executed = 8
        metrics.streaming_pending = 1
        metrics.streaming_loading = 2
        metrics.streaming_total_requests = 6
        metrics.streaming_total_completed = 4
        metrics.streaming_total_failed = 1
        metrics.streaming_total_cancelled = 1
        metrics.streaming_total_rejected = 0
        metrics.geometry_failure_count = 2
        metrics.geometry_failures[0] = 1
        metrics.geometry_failures[1] = 0
        self._failure_labels = [
            ctypes.create_string_buffer(b"geometry.io_failure"),
            ctypes.create_string_buffer(b"geometry.validation"),
        ]
        metrics.geometry_failure_labels[0] = ctypes.cast(
            self._failure_labels[0], ctypes.POINTER(ctypes.c_char)
        )
        metrics.geometry_failure_labels[1] = ctypes.cast(
            self._failure_labels[1], ctypes.POINTER(ctypes.c_char)
        )

    def _fill_hot_reload_metrics(self, out_ptr) -> None:
        metrics = ctypes.cast(out_ptr, ctypes.POINTER(report.HotReloadMetrics)).contents
        metrics.attempt_count = 5
        metrics.failure_count = 2
        metrics.cancelled_count = 1
        metrics.rejected_count = 0
        metrics.pending_count = 1
        metrics.loading_count = 0
        metrics.total_requests = 5
        metrics.last_error = b"Decode failed"
        metrics.error_hint = b"Re-export asset"


@pytest.fixture
def stub_bindings(monkeypatch: pytest.MonkeyPatch) -> report.RuntimeStreamingBindings:
    library = StubRuntimeLibrary()

    def _load(_name: str, _directory: Optional[Path]) -> report.RuntimeStreamingBindings:
        return report.RuntimeStreamingBindings(library)

    monkeypatch.setattr(report.RuntimeStreamingBindings, "load", staticmethod(_load))
    return report.RuntimeStreamingBindings(library)


def test_collects_hot_reload_metrics(
    stub_bindings: report.RuntimeStreamingBindings,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(sys, "argv", ["streaming_report.py"])
    report.main()
    captured = capsys.readouterr().out
    payload = json.loads(captured)

    assert payload["hot_reload"]["attempt_count"] == 5
    assert payload["hot_reload"]["failure_count"] == 2
    assert payload["hot_reload"]["cancelled_count"] == 1
    assert payload["hot_reload"]["recent_failures"][0]["identifier"] == "mesh/hero"
    failures = payload["geometry_failures_by_error"]
    observed = failures.get("geometry.io_failure", failures.get("error_0"))
    assert observed == 1


def test_renders_text_dashboard(
    stub_bindings: report.RuntimeStreamingBindings,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(sys, "argv", ["streaming_report.py", "--format", "text"])
    report.main()
    captured = capsys.readouterr().out
    assert "Streaming Queue" in captured
    assert "Hot Reload Summary" in captured
    assert "Geometry Failure Attribution" in captured


def test_emits_chrome_trace(
    stub_bindings: report.RuntimeStreamingBindings,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    trace_path = tmp_path / "trace.json"
    monkeypatch.setattr(
        sys,
        "argv",
        ["streaming_report.py", "--chrome-trace", str(trace_path)],
    )
    report.main()
    _ = capsys.readouterr()
    trace_payload = json.loads(trace_path.read_text(encoding="utf-8"))
    assert "traceEvents" in trace_payload
    counters = [event for event in trace_payload["traceEvents"] if event.get("ph") == "C"]
    assert any(event.get("name") == "Streaming Counters" for event in counters)
    assert any(event.get("name") == "Hot Reload Counters" for event in counters)
