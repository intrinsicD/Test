#!/usr/bin/env python3
"""Drive the canonical CMake presets for CI smoke coverage."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

PIPELINES = {
    "linux": (
        {
            "configure": "linux-gcc-debug",
            "build": "linux-gcc-debug",
            "test": "linux-gcc-debug",
        },
        {
            "configure": "linux-gcc-release",
            "build": "linux-gcc-release",
            "test": "linux-gcc-release",
        },
    ),
    "windows": (
        {
            "configure": "windows-msvc-debug",
            "build": "windows-msvc-debug",
            "test": "windows-msvc-debug",
        },
        {
            "configure": "windows-msvc-release",
            "build": "windows-msvc-release",
            "test": "windows-msvc-release",
        },
    ),
}


def run_command(command: list[str]) -> None:
    print(f"[ci] running: {' '.join(command)}", flush=True)
    subprocess.run(command, cwd=REPO_ROOT, check=True)


def drive_pipeline(pipeline: dict[str, str]) -> None:
    run_command(["cmake", "--preset", pipeline["configure"]])
    run_command(["cmake", "--build", "--preset", pipeline["build"]])
    run_command(["ctest", "--preset", pipeline["test"]])


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--platform",
        choices=tuple(PIPELINES.keys()) + ("all",),
        default=None,
        help=(
            "Select which platform preset pipeline to execute. "
            "Defaults to the host platform if omitted."
        ),
    )
    parser.add_argument(
        "--skip-release",
        action="store_true",
        help="Only run debug presets for the selected platforms.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.platform is None:
        args.platform = "windows" if sys.platform.startswith("win") else "linux"

    selected_platforms: tuple[str, ...]
    if args.platform == "all":
        selected_platforms = tuple(PIPELINES.keys())
    else:
        selected_platforms = (args.platform,)

    try:
        for platform in selected_platforms:
            pipelines = PIPELINES[platform]
            if args.skip_release:
                pipelines = pipelines[:1]
            for pipeline in pipelines:
                drive_pipeline(pipeline)
    except subprocess.CalledProcessError as error:
        print(f"[ci] command failed with exit code {error.returncode}", file=sys.stderr)
        return error.returncode

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
