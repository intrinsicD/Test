from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest

_BINARY_ENV = "TOOLS_EDITOR_SMOKE_BINARY"
_PRESET_ENV = "TOOLS_EDITOR_SMOKE_PRESET"
_DEFAULT_PRESET = "linux-gcc-debug"
_GTEST_FILTER = "SandboxConfigurationLoader.*:PanelRegistry.*:RuntimePanelBridge.*"


def _candidate_binaries(repo_root: Path, preset: str) -> list[Path]:
    build_roots = [
        repo_root / "out" / "build" / preset,
        repo_root / "build" / preset,
        repo_root / "out" / "build",
        repo_root / "build",
    ]
    binary_names = ("test_tools_module", "test_tools_module.exe")
    configuration_dirs = ("Debug", "RelWithDebInfo", "Release")

    candidates: list[Path] = []
    for root in build_roots:
        for name in binary_names:
            candidates.append(root / "engine" / "tools" / "tests" / name)
            for config in configuration_dirs:
                candidates.append(root / "engine" / "tools" / "tests" / config / name)
        for name in binary_names:
            candidates.append(root / name)
            for config in configuration_dirs:
                candidates.append(root / config / name)
    return candidates


def _resolve_binary(repo_root: Path) -> Path | None:
    env_value = os.environ.get(_BINARY_ENV)
    if env_value:
        binary = Path(env_value)
        if not binary.is_file():
            pytest.fail(
                f"editor smoke binary {binary} does not exist;"
                " build test_tools_module or adjust TOOLS_EDITOR_SMOKE_BINARY"
            )
        return binary

    preset = os.environ.get(_PRESET_ENV, _DEFAULT_PRESET)
    for candidate in _candidate_binaries(repo_root, preset):
        if candidate.is_file():
            return candidate
    return None


def test_editor_smoke(tmp_path: Path) -> None:
    del tmp_path
    repo_root = Path(__file__).resolve().parents[2]
    binary = _resolve_binary(repo_root)
    if binary is None:
        pytest.skip(
            "test_tools_module binary not found; "
            "run 'cmake --build --preset linux-gcc-debug --target test_tools_module' "
            "or set TOOLS_EDITOR_SMOKE_BINARY to the compiled executable"
        )

    result = subprocess.run(
        [
            str(binary),
            f"--gtest_filter={_GTEST_FILTER}",
            "--gtest_color=no",
        ],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=binary.parent,
    )

    if result.returncode != 0:
        pytest.fail(
            "editor smoke scenario failed\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )

    assert "SandboxConfigurationLoader." in result.stdout
    assert "PanelRegistry." in result.stdout
    assert "RuntimePanelBridge." in result.stdout
    assert "[  PASSED  ]" in result.stdout
