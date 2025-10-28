"""Orchestrate comparative benchmarks between engine runs and reference baselines.

This module implements the first automation step for the `CC-310` roadmap item.
It parses a declarative configuration file, executes the configured commands,
collects metric outputs, and validates them against regression thresholds.

Usage
-----

    python scripts/benchmarks/run_comparative_benchmarks.py \
        --config benchmarks/example.json

The tool emits a machine-readable JSON summary along with a CSV table of
per-scenario metrics under the configured output directory. Pass ``--output``
and ``--table`` to override the default artifact paths when integrating with
external dashboards or CI uploads.

Configuration format (JSON or YAML)
-----------------------------------

```
{
  "version": 1,
  "output_directory": "artifacts",
  "scenarios": [
    {
      "name": "remesh-baseline",
      "dataset": "datasets/bunny",
      "engine": {
        "command": ["python", "run_engine.py", "{output_path}"],
        "output": "{output_dir}/{scenario}_engine.json"
      },
      "reference": {
        "command": ["python", "run_reference.py", "{output_path}"],
        "output": "{output_dir}/{scenario}_reference.json"
      },
      "metrics": [
        {
          "name": "fps",
          "higher_is_better": true,
          "threshold": {"type": "relative", "max_regression": 0.05}
        }
      ]
    }
  ]
}
```

`{output_dir}` and `{scenario}` placeholders are expanded relative to the
configuration file directory. Each command may reference `{output_path}` to
receive the concrete output location for its metrics JSON file.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Mapping, Optional, Sequence

try:  # Optional YAML support without hard dependency at runtime.
    import yaml  # type: ignore
except ModuleNotFoundError:  # pragma: no cover - exercised implicitly when PyYAML missing.
    yaml = None


class BenchmarkConfigError(RuntimeError):
    """Raised when the benchmark configuration is invalid."""


class BenchmarkExecutionError(RuntimeError):
    """Raised when execution of a benchmark scenario fails."""


@dataclass(frozen=True)
class MetricThreshold:
    """Regression threshold associated with a metric."""

    mode: str  # "relative" or "absolute"
    limit: float

    def __post_init__(self) -> None:
        mode = self.mode.lower()
        if mode not in {"relative", "absolute"}:
            raise BenchmarkConfigError(f"unsupported threshold mode: {self.mode!r}")
        if not math.isfinite(self.limit) or self.limit < 0.0:
            raise BenchmarkConfigError("threshold limit must be a non-negative finite value")
        object.__setattr__(self, "mode", mode)


@dataclass(frozen=True)
class MetricSpec:
    """Definition of a metric comparison."""

    name: str
    higher_is_better: bool
    threshold: MetricThreshold


@dataclass(frozen=True)
class CommandSpec:
    """Command executed to generate benchmark metrics."""

    tokens: Optional[Sequence[str]]
    output_path: Path

    def format_tokens(self, substitutions: Mapping[str, str]) -> Optional[List[str]]:
        if self.tokens is None:
            return None
        formatted: List[str] = []
        mapping = _StrictSubstitutions(substitutions)
        for token in self.tokens:
            formatted.append(str(token).format_map(mapping))
        return formatted


@dataclass(frozen=True)
class BenchmarkScenario:
    """Single benchmark scenario definition."""

    name: str
    dataset: Optional[str]
    engine: CommandSpec
    reference: CommandSpec
    metrics: Sequence[MetricSpec]


@dataclass(frozen=True)
class BenchmarkConfig:
    """Top-level benchmark configuration."""

    source_path: Path
    output_dir: Path
    scenarios: Sequence[BenchmarkScenario]


@dataclass(frozen=True)
class MetricResult:
    """Outcome of a single metric comparison."""

    spec: MetricSpec
    engine_value: float
    reference_value: float
    delta: float
    relative_delta: Optional[float]
    passed: bool
    regression_amount: float

    def as_dict(self) -> Mapping[str, object]:
        payload: Dict[str, object] = {
            "name": self.spec.name,
            "engine_value": self.engine_value,
            "reference_value": self.reference_value,
            "delta": self.delta,
            "passed": self.passed,
            "threshold": {
                "mode": self.spec.threshold.mode,
                "limit": self.spec.threshold.limit,
                "higher_is_better": self.spec.higher_is_better,
            },
            "regression_amount": self.regression_amount,
        }
        if self.relative_delta is not None:
            payload["relative_delta"] = self.relative_delta
        return payload


@dataclass(frozen=True)
class ScenarioResult:
    """Aggregated results for a benchmark scenario."""

    scenario: BenchmarkScenario
    metrics: Sequence[MetricResult]

    @property
    def passed(self) -> bool:
        return all(metric.passed for metric in self.metrics)

    def as_dict(self) -> Mapping[str, object]:
        return {
            "name": self.scenario.name,
            "dataset": self.scenario.dataset,
            "passed": self.passed,
            "metrics": [metric.as_dict() for metric in self.metrics],
        }


@dataclass(frozen=True)
class BenchmarkSummary:
    """Summary across all executed scenarios."""

    results: Sequence[ScenarioResult]

    @property
    def passed(self) -> bool:
        return all(result.passed for result in self.results)

    def as_dict(self) -> Mapping[str, object]:
        return {
            "passed": self.passed,
            "scenarios": [result.as_dict() for result in self.results],
        }


class _StrictSubstitutions(dict):
    """Mapping that raises for missing placeholders during string formatting."""

    def __missing__(self, key: str) -> str:  # pragma: no cover - defensive guard
        raise BenchmarkConfigError(f"missing substitution for placeholder '{key}'")


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Execute comparative benchmarks defined in a configuration file.",
    )
    parser.add_argument(
        "--config",
        type=Path,
        required=True,
        help="Path to benchmark configuration (JSON or YAML).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional path for the summary JSON output. Defaults to <output_dir>/comparative_summary.json.",
    )
    parser.add_argument(
        "--table",
        type=Path,
        help="Optional path for the summary CSV output. Defaults to <output_dir>/comparative_summary.csv.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Skip executing commands; useful when results are precomputed for testing.",
    )
    return parser.parse_args(argv)


def load_config(path: Path) -> BenchmarkConfig:
    """Parse and validate a benchmark configuration file."""

    if not path.exists():
        raise BenchmarkConfigError(f"configuration file not found: {path}")
    raw_data = _load_raw_config(path)
    if not isinstance(raw_data, Mapping):
        raise BenchmarkConfigError("configuration root must be a mapping")

    base_dir = path.parent

    output_dir = raw_data.get("output_directory")
    substitutions = {
        "config_dir": str(base_dir),
    }
    if output_dir is None:
        resolved_output = base_dir / "comparative_results"
    else:
        resolved_output = _resolve_path(str(output_dir), base_dir, substitutions)
    resolved_output = resolved_output.resolve()

    scenarios_data = raw_data.get("scenarios")
    if not isinstance(scenarios_data, Sequence) or not scenarios_data:
        raise BenchmarkConfigError("configuration must provide a non-empty 'scenarios' list")

    scenarios: List[BenchmarkScenario] = []
    for index, entry in enumerate(scenarios_data):
        if not isinstance(entry, Mapping):
            raise BenchmarkConfigError(f"scenario #{index} must be a mapping")
        scenarios.append(_parse_scenario(entry, base_dir, resolved_output))

    return BenchmarkConfig(source_path=path, output_dir=resolved_output, scenarios=scenarios)


def execute_benchmarks(config: BenchmarkConfig, *, dry_run: bool = False) -> BenchmarkSummary:
    """Execute all scenarios defined in *config* and return the aggregated summary."""

    results: List[ScenarioResult] = []
    for scenario in config.scenarios:
        results.append(_execute_scenario(config, scenario, dry_run=dry_run))
    return BenchmarkSummary(results=results)


def write_summary(summary: BenchmarkSummary, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = summary.as_dict()
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def write_summary_table(summary: BenchmarkSummary, path: Path) -> None:
    """Persist a tabular view of *summary* for downstream analysis tools."""

    path.parent.mkdir(parents=True, exist_ok=True)
    headers = [
        "scenario",
        "dataset",
        "metric",
        "higher_is_better",
        "engine_value",
        "reference_value",
        "delta",
        "relative_delta",
        "passed",
        "threshold_mode",
        "threshold_limit",
        "regression_amount",
    ]

    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(headers)
        for result in summary.results:
            dataset = result.scenario.dataset or ""
            for metric in result.metrics:
                if metric.relative_delta is None:
                    relative_delta = ""
                else:
                    relative_delta = f"{metric.relative_delta:.10f}"
                writer.writerow(
                    [
                        result.scenario.name,
                        dataset,
                        metric.spec.name,
                        str(metric.spec.higher_is_better),
                        f"{metric.engine_value:.10f}",
                        f"{metric.reference_value:.10f}",
                        f"{metric.delta:.10f}",
                        relative_delta,
                        str(metric.passed),
                        metric.spec.threshold.mode,
                        f"{metric.spec.threshold.limit:.10f}",
                        f"{metric.regression_amount:.10f}",
                    ]
                )


def format_summary_text(summary: BenchmarkSummary) -> str:
    lines: List[str] = []
    for result in summary.results:
        status = "PASS" if result.passed else "FAIL"
        header = f"[{status}] {result.scenario.name}"
        if result.scenario.dataset:
            header += f" (dataset: {result.scenario.dataset})"
        lines.append(header)
        for metric in result.metrics:
            rel = metric.relative_delta
            if rel is not None:
                rel_percent = f"{rel * 100.0:+.2f}%"
            else:
                rel_percent = "n/a"
            direction = "↑" if metric.spec.higher_is_better else "↓"
            lines.append(
                "    - {name}: engine={engine:.4f}, reference={reference:.4f}, "
                "delta={delta:+.4f} ({rel}), threshold={mode}≤{limit:.4f} {direction}"
                .format(
                    name=metric.spec.name,
                    engine=metric.engine_value,
                    reference=metric.reference_value,
                    delta=metric.delta,
                    rel=rel_percent,
                    mode=metric.spec.threshold.mode,
                    limit=metric.spec.threshold.limit,
                    direction=direction,
                )
            )
            if not metric.passed:
                lines.append(
                    f"      Regression {metric.regression_amount:.4f} exceeds allowable limit"
                )
    if not lines:
        return "No scenarios executed."
    return "\n".join(lines)


def _load_raw_config(path: Path) -> Mapping[str, object]:
    text = path.read_text(encoding="utf-8")
    suffix = path.suffix.lower()
    if suffix in {".json", ""}:
        return json.loads(text)
    if suffix in {".yml", ".yaml"}:
        if yaml is None:
            raise BenchmarkConfigError(
                "PyYAML is not installed; install it or provide a JSON configuration."
            )
        data = yaml.safe_load(text)
        if not isinstance(data, Mapping):
            raise BenchmarkConfigError("YAML configuration must evaluate to a mapping")
        return data
    raise BenchmarkConfigError(f"unsupported configuration format: {suffix}")


def _parse_scenario(
    data: Mapping[str, object],
    base_dir: Path,
    output_dir: Path,
) -> BenchmarkScenario:
    name = data.get("name")
    if not isinstance(name, str) or not name:
        raise BenchmarkConfigError("scenario missing required 'name'")
    dataset = data.get("dataset")
    if dataset is not None and not isinstance(dataset, str):
        raise BenchmarkConfigError(f"scenario '{name}' has non-string dataset entry")

    context = {
        "output_dir": str(output_dir),
        "scenario": name,
        "config_dir": str(base_dir),
    }

    engine = _parse_command_spec(data.get("engine"), base_dir, context, role="engine")
    reference = _parse_command_spec(data.get("reference"), base_dir, context, role="reference")

    metrics_data = data.get("metrics")
    if not isinstance(metrics_data, Sequence) or not metrics_data:
        raise BenchmarkConfigError(f"scenario '{name}' must declare one or more metrics")
    metrics: List[MetricSpec] = []
    for entry in metrics_data:
        if not isinstance(entry, Mapping):
            raise BenchmarkConfigError(f"scenario '{name}' has invalid metric entry")
        metrics.append(_parse_metric_spec(entry, name))

    return BenchmarkScenario(name=name, dataset=dataset, engine=engine, reference=reference, metrics=metrics)


def _parse_command_spec(
    entry: object,
    base_dir: Path,
    context: Mapping[str, str],
    *,
    role: str,
) -> CommandSpec:
    if not isinstance(entry, Mapping):
        raise BenchmarkConfigError(f"scenario '{context['scenario']}' missing '{role}' definition")

    raw_output = entry.get("output")
    if not isinstance(raw_output, str) or not raw_output:
        raise BenchmarkConfigError(
            f"scenario '{context['scenario']}' {role} definition requires an 'output' path"
        )
    output_path = _resolve_path(raw_output, base_dir, context)

    raw_command = entry.get("command")
    if raw_command is None:
        tokens: Optional[Sequence[str]] = None
    else:
        if not isinstance(raw_command, Sequence) or not raw_command:
            raise BenchmarkConfigError(
                f"scenario '{context['scenario']}' {role} command must be a non-empty list"
            )
        tokens = [str(token) for token in raw_command]

    return CommandSpec(tokens=tokens, output_path=output_path)


def _parse_metric_spec(entry: Mapping[str, object], scenario_name: str) -> MetricSpec:
    name = entry.get("name")
    if not isinstance(name, str) or not name:
        raise BenchmarkConfigError(f"scenario '{scenario_name}' metric missing 'name'")
    higher = entry.get("higher_is_better")
    if higher is None:
        raise BenchmarkConfigError(
            f"scenario '{scenario_name}' metric '{name}' missing 'higher_is_better' flag"
        )
    higher_bool = bool(higher)

    threshold_entry = entry.get("threshold")
    if not isinstance(threshold_entry, Mapping):
        raise BenchmarkConfigError(
            f"scenario '{scenario_name}' metric '{name}' missing 'threshold' mapping"
        )
    mode = threshold_entry.get("type")
    if not isinstance(mode, str):
        raise BenchmarkConfigError(
            f"scenario '{scenario_name}' metric '{name}' threshold missing 'type'"
        )
    mode_lower = mode.lower()
    if mode_lower == "relative":
        limit_value = threshold_entry.get("max_regression")
    elif mode_lower == "absolute":
        limit_value = threshold_entry.get("max_delta")
    else:
        raise BenchmarkConfigError(
            f"scenario '{scenario_name}' metric '{name}' has unsupported threshold type '{mode}'"
        )
    try:
        limit = float(limit_value)
    except (TypeError, ValueError) as exc:  # pragma: no cover - defensive guard
        raise BenchmarkConfigError(
            f"scenario '{scenario_name}' metric '{name}' threshold limit is invalid"
        ) from exc

    threshold = MetricThreshold(mode=mode_lower, limit=limit)
    return MetricSpec(name=name, higher_is_better=higher_bool, threshold=threshold)


def _execute_scenario(
    config: BenchmarkConfig,
    scenario: BenchmarkScenario,
    *,
    dry_run: bool,
) -> ScenarioResult:
    context = {
        "output_dir": str(config.output_dir),
        "scenario": scenario.name,
        "config_dir": str(config.source_path.parent),
    }

    engine_metrics = _execute_role(
        command=scenario.engine,
        context=context,
        role="engine",
        dry_run=dry_run,
        working_directory=config.source_path.parent,
    )
    reference_metrics = _execute_role(
        command=scenario.reference,
        context=context,
        role="reference",
        dry_run=dry_run,
        working_directory=config.source_path.parent,
    )

    metric_results: List[MetricResult] = []
    for spec in scenario.metrics:
        if spec.name not in engine_metrics:
            raise BenchmarkExecutionError(
                f"scenario '{scenario.name}' engine results missing metric '{spec.name}'"
            )
        if spec.name not in reference_metrics:
            raise BenchmarkExecutionError(
                f"scenario '{scenario.name}' reference results missing metric '{spec.name}'"
            )
        metric_results.append(
            _evaluate_metric(spec, engine_metrics[spec.name], reference_metrics[spec.name])
        )

    return ScenarioResult(scenario=scenario, metrics=metric_results)


def _execute_role(
    command: CommandSpec,
    context: Mapping[str, str],
    *,
    role: str,
    dry_run: bool,
    working_directory: Path,
) -> Mapping[str, float]:
    output_path = command.output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)

    substitutions = dict(context)
    substitutions.update(
        {
            "output_path": str(output_path),
            "role": role,
        }
    )

    formatted_command = command.format_tokens(substitutions)
    if formatted_command is not None and not dry_run:
        try:
            subprocess.run(
                formatted_command,
                cwd=working_directory,
                check=True,
            )
        except subprocess.CalledProcessError as exc:  # pragma: no cover - exercised via tests
            raise BenchmarkExecutionError(
                f"{role} command failed with exit code {exc.returncode}: {formatted_command}"
            ) from exc

    if not output_path.exists():
        raise BenchmarkExecutionError(
            f"{role} output missing at {output_path}. Command may have failed or skipped."
        )

    return _load_metrics(output_path)


def _load_metrics(path: Path) -> Mapping[str, float]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise BenchmarkExecutionError(f"failed to parse metrics JSON at {path}: {exc}") from exc
    if not isinstance(payload, Mapping):
        raise BenchmarkExecutionError(f"metrics file at {path} must contain a JSON object")
    metrics = payload.get("metrics")
    if not isinstance(metrics, Mapping) or not metrics:
        raise BenchmarkExecutionError(f"metrics payload at {path} missing 'metrics' mapping")
    resolved: Dict[str, float] = {}
    for key, value in metrics.items():
        try:
            resolved[str(key)] = float(value)
        except (TypeError, ValueError) as exc:
            raise BenchmarkExecutionError(
                f"metric '{key}' in {path} is not a numeric value"
            ) from exc
    return resolved


def _evaluate_metric(spec: MetricSpec, engine_value: float, reference_value: float) -> MetricResult:
    delta = engine_value - reference_value
    relative_delta: Optional[float]
    regression_amount: float

    baseline = abs(reference_value) if abs(reference_value) > 1e-8 else None

    if spec.higher_is_better:
        regression_amount = max(0.0, reference_value - engine_value)
        if spec.threshold.mode == "relative" and baseline is not None:
            relative_delta = delta / reference_value if reference_value != 0.0 else None
            allowed = spec.threshold.limit * abs(reference_value)
            passed = regression_amount <= allowed + 1e-8
        elif spec.threshold.mode == "relative":
            relative_delta = None
            passed = regression_amount <= spec.threshold.limit + 1e-8
        else:
            relative_delta = delta / reference_value if reference_value != 0.0 else None
            passed = regression_amount <= spec.threshold.limit + 1e-8
    else:
        regression_amount = max(0.0, engine_value - reference_value)
        if spec.threshold.mode == "relative" and baseline is not None:
            relative_delta = delta / reference_value if reference_value != 0.0 else None
            allowed = spec.threshold.limit * abs(reference_value)
            passed = regression_amount <= allowed + 1e-8
        elif spec.threshold.mode == "relative":
            relative_delta = None
            passed = regression_amount <= spec.threshold.limit + 1e-8
        else:
            relative_delta = delta / reference_value if reference_value != 0.0 else None
            passed = regression_amount <= spec.threshold.limit + 1e-8

    return MetricResult(
        spec=spec,
        engine_value=engine_value,
        reference_value=reference_value,
        delta=delta,
        relative_delta=relative_delta,
        passed=passed,
        regression_amount=regression_amount,
    )


def _resolve_path(template: str, base_dir: Path, substitutions: Mapping[str, str]) -> Path:
    formatted = template.format_map(_StrictSubstitutions(substitutions))
    candidate = Path(formatted)
    if not candidate.is_absolute():
        candidate = (base_dir / candidate).resolve()
    return candidate


def main(argv: Optional[Sequence[str]] = None) -> int:
    try:
        args = parse_args(argv)
        config = load_config(args.config)
        summary = execute_benchmarks(config, dry_run=args.dry_run)
        summary_path = args.output or config.output_dir / "comparative_summary.json"
        write_summary(summary, summary_path)
        table_path = args.table or config.output_dir / "comparative_summary.csv"
        write_summary_table(summary, table_path)
        print(format_summary_text(summary))
        return 0 if summary.passed else 1
    except BenchmarkConfigError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    except BenchmarkExecutionError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 3


if __name__ == "__main__":  # pragma: no cover - exercised in integration environments
    sys.exit(main())

