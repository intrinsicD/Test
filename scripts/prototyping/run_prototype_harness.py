"""CLI entry point for executing the AI-004 runtime prototype harness."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import replace
from pathlib import Path
from typing import Iterable, Optional

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
    DatasetSummary,
    HarnessConfigurationSummary,
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


def _parse_boolean_token(value: str) -> bool:
    lowered = value.strip().lower()
    if lowered in {"1", "true", "yes", "on"}:
        return True
    if lowered in {"0", "false", "no", "off"}:
        return False
    raise argparse.ArgumentTypeError(f"invalid boolean value '{value}'")


def _parse_overlay_argument(value: str) -> tuple[str, bool]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("overlay overrides must be provided as key=value")
    key, raw = value.split("=", 1)
    key = key.strip()
    if not key:
        raise argparse.ArgumentTypeError("overlay overrides require a non-empty key")
    return key, _parse_boolean_token(raw)


def _collect_overlay_overrides(entries: Iterable[tuple[str, bool]]) -> Optional[dict[str, bool]]:
    overrides: dict[str, bool] = {}
    for key, value in entries:
        overrides[key] = value
    return overrides or None


def _parse_shading_mode(value: str) -> str:
    lowered = value.strip().lower()
    if lowered not in {"forward", "deferred"}:
        raise argparse.ArgumentTypeError("shading mode must be 'forward' or 'deferred'")
    return lowered


def _make_options(
    args: argparse.Namespace, *, scenario_label: Optional[str], overlays: Optional[dict[str, bool]]
) -> HarnessExecutionOptions:
    return HarnessExecutionOptions(
        frames=args.frames,
        dt=args.dt,
        dry_run=args.dry_run,
        scenario_label=scenario_label,
        dataset_id=args.dataset,
        rendering_preset=args.rendering_preset,
        shading_mode=args.shading_mode,
        runtime_profile=args.runtime_profile,
        overlays=overlays,
        resolution_width=args.resolution_width,
        resolution_height=args.resolution_height,
    )


def _selected_dataset_summary(summary: HarnessConfigurationSummary) -> Optional[DatasetSummary]:
    if summary.selected_dataset is None:
        return None
    for dataset in summary.datasets:
        if dataset.identifier == summary.selected_dataset:
            return dataset
    return None


def _print_summary(summary: HarnessConfigurationSummary) -> None:
    dataset = summary.selected_dataset or "<none>"
    rendering = summary.rendering
    preset = rendering.preset if rendering else "<unspecified>"
    shading = rendering.shading_mode if rendering else "<unspecified>"
    runtime_profile = summary.selected_algorithm_variant or "<unspecified>"
    print(
        f"Configuration: dataset={dataset} preset={preset} shading={shading} runtime={runtime_profile}"
    )

    dataset_summary = _selected_dataset_summary(summary)
    if dataset_summary is None:
        print("Dataset assets: <none>")
        return

    assets = dataset_summary.assets
    total = len(assets)
    failures = sum(1 for asset in assets if not asset.verified)
    failure_label = "failure" if failures == 1 else "failures"
    print(
        f"Dataset assets ({dataset_summary.identifier}): "
        f"{total} total, {failures} {failure_label}"
    )
    for asset in assets:
        status = "ok" if asset.verified else "FAILED"
        message = f" — {asset.message}" if asset.message else ""
        print(f"  - {asset.role}: {status}{message}")
        print(f"    path={asset.resolved_path}")


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


def _list_datasets(
    summary: HarnessConfigurationSummary,
    *,
    config_label: str | None,
    config_path: Path | None,
    json_path: Path | None,
) -> int:
    header_parts: list[str] = []
    if config_label:
        header_parts.append(f"{config_label}")
    if config_path is not None:
        header_parts.append(str(config_path))
    if header_parts:
        joined = ", ".join(header_parts)
        print(f"Datasets for {joined}:")
    else:
        print("Datasets:")

    if not summary.datasets:
        print("  <none>")
    else:
        for dataset in summary.datasets:
            label = dataset.label or dataset.identifier
            tags = ", ".join(dataset.tags) if dataset.tags else "<none>"
            asset_total = len(dataset.assets)
            verified_assets = sum(1 for asset in dataset.assets if asset.verified)
            if asset_total == 0:
                verification = "no assets declared"
            elif verified_assets == asset_total:
                verification = "all assets verified"
            else:
                verification = f"{verified_assets}/{asset_total} assets verified"
            print(
                "  - "
                f"{dataset.identifier} ({label}) "
                f"kind={dataset.kind} tags=[{tags}] {verification}"
            )
            if asset_total and verified_assets != asset_total:
                for asset in dataset.assets:
                    if asset.verified:
                        continue
                    reason = asset.message or "verification failed"
                    print(f"    • {asset.role}: {reason}")

    if summary.selected_dataset is not None:
        print(f"Selected dataset: {summary.selected_dataset}")

    if json_path is not None:
        payload = configuration_summary_to_dict(summary)
        export: dict[str, object] = {
            "datasets": payload.get("datasets", []),
            "selected_dataset": payload.get("selected_dataset"),
        }
        if config_label is not None:
            export["source_label"] = config_label
        if config_path is not None:
            export["config_path"] = str(config_path)
        _write_json(json_path, export)
    return 0


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
        harness = load_harness(
            str(config_path),
            require_schema=True if args.require_schema else None,
            case_study_id=args.case_study,
        )
    except PrototypeHarnessError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if args.case_study is not None:
        label_suffix = f" ({case_label})" if case_label else ""
        print(f"Selected case study '{args.case_study}'{label_suffix} from {config_path}")

    description = harness.describe_configuration()
    _print_summary(description)

    if args.describe_json is not None:
        _write_json(args.describe_json, configuration_summary_to_dict(description))
    if args.list_benchmarks:
        _print_benchmark_scenarios(description)

    overlay_overrides = _collect_overlay_overrides(args.overlays)

    if args.dry_run:
        total_runs = args.repeat
        base_options = _make_options(
            args,
            scenario_label=args.case_study,
            overlays=overlay_overrides,
        )
        base_options = replace(base_options, frames=1)
        run_count = total_runs if total_runs > 1 else None
        for index in range(1, total_runs + 1):
            options = base_options
            if run_count is not None:
                options = replace(base_options, run_index=index, run_count=run_count)
            summary = harness.run_headless(options)
            print(f"Dry run summary{_format_run_prefix(index, total_runs)}: {summarize(summary)}")
            if args.summary_json is not None:
                _write_json(
                    _summary_path_for_run(args.summary_json, index, total_runs),
                    run_summary_to_dict(summary),
                )
        return 0

    base_options = _make_options(args, scenario_label=args.case_study, overlays=overlay_overrides)
    total_runs = args.repeat
    run_count = total_runs if total_runs > 1 else None
    for index in range(1, total_runs + 1):
        options = base_options
        if run_count is not None:
            options = replace(base_options, run_index=index, run_count=run_count)
        try:
            summary = harness.run_headless(options)
        except PrototypeHarnessError as error:
            print(f"error: {error}", file=sys.stderr)
            return 3
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
        "--dataset",
        help="Override the dataset identifier before executing the harness.",
    )
    parser.add_argument(
        "--rendering-preset",
        help="Override the rendering preset recorded in telemetry and summaries.",
    )
    parser.add_argument(
        "--shading-mode",
        type=_parse_shading_mode,
        help="Override the research rendering shading mode (forward or deferred).",
    )
    parser.add_argument(
        "--resolution-width",
        type=int,
        help="Override the rendering resolution width in pixels.",
    )
    parser.add_argument(
        "--resolution-height",
        type=int,
        help="Override the rendering resolution height in pixels.",
    )
    parser.add_argument(
        "--runtime-profile",
        help="Override the runtime profile / algorithm variant before executing the harness.",
    )
    parser.add_argument(
        "--overlay",
        dest="overlays",
        action="append",
        type=_parse_overlay_argument,
        default=[],
        metavar="KEY=BOOL",
        help="Override rendering overlay state (repeatable, e.g., --overlay normals=1).",
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
        "--list-datasets",
        action="store_true",
        help="List datasets declared in the configuration without executing the harness.",
    )
    parser.add_argument(
        "--datasets-json",
        type=Path,
        help="Write dataset metadata extracted from the configuration to the specified JSON file.",
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

    if args.datasets_json is not None and not args.list_datasets:
        parser.error("--datasets-json requires --list-datasets")

    if args.list_datasets:
        if args.summary_json is not None or args.describe_json is not None or args.list_benchmarks:
            parser.error("--list-datasets cannot be combined with execution options")
        if args.config is None and args.case_study is None:
            parser.error("--list-datasets requires --config or --case-study")
        if args.datasets_json is not None and args.datasets_json.is_dir():
            parser.error("--datasets-json must reference a file path")
        try:
            config_path, case_label = _resolve_config_argument(args)
        except CaseStudyError as error:
            print(f"error: {error}", file=sys.stderr)
            return 2
        try:
            harness = load_harness(
                str(config_path),
                require_schema=True if args.require_schema else None,
                case_study_id=args.case_study,
            )
        except PrototypeHarnessError as error:
            print(f"error: {error}", file=sys.stderr)
            return 2
        summary = harness.describe_configuration()
        return _list_datasets(
            summary,
            config_label=case_label,
            config_path=config_path,
            json_path=args.datasets_json,
        )

    if args.frames <= 0 and not args.dry_run:
        parser.error("--frames must be greater than zero unless --dry-run is specified")

    if args.repeat <= 0:
        parser.error("--repeat must be greater than zero")

    if args.config is None and args.case_study is None:
        parser.error("one of --config or --case-study is required")

    return _run(args)


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())

