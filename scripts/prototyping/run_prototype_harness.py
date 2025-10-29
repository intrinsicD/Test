"""CLI entry point for executing the AI-004 runtime prototype harness."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Iterable

PROJECT_ROOT = Path(__file__).resolve().parents[2]
PYTHON_ROOT = PROJECT_ROOT / "python"
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

from engine3g.case_studies import (  # type: ignore
    CaseStudyError,
    available_case_studies,
    get_case_study,
)
from engine3g.prototype_harness import (  # type: ignore
    configuration_summary_to_dict,
    HarnessExecutionOptions,
    PrototypeHarness,
    PrototypeHarnessError,
    run_summary_to_dict,
    load_harness,
    summarize,
)


def _format_case_study_help() -> str:
    cases = available_case_studies()
    if not cases:
        return "Identifier of a packaged case study configuration."
    entries = ", ".join(
        f"{case.identifier} ({case.label})" if case.label != case.identifier else case.identifier
        for case in cases
    )
    return f"Identifier of a packaged case study configuration. Available: {entries}."


def _make_options(args: argparse.Namespace) -> HarnessExecutionOptions:
    return HarnessExecutionOptions(frames=args.frames, dt=args.dt, dry_run=args.dry_run)


def _print_summary(harness: PrototypeHarness) -> None:
    dataset = harness.selected_dataset.identifier if harness.selected_dataset else "<none>"
    rendering = harness.configuration.rendering
    preset = rendering.preset if rendering else "<unspecified>"
    shading = rendering.shading_mode if rendering else "<unspecified>"
    print(f"Configuration: dataset={dataset} preset={preset} shading={shading}")


def _print_benchmark_scenarios(summary) -> None:
    benchmarks = summary.benchmarks
    if benchmarks is None or not benchmarks.scenarios:
        print("Benchmark scenarios: <none>")
        return
    print("Benchmark scenarios:")
    for scenario in benchmarks.scenarios:
        dataset = scenario.dataset or "<unspecified>"
        preset = scenario.rendering_preset or "<unspecified>"
        runtime_profile = scenario.runtime_profile or "<unspecified>"
        print(
            "  - "
            f"{scenario.identifier} ({scenario.name}) "
            f"dataset={dataset} preset={preset} runtime_profile={runtime_profile}"
        )


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _resolve_config_argument(args: argparse.Namespace) -> tuple[Path, str | None]:
    if args.case_study is not None:
        case = get_case_study(args.case_study)
        return case.config_path, case.label
    assert args.config is not None  # argparse enforces mutually exclusive requirements
    return args.config, None


def _run(args: argparse.Namespace) -> int:
    try:
        config_path, case_label = _resolve_config_argument(args)
    except CaseStudyError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    try:
        harness = load_harness(str(config_path), require_schema=True if args.require_schema else None)
    except PrototypeHarnessError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if args.case_study is not None:
        label_suffix = f" ({case_label})" if case_label else ""
        print(f"Selected case study '{args.case_study}'{label_suffix} from {config_path}")

    _print_summary(harness)

    description = None
    if args.describe_json is not None or args.list_benchmarks:
        description = harness.describe_configuration()
    if args.describe_json is not None and description is not None:
        _write_json(args.describe_json, configuration_summary_to_dict(description))
    if args.list_benchmarks and description is not None:
        _print_benchmark_scenarios(description)

    if args.dry_run:
        summary = harness.run_headless(HarnessExecutionOptions(dry_run=True, frames=1, dt=args.dt))
        print(f"Dry run summary: {summarize(summary)}")
        if args.summary_json is not None:
            _write_json(args.summary_json, run_summary_to_dict(summary))
        return 0

    options = _make_options(args)
    try:
        summary = harness.run_headless(options)
    except PrototypeHarnessError as error:
        print(f"error: {error}", file=sys.stderr)
        return 3

    print(f"Execution summary: {summarize(summary)}")
    if args.summary_json is not None:
        _write_json(args.summary_json, run_summary_to_dict(summary))
    return 0


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    case_studies_list = available_case_studies()
    case_study_ids = [case.identifier for case in case_studies_list]
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--config",
        type=Path,
        help="Path to an AI-004 configuration manifest (YAML or JSON).",
    )
    group.add_argument(
        "--case-study",
        choices=case_study_ids if case_study_ids else None,
        help=_format_case_study_help(),
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
    parser.add_argument(
        "--describe-json",
        type=Path,
        help=(
            "Write a JSON description of datasets, rendering presets, and runtime parameters "
            "for the sandbox UI."
        ),
    )
    parser.add_argument(
        "--summary-json",
        type=Path,
        help="Write the execution summary to the specified JSON file.",
    )
    parser.add_argument(
        "--list-benchmarks",
        action="store_true",
        help="List benchmark scenarios defined in the configuration before execution.",
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.frames <= 0 and not args.dry_run:
        parser.error("--frames must be greater than zero unless --dry-run is specified")

    return _run(args)


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())

