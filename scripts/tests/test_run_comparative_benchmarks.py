from __future__ import annotations

import json
import sys
from pathlib import Path

import csv
import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts.benchmarks import run_comparative_benchmarks


def _write_config(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")


def _create_metric_emitter(path: Path) -> Path:
    script = (
        "import json, pathlib, sys\n"
        "target = pathlib.Path(sys.argv[1])\n"
        "value = float(sys.argv[2])\n"
        "target.parent.mkdir(parents=True, exist_ok=True)\n"
        "target.write_text(json.dumps({'metrics': {'fps': value}}), encoding='utf-8')\n"
    )
    path.write_text(script, encoding="utf-8")
    return path


def test_load_config_json(tmp_path: Path) -> None:
    config_path = tmp_path / "config.json"
    config_data = {
        "version": 1,
        "output_directory": "outputs",
        "scenarios": [
            {
                "name": "example",
                "dataset": "datasets/demo",
                "engine": {
                    "command": [sys.executable, "engine_runner.py", "{output_path}"],
                    "output": "{output_dir}/{scenario}_engine.json",
                },
                "reference": {
                    "command": [sys.executable, "reference_runner.py", "{output_path}"],
                    "output": "{output_dir}/{scenario}_reference.json",
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.1},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config_data)

    config = run_comparative_benchmarks.load_config(config_path)

    assert config.output_dir == (tmp_path / "outputs").resolve()
    assert len(config.scenarios) == 1
    scenario = config.scenarios[0]
    assert scenario.name == "example"
    assert scenario.dataset == "datasets/demo"
    assert scenario.engine.output_path.name.endswith("_engine.json")
    assert scenario.reference.output_path.name.endswith("_reference.json")
    assert scenario.metrics[0].threshold.mode == "relative"


def test_execute_benchmarks_success(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    emitter = _create_metric_emitter(tmp_path / "emit_metrics.py")
    config_path = tmp_path / "config.json"
    config = {
        "output_directory": "outputs",
        "scenarios": [
            {
                "name": "success",
                "engine": {
                    "command": [sys.executable, str(emitter), "{output_path}", "120.0"],
                    "output": "{output_dir}/{scenario}_engine.json",
                },
                "reference": {
                    "command": [sys.executable, str(emitter), "{output_path}", "100.0"],
                    "output": "{output_dir}/{scenario}_reference.json",
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.2},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config)

    exit_code = run_comparative_benchmarks.main(["--config", str(config_path)])
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "[PASS] success" in captured.out

    summary_path = (tmp_path / "outputs" / "comparative_summary.json").resolve()
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    assert summary["passed"] is True
    scenario_summary = summary["scenarios"][0]
    assert scenario_summary["passed"] is True
    metric_summary = scenario_summary["metrics"][0]
    assert metric_summary["name"] == "fps"
    assert metric_summary["passed"] is True
    assert "plot" in metric_summary
    plot_path = Path(metric_summary["plot"])
    if not plot_path.is_absolute():
        plot_path = summary_path.parent / plot_path
    assert plot_path.exists()

    table_path = (tmp_path / "outputs" / "comparative_summary.csv").resolve()
    with table_path.open(encoding="utf-8") as handle:
        rows = list(csv.reader(handle))
    assert rows[0] == [
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
    assert rows[1][0] == "success"
    assert rows[1][2] == "fps"
    assert rows[1][3] == "True"
    assert rows[1][8] == "True"


def test_execute_benchmarks_regression_failure(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    emitter = _create_metric_emitter(tmp_path / "emit_metrics.py")
    config_path = tmp_path / "config.json"
    config = {
        "output_directory": "outputs",
        "scenarios": [
            {
                "name": "regression",
                "engine": {
                    "command": [sys.executable, str(emitter), "{output_path}", "90.0"],
                    "output": "{output_dir}/{scenario}_engine.json",
                },
                "reference": {
                    "command": [sys.executable, str(emitter), "{output_path}", "100.0"],
                    "output": "{output_dir}/{scenario}_reference.json",
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.05},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config)

    exit_code = run_comparative_benchmarks.main(["--config", str(config_path)])
    captured = capsys.readouterr()

    assert exit_code == 1
    assert "[FAIL] regression" in captured.out
    assert "Regression" in captured.out

    summary_path = (tmp_path / "outputs" / "comparative_summary.json").resolve()
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    assert summary["passed"] is False
    assert summary["scenarios"][0]["passed"] is False
    assert summary["scenarios"][0]["metrics"][0]["passed"] is False
    assert "plot" in summary["scenarios"][0]["metrics"][0]


def test_dry_run_uses_existing_outputs(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    output_dir = tmp_path / "outputs"
    output_dir.mkdir()
    engine_output = output_dir / "dry_engine.json"
    reference_output = output_dir / "dry_reference.json"
    engine_output.write_text(json.dumps({"metrics": {"fps": 110.0}}), encoding="utf-8")
    reference_output.write_text(json.dumps({"metrics": {"fps": 100.0}}), encoding="utf-8")

    config_path = tmp_path / "config.json"
    config = {
        "output_directory": str(output_dir),
        "scenarios": [
            {
                "name": "dry",
                "engine": {
                    "command": ["python", "-c", "raise SystemExit('should not run')"],
                    "output": str(engine_output),
                },
                "reference": {
                    "command": ["python", "-c", "raise SystemExit('should not run')"],
                    "output": str(reference_output),
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.5},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config)

    exit_code = run_comparative_benchmarks.main([
        "--config",
        str(config_path),
        "--dry-run",
    ])
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "[PASS] dry" in captured.out

    default_table = (output_dir / "comparative_summary.csv").resolve()
    assert default_table.exists()
    plot_dir = output_dir / "plots"
    assert any(plot_dir.glob("*.svg"))


def test_custom_table_path(tmp_path: Path) -> None:
    emitter = _create_metric_emitter(tmp_path / "emit_metrics.py")
    config_path = tmp_path / "config.json"
    config = {
        "output_directory": "outputs",
        "scenarios": [
            {
                "name": "custom",
                "engine": {
                    "command": [sys.executable, str(emitter), "{output_path}", "105.0"],
                    "output": "{output_dir}/{scenario}_engine.json",
                },
                "reference": {
                    "command": [sys.executable, str(emitter), "{output_path}", "100.0"],
                    "output": "{output_dir}/{scenario}_reference.json",
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.2},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config)

    table_path = tmp_path / "table" / "summary.csv"
    exit_code = run_comparative_benchmarks.main(
        ["--config", str(config_path), "--table", str(table_path)]
    )

    assert exit_code == 0
    assert table_path.exists()
    with table_path.open(encoding="utf-8") as handle:
        rows = list(csv.reader(handle))
    assert rows[1][0] == "custom"


def test_no_plots_flag(tmp_path: Path) -> None:
    emitter = _create_metric_emitter(tmp_path / "emit_metrics.py")
    config_path = tmp_path / "config.json"
    config = {
        "output_directory": "outputs",
        "scenarios": [
            {
                "name": "noplots",
                "engine": {
                    "command": [sys.executable, str(emitter), "{output_path}", "110.0"],
                    "output": "{output_dir}/{scenario}_engine.json",
                },
                "reference": {
                    "command": [sys.executable, str(emitter), "{output_path}", "100.0"],
                    "output": "{output_dir}/{scenario}_reference.json",
                },
                "metrics": [
                    {
                        "name": "fps",
                        "higher_is_better": True,
                        "threshold": {"type": "relative", "max_regression": 0.2},
                    }
                ],
            }
        ],
    }
    _write_config(config_path, config)

    exit_code = run_comparative_benchmarks.main(
        ["--config", str(config_path), "--no-plots"]
    )

    assert exit_code == 0
    summary_path = (tmp_path / "outputs" / "comparative_summary.json").resolve()
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    metric_summary = summary["scenarios"][0]["metrics"][0]
    assert "plot" not in metric_summary
    assert not (summary_path.parent / "plots").exists()

