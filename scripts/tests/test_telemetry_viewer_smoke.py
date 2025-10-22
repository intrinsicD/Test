from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

RUNTIME_LIBRARY_ENV = "TEST_ENGINE_RUNTIME_LIBRARY_DIR"


def _has_runtime_library(directory: Path) -> bool:
    for pattern in (
        "libengine_runtime*.so",
        "libengine_runtime*.dylib",
        "engine_runtime*.dll",
    ):
        if any(directory.glob(pattern)):
            return True
    return False


def test_telemetry_viewer_cli_smoke(tmp_path: Path) -> None:
    env_value = os.environ.get(RUNTIME_LIBRARY_ENV)
    if not env_value:
        pytest.skip(
            "set TEST_ENGINE_RUNTIME_LIBRARY_DIR to the build output containing "
            "the engine runtime shared library to enable the smoke test"
        )

    runtime_dir = Path(env_value)
    if not runtime_dir.is_dir():
        pytest.fail(
            f"runtime library directory {runtime_dir} does not exist or is not a directory"
        )
    if not _has_runtime_library(runtime_dir):
        pytest.fail(
            f"no engine runtime shared library found under {runtime_dir}; "
            "ensure CI packages the runtime artifacts"
        )

    telemetry_output = tmp_path / "runtime_telemetry.json"
    capture_cmd = [
        sys.executable,
        str(Path(__file__).resolve().parents[1] / "diagnostics" / "runtime_frame_telemetry.py"),
        "--library-dir",
        str(runtime_dir),
        "--frames",
        "4",
        "--dt",
        "0.016",
        "--window-backend",
        "mock",
        "--output",
        str(telemetry_output),
    ]
    capture_result = subprocess.run(
        capture_cmd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if capture_result.returncode != 0:
        pytest.fail(
            "runtime telemetry capture failed\n"
            f"stdout:\n{capture_result.stdout}\n"
            f"stderr:\n{capture_result.stderr}"
        )
    assert telemetry_output.is_file(), "runtime telemetry output was not created"

    viewer_cmd = [
        sys.executable,
        str(Path(__file__).resolve().parents[1] / "diagnostics" / "telemetry_viewer.py"),
        "--input",
        str(telemetry_output),
        "--metric-prefix",
        "runtime.lifecycle.",
        "--max-issues",
        "0",
    ]
    viewer_result = subprocess.run(
        viewer_cmd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert viewer_result.returncode == 0, (
        "telemetry viewer CLI failed\n"
        f"stdout:\n{viewer_result.stdout}\n"
        f"stderr:\n{viewer_result.stderr}"
    )
    assert "Runtime Telemetry Viewer" in viewer_result.stdout
