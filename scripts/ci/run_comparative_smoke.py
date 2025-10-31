"""Execute the CC-310 comparative benchmark smoke suite.

This helper seeds the comparative benchmark orchestrator with precomputed
metrics so CI can validate the configuration pipeline without executing the
runtime. The smoke suite focuses on schema integrity, summary generation, and
plot creation — it should complete in well under a second.

Usage::

    python scripts/ci/run_comparative_smoke.py \
        --workspace-root /path/to/repo

The command exits with status ``0`` when all scenarios pass the configured
thresholds and ``1`` when regressions are detected. Configuration and metric
fixtures live under ``assets/benchmarks/ai004``.

Pass ``--output-dir`` to regenerate artefacts in-place (for example,
``--output-dir assets/benchmarks/ai004/reports`` when refreshing published
reports).
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
import tempfile
from pathlib import Path
from typing import Optional, Sequence

REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from scripts.benchmarks import run_comparative_benchmarks as orchestrator


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the comparative benchmark smoke configuration using precomputed metrics.",
    )
    default_root = REPO_ROOT
    parser.add_argument(
        "--workspace-root",
        type=Path,
        default=default_root,
        help="Repository root containing assets/benchmarks/ai004/comparative_config.json.",
    )
    parser.add_argument(
        "--config-template",
        type=Path,
        help="Optional path to the comparative benchmark configuration template.",
    )
    parser.add_argument(
        "--samples-root",
        type=Path,
        help="Directory containing per-scenario metric fixtures (defaults to assets/benchmarks/ai004/data).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Optional directory to write artefacts instead of a temporary workspace.",
    )
    return parser.parse_args(argv)


def _resolve_paths(args: argparse.Namespace) -> tuple[Path, Path, Path]:
    workspace_root = args.workspace_root.resolve()
    config_template = (
        args.config_template
        if args.config_template is not None
        else workspace_root / "assets" / "benchmarks" / "ai004" / "comparative_config.json"
    ).resolve()
    samples_root = (
        args.samples_root
        if args.samples_root is not None
        else workspace_root / "assets" / "benchmarks" / "ai004" / "data"
    ).resolve()
    if not config_template.is_file():
        raise SystemExit(f"error: configuration template not found: {config_template}")
    if not samples_root.is_dir():
        raise SystemExit(f"error: samples directory not found: {samples_root}")
    return workspace_root, config_template, samples_root


def _prepare_outputs(config: orchestrator.BenchmarkConfig, samples_root: Path) -> None:
    for scenario in config.scenarios:
        scenario_dir = samples_root / scenario.name
        engine_sample = scenario_dir / "engine_metrics.json"
        reference_sample = scenario_dir / "reference_metrics.json"
        if not engine_sample.is_file() or not reference_sample.is_file():
            raise SystemExit(
                f"error: missing metric fixtures for scenario '{scenario.name}' under {scenario_dir}"
            )
        scenario.engine.output_path.parent.mkdir(parents=True, exist_ok=True)
        scenario.reference.output_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(engine_sample, scenario.engine.output_path)
        shutil.copyfile(reference_sample, scenario.reference.output_path)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    _, config_template, samples_root = _resolve_paths(args)

    with tempfile.TemporaryDirectory(prefix="comparative_smoke_") as temp_dir_str:
        temp_dir = Path(temp_dir_str)
        config_data = json.loads(config_template.read_text(encoding="utf-8"))
        if args.output_dir is None:
            output_dir = temp_dir / "outputs"
        else:
            output_dir = args.output_dir.resolve()
            output_dir.mkdir(parents=True, exist_ok=True)
        config_data["output_directory"] = str(output_dir)
        temp_config_path = temp_dir / "config.json"
        temp_config_path.write_text(json.dumps(config_data, indent=2), encoding="utf-8")

        config = orchestrator.load_config(temp_config_path)
        _prepare_outputs(config, samples_root)
        summary = orchestrator.execute_benchmarks(config, dry_run=True)
        summary = orchestrator.attach_plots(summary, output_dir / "plots")
        summary_path = output_dir / "comparative_summary.json"
        orchestrator.write_summary(summary, summary_path)
        orchestrator.write_summary_table(summary, output_dir / "comparative_summary.csv")
        print(orchestrator.format_summary_text(summary))
        return 0 if summary.passed else 1


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    sys.exit(main())
