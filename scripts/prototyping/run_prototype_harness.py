"""CLI entry point for executing the AI-004 runtime prototype harness."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import replace
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
    describe_case_studies,
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


def _summary_path_for_run(base: Path, index: int, total: int) -> Path:
    if total <= 1:
        return base
    width = max(2, len(str(total)))
    suffix = "".join(base.suffixes)
    name = base.name
    if suffix:
        stem = name[: -len(suffix)]
    else:
        stem = name
    run_token = f"run{index:0{width}d}"
    new_name = f"{stem}-{run_token}{suffix}"
    return base.with_name(new_name)


def _format_run_prefix(index: int, total: int) -> str:
    if total <= 1:
        return ""
    return f" [{index}/{total}]"


def _list_case_studies(json_path: Path | None) -> int:
    summaries = describe_case_studies(relative_to=PROJECT_ROOT)
    if not summaries:
        print("No case studies registered.")
    else:
        print("Available case studies:")
        for entry in summaries:
            tags = ", ".join(entry["tags"]) if entry["tags"] else "<none>"
            print(
                "  - "
                f"{entry['id']} ({entry['label']}) "
                f"tags=[{tags}] config={entry['config']}"
            )
    if json_path is not None:
        _write_json(json_path, {"case_studies": list(summaries)})
    return 0


def _resolve_config_argument(args: argparse.Namespace) -> tuple[Path, str | None]:
    if args.case_study is not None:
        case = get_case_study(args.case_study)
        return case.config_path, case.label
    assert args.config is not None  # validated by main()
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
        total_runs = args.repeat
        for index in range(1, total_runs + 1):
            summary = harness.run_headless(HarnessExecutionOptions(dry_run=True, frames=1, dt=args.dt))
            run_count = total_runs if total_runs > 1 else None
            summary = replace(
                summary,
                run_index=index if run_count is not None else None,
                run_count=run_count,
            )
            print(f"Dry run summary{_format_run_prefix(index, total_runs)}: {summarize(summary)}")
            if args.summary_json is not None:
                _write_json(
                    _summary_path_for_run(args.summary_json, index, total_runs),
                    run_summary_to_dict(summary),
                )
        return 0

    options = _make_options(args)
    total_runs = args.repeat
    for index in range(1, total_runs + 1):
        try:
            summary = harness.run_headless(options)
        except PrototypeHarnessError as error:
            print(f"error: {error}", file=sys.stderr)
            return 3
        run_count = total_runs if total_runs > 1 else None
        summary = replace(
            summary,
            run_index=index if run_count is not None else None,
            run_count=run_count,
        )
        print(f"Execution summary{_format_run_prefix(index, total_runs)}: {summarize(summary)}")
        if args.summary_json is not None:
            _write_json(
                _summary_path_for_run(args.summary_json, index, total_runs),
                run_summary_to_dict(summary),
            )
    return 0


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    case_studies_list = available_case_studies()
    case_study_ids = [case.identifier for case in case_studies_list]
    group = parser.add_mutually_exclusive_group()
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
        "--repeat",
        type=int,
        default=1,
        help="Number of sequential runs to execute (default: 1).",
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
    parser.add_argument(
        "--list-case-studies",
        action="store_true",
        help="List bundled case studies without executing the harness.",
    )
    parser.add_argument(
        "--case-studies-json",
        type=Path,
        help="Write the registered case studies and resolved paths to the specified JSON file.",
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.list_case_studies:
        if args.summary_json is not None or args.describe_json is not None or args.list_benchmarks:
            parser.error("--list-case-studies cannot be combined with configuration execution options")
        if args.case_studies_json is not None and args.case_studies_json.is_dir():
            parser.error("--case-studies-json must reference a file path")
        return _list_case_studies(args.case_studies_json)

    if args.case_studies_json is not None:
        parser.error("--case-studies-json requires --list-case-studies")

    if args.frames <= 0 and not args.dry_run:
        parser.error("--frames must be greater than zero unless --dry-run is specified")

    if args.repeat <= 0:
        parser.error("--repeat must be greater than zero")

    if args.config is None and args.case_study is None:
        parser.error("one of --config or --case-study is required")

    return _run(args)


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())

