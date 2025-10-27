from __future__ import annotations

from importlib import import_module
import json
from pathlib import Path
import sys
from typing import List

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

_PYTHON_ROOT = _PROJECT_ROOT / "python"
if str(_PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(_PYTHON_ROOT))

validate_module = import_module("scripts.validate_ai004_config")


def _write_dataset(path: Path) -> None:
    entry = {
        "id": "remesh-sample",
        "schema": {"id": "ai-004.dataset", "version": 1},
        "kind": "geometry.remesh",
        "tags": ["geometry"],
        "source": {"generator": "geometry_remesh", "mesh": "input.obj"},
        "outputs": {"mesh": "output.obj"},
        "remeshing": {"mode": "uniform"},
        "feature_preservation": {
            "lock_boundary_edges": True,
            "lock_feature_edges": True,
            "minimum_feature_angle_degrees": 30.0,
        },
        "metrics": {
            "input": {
                "vertices": 8,
                "faces": 12,
                "edge_length": {"min": 0.4, "max": 1.2, "mean": 0.8},
            },
            "output": {
                "vertices": 16,
                "faces": 24,
                "edge_length": {"min": 0.2, "max": 1.0, "mean": 0.6},
            },
        },
        "statistics": {
            "iterations": 1,
            "max_error": 0.1,
            "min_edge_length": 0.1,
            "max_edge_length": 1.0,
            "max_surface_deviation": 0.1,
            "mean_surface_deviation": 0.05,
            "rms_surface_deviation": 0.07,
        },
    }
    payload = {"datasets": [entry]}
    path.write_text(json.dumps(payload), encoding="utf-8")


def _write_configuration(
    path: Path,
    dataset_id: str = "remesh-sample",
    *,
    rendering_schema: str = "ai-004.rendering",
) -> None:
    config = {
        "datasets": [],
        "rendering": {
            "schema": {"id": rendering_schema, "version": 1},
            "preset": "research-baseline",
        },
        "runtime": {
            "schema": {"id": "ai-004.runtime", "version": 1},
            "dataset": dataset_id,
        },
        "benchmarks": {
            "schema": {"id": "ai-004.benchmarks", "version": 1},
            "scenarios": [
                {
                    "id": "case",
                    "name": "Case",
                    "engine": {"output": "engine.json"},
                    "reference": {"output": "ref.json"},
                    "metrics": [
                        {
                            "name": "fps",
                            "higher_is_better": True,
                            "threshold": {"type": "relative", "max_regression": 0.05},
                        }
                    ],
                }
            ],
        },
    }
    path.write_text(json.dumps(config), encoding="utf-8")


def _run(argv: List[str]) -> int:
    return validate_module.main(argv)


def test_successful_validation(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    dataset = tmp_path / "dataset.json"
    config = tmp_path / "config.json"
    _write_dataset(dataset)
    _write_configuration(config)

    exit_code = _run(["--dataset", str(dataset), "--config", str(config)])

    captured = capsys.readouterr()
    assert exit_code == 0
    assert "Validated 2 files successfully." in captured.out
    assert captured.err == ""


def test_reports_validation_error(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    dataset = tmp_path / "dataset.json"
    invalid_config = tmp_path / "config.json"
    _write_dataset(dataset)
    _write_configuration(invalid_config, rendering_schema="ai-004.invalid")

    exit_code = _run(["--dataset", str(dataset), "--config", str(invalid_config)])

    captured = capsys.readouterr()
    assert exit_code == 1
    assert "rendering.schema.id" in captured.err
    assert captured.out == ""


def test_missing_arguments_exits_with_error() -> None:
    with pytest.raises(SystemExit):
        _run([])


def test_missing_file_reports_error(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    dataset = tmp_path / "dataset.json"
    _write_dataset(dataset)

    exit_code = _run(["--dataset", str(dataset), "--config", str(tmp_path / "missing.json")])

    captured = capsys.readouterr()
    assert exit_code == 1
    assert "does not exist" in captured.err
