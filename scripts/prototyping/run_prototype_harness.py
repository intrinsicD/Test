"""CLI entry point for executing the AI-004 runtime prototype harness."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable

PROJECT_ROOT = Path(__file__).resolve().parents[1]
PYTHON_ROOT = PROJECT_ROOT / "python"
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

from engine3g.prototype_harness import (  # type: ignore
    HarnessExecutionOptions,
    PrototypeHarness,
    PrototypeHarnessError,
    load_harness,
    summarize,
)


def _make_options(args: argparse.Namespace) -> HarnessExecutionOptions:
    return HarnessExecutionOptions(frames=args.frames, dt=args.dt, dry_run=args.dry_run)


def _print_summary(harness: PrototypeHarness) -> None:
    dataset = harness.selected_dataset.identifier if harness.selected_dataset else "<none>"
    rendering = harness.configuration.rendering
    preset = rendering.preset if rendering else "<unspecified>"
    shading = rendering.shading_mode if rendering else "<unspecified>"
    print(f"Configuration: dataset={dataset} preset={preset} shading={shading}")


def _run(args: argparse.Namespace) -> int:
    try:
        harness = load_harness(str(args.config), require_schema=True if args.require_schema else None)
    except PrototypeHarnessError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    _print_summary(harness)

    if args.dry_run:
        summary = harness.run_headless(HarnessExecutionOptions(dry_run=True, frames=1, dt=args.dt))
        print(f"Dry run summary: {summarize(summary)}")
        return 0

    options = _make_options(args)
    try:
        summary = harness.run_headless(options)
    except PrototypeHarnessError as error:
        print(f"error: {error}", file=sys.stderr)
        return 3

    print(f"Execution summary: {summarize(summary)}")
    return 0


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--config",
        required=True,
        type=Path,
        help="Path to an AI-004 configuration manifest (YAML or JSON).",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=600,
        help="Number of frames to execute (ignored with --dry-run).",
    )
    parser.add_argument(
        "--dt",
        type=float,
        default=1.0 / 60.0,
        help="Simulation timestep in seconds.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate configuration without loading the runtime library.",
    )
    parser.add_argument(
        "--require-schema",
        action="store_true",
        help=(
            "Fail if configuration or dataset sections omit ai-004 schema headers. "
            "When unset the harness honours the ENGINE_AI004_SCHEMA_V1 feature flag."
        ),
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.frames <= 0 and not args.dry_run:
        parser.error("--frames must be greater than zero unless --dry-run is specified")

    return _run(args)


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())

