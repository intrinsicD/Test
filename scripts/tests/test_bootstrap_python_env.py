from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from typing import List

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts import bootstrap_python_env as bootstrap


def _simulate_venv_creation(args: List[str]) -> None:
    if len(args) >= 4 and args[1:3] == ["-m", "venv"]:
        env_dir = Path(args[3])
        scripts_dir = "Scripts" if os.name == "nt" else "bin"
        python_name = "python.exe" if os.name == "nt" else "python"
        bin_dir = env_dir / scripts_dir
        bin_dir.mkdir(parents=True, exist_ok=True)
        (bin_dir / python_name).write_text("", encoding="utf-8")


def test_ensure_virtualenv_creates_directory(tmp_path: Path) -> None:
    venv_path = tmp_path / "env"
    bootstrap.ensure_virtualenv(venv_path, sys.executable)
    assert (venv_path / ("Scripts" if os.name == "nt" else "bin")).is_dir()


def test_install_requirements_invokes_pip(tmp_path: Path) -> None:
    python_dir = tmp_path / "venv" / ("Scripts" if os.name == "nt" else "bin")
    python_dir.mkdir(parents=True)
    python_exe = python_dir / ("python.exe" if os.name == "nt" else "python")
    python_exe.write_text("", encoding="utf-8")

    requirements = tmp_path / "requirements.txt"
    requirements.write_text("pytest==0.0", encoding="utf-8")

    invoked: List[List[str]] = []

    def fake_runner(args: List[str]) -> subprocess.CompletedProcess[str]:
        invoked.append(list(args))
        return subprocess.CompletedProcess(args, 0)

    bootstrap.install_requirements(python_exe, requirements, runner=fake_runner)

    assert invoked[0][:4] == [str(python_exe), "-m", "pip", "install"]
    assert invoked[1] == [str(python_exe), "-m", "pip", "install", "-r", str(requirements)]


def test_install_requirements_missing_manifest(tmp_path: Path) -> None:
    python_path = tmp_path / "python"
    python_path.write_text("", encoding="utf-8")
    with pytest.raises(bootstrap.BootstrapError):
        bootstrap.install_requirements(python_path, tmp_path / "missing.txt")


def test_format_activation_instructions_includes_pythonpath(tmp_path: Path) -> None:
    env_dir = tmp_path / "env"
    env_dir.mkdir()
    instructions = bootstrap.format_activation_instructions(env_dir)
    assert "PYTHONPATH" in instructions


def test_bootstrap_environment_skip_install(tmp_path: Path) -> None:
    venv_path = tmp_path / "env"

    commands: List[List[str]] = []

    def fake_runner(args: List[str]) -> subprocess.CompletedProcess[str]:
        commands.append(list(args))
        _simulate_venv_creation(args)
        return subprocess.CompletedProcess(args, 0)

    args = bootstrap.parse_arguments([
        "--venv-path",
        str(venv_path),
        "--skip-install",
    ])
    args.python = sys.executable

    result = bootstrap.bootstrap_environment(args, runner=fake_runner)

    assert result.venv_path == venv_path
    assert not result.requirements_installed
    assert any(cmd[1:3] == ["-m", "venv"] for cmd in commands)


def test_bootstrap_environment_installs_dependencies(tmp_path: Path) -> None:
    venv_path = tmp_path / "env"
    requirements = tmp_path / "requirements.txt"
    requirements.write_text("pytest==0.0", encoding="utf-8")

    commands: List[List[str]] = []

    def fake_runner(args: List[str]) -> subprocess.CompletedProcess[str]:
        commands.append(list(args))
        _simulate_venv_creation(args)
        return subprocess.CompletedProcess(args, 0)

    args = bootstrap.parse_arguments([
        "--venv-path",
        str(venv_path),
        "--requirements",
        str(requirements),
    ])
    args.python = sys.executable

    result = bootstrap.bootstrap_environment(args, runner=fake_runner)

    assert result.requirements_installed
    pip_invocations = [cmd for cmd in commands if cmd[1:3] == ["-m", "pip"]]
    assert len(pip_invocations) == 2
