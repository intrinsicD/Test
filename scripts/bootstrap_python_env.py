"""Utility for provisioning the workspace Python environment.

This script codifies the manual steps documented in :mod:`python/README.md`
for roadmap item ``PY-015``.  It performs the following actions:

* Creates (or recreates) a virtual environment at the requested location.
* Upgrades ``pip`` inside the environment unless explicitly disabled.
* Installs dependencies from ``python/requirements.txt``.
* Prints shell-specific activation and ``PYTHONPATH`` guidance.

The implementation intentionally exposes small, testable helpers so the
automation layer under ``scripts/tests`` can validate behaviour without
touching the real developer environment.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable, Sequence


Runner = Callable[[Sequence[str]], subprocess.CompletedProcess[str]]


class BootstrapError(RuntimeError):
    """Raised when provisioning the Python environment fails."""


@dataclass(frozen=True)
class BootstrapResult:
    """Summary of the executed bootstrap actions."""

    venv_path: Path
    python_path: Path
    requirements_installed: bool


def _default_runner(args: Sequence[str]) -> subprocess.CompletedProcess[str]:
    """Run the given command via :func:`subprocess.run` with ``check=True``."""

    return subprocess.run(list(args), check=True, text=True, capture_output=False)


def parse_arguments(argv: Sequence[str] | None = None) -> argparse.Namespace:
    """Parse command line arguments for the bootstrap script."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--venv-path",
        type=Path,
        default=Path(".venv"),
        help="Directory that will host the virtual environment (default: .venv)",
    )
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="Python interpreter used to create the virtual environment",
    )
    parser.add_argument(
        "--requirements",
        type=Path,
        default=Path("python/requirements.txt"),
        help="Requirements manifest installed into the environment",
    )
    parser.add_argument(
        "--recreate",
        action="store_true",
        help="Remove any existing environment before provisioning a new one",
    )
    parser.add_argument(
        "--skip-install",
        action="store_true",
        help="Create the environment without installing dependencies",
    )
    parser.add_argument(
        "--no-upgrade-pip",
        dest="upgrade_pip",
        action="store_false",
        help="Skip upgrading pip prior to installing dependencies",
    )
    parser.set_defaults(upgrade_pip=True)
    return parser.parse_args(argv)


def ensure_virtualenv(
    venv_path: Path,
    python_executable: str,
    recreate: bool = False,
    runner: Runner | None = None,
) -> None:
    """Create the requested virtual environment if required."""

    if venv_path.exists():
        if recreate:
            shutil.rmtree(venv_path)
        else:
            return

    venv_path.parent.mkdir(parents=True, exist_ok=True)
    runner = runner or _default_runner
    try:
        runner([python_executable, "-m", "venv", str(venv_path)])
    except subprocess.CalledProcessError as exc:  # pragma: no cover - defensive
        raise BootstrapError(f"Failed to create virtual environment: {exc}") from exc


def resolve_venv_python(venv_path: Path) -> Path:
    """Return the interpreter inside the provided virtual environment."""

    scripts_dir = "Scripts" if os.name == "nt" else "bin"
    python_name = "python.exe" if os.name == "nt" else "python"
    python_path = venv_path / scripts_dir / python_name
    if not python_path.exists():  # pragma: no cover - defensive
        raise BootstrapError(
            f"Python executable not found in virtual environment at {python_path}"
        )
    return python_path


def install_requirements(
    python_path: Path,
    requirements_file: Path,
    upgrade_pip: bool = True,
    runner: Runner | None = None,
) -> None:
    """Install workspace dependencies into the provided environment."""

    if not requirements_file.is_file():
        raise BootstrapError(f"Requirements file not found: {requirements_file}")

    commands: list[list[str]] = []
    if upgrade_pip:
        commands.append([str(python_path), "-m", "pip", "install", "--upgrade", "pip"])
    commands.append([str(python_path), "-m", "pip", "install", "-r", str(requirements_file)])

    runner = runner or _default_runner
    for command in commands:
        try:
            runner(command)
        except subprocess.CalledProcessError as exc:  # pragma: no cover - defensive
            raise BootstrapError(f"Dependency installation failed: {exc}") from exc


def format_activation_instructions(venv_path: Path) -> str:
    """Return shell-specific activation and ``PYTHONPATH`` guidance."""

    repo_root = venv_path.resolve().parent.resolve()
    posix_activate = venv_path / "bin" / "activate"
    windows_activate = venv_path / "Scripts" / "Activate.ps1"
    posix_path_export = f"export PYTHONPATH=\"{repo_root / 'python'}:$PYTHONPATH\""
    windows_path_export = (
        "$env:PYTHONPATH = \"{repo_root / 'python'};\" + $env:PYTHONPATH"
    )

    lines = [
        "Next steps:",
        f"  • POSIX:   source {posix_activate}",
        f"             {posix_path_export}",
        f"  • Windows: {windows_activate}",
        f"             {windows_path_export}",
        "  • Run pytest via 'pytest python/tests scripts/tests'",
    ]
    return "\n".join(lines)


def bootstrap_environment(
    args: argparse.Namespace, runner: Runner | None = None
) -> BootstrapResult:
    """Execute the bootstrap workflow for the provided arguments."""

    ensure_virtualenv(args.venv_path, args.python, args.recreate, runner=runner)
    python_path = resolve_venv_python(args.venv_path)
    installed = False
    if not args.skip_install:
        install_requirements(
            python_path, args.requirements, args.upgrade_pip, runner=runner
        )
        installed = True
    return BootstrapResult(args.venv_path, python_path, installed)


def main(argv: Iterable[str] | None = None) -> int:
    """CLI entry point."""

    args = parse_arguments(list(argv) if argv is not None else None)
    result = bootstrap_environment(args)
    print(  # noqa: T201 - intentional user-facing output
        "Python environment provisioned at", result.venv_path
    )
    print(  # noqa: T201 - intentional user-facing output
        "Using interpreter:", result.python_path
    )
    if result.requirements_installed:
        print("Dependencies installed from", args.requirements)  # noqa: T201
    else:
        print("Dependencies were not installed (--skip-install)")  # noqa: T201
    print(format_activation_instructions(result.venv_path))  # noqa: T201
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI behaviour
    raise SystemExit(main())
