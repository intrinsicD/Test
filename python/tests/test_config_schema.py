from __future__ import annotations

import json
import sys
from copy import deepcopy
from pathlib import Path

import pytest

# Ensure the package is importable when tests run from the repository root.
_TESTS_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _TESTS_DIR.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from engine3g import (
    Ai004Configuration,
    ConfigurationSchemaError,
    DatasetManifest,
    RenderingConfig,
    RuntimeConfig,
    TelemetryConfig,
    load_configuration,
    load_dataset_manifest,
)

try:  # pragma: no cover - optional dependency for YAML tests
    import yaml  # type: ignore
except ModuleNotFoundError:  # pragma: no cover
    yaml = None  # type: ignore


BASE_ENTRY = {
    "id": "remesh-sample",
    "schema": {"id": "ai-004.dataset", "version": 1},
    "kind": "geometry.remesh",
    "tags": ["geometry", "remesh"],
    "source": {"generator": "geometry_remesh", "mesh": "input.obj"},
    "outputs": {"mesh": "output.obj"},
    "remeshing": {
        "mode": "uniform",
        "targets": {
            "target_edge_length": 0.25,
            "relative_edge_scale": 0.75,
            "max_normal_deviation_degrees": 12.5,
            "max_surface_deviation": 0.02,
        },
    },
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
            "faces": 28,
            "edge_length": {"min": 0.2, "max": 1.0, "mean": 0.5},
        },
    },
    "parameterization": {
        "mode": "generate_lscm",
        "target_texel_density": 256.0,
        "texel_density": 250.0,
        "chart_count": 1,
        "average_stretch": 1.02,
        "max_stretch": 1.1,
        "fill_ratio": 0.85,
        "total_seam_length": 4.0,
        "atlas_area": 1.0,
        "total_chart_area": 0.92,
        "charts": [
            {
                "index": 0,
                "min_uv": [0.0, 0.0],
                "max_uv": [1.0, 1.0],
                "translation": [0.0, 0.0],
                "scale": 1.0,
                "area": 1.0,
                "boundary_length": 4.0,
            }
        ],
    },
    "statistics": {
        "iterations": 8,
        "max_error": 0.0025,
        "min_edge_length": 0.2,
        "max_edge_length": 1.0,
        "max_surface_deviation": 0.015,
        "mean_surface_deviation": 0.010,
        "rms_surface_deviation": 0.012,
    },
    "job_label": "Remesh Sample",
}


@pytest.mark.skipif(yaml is None, reason="PyYAML is required for YAML parsing")
def test_load_dataset_manifest_from_yaml(tmp_path: Path) -> None:
    entry = deepcopy(BASE_ENTRY)
    manifest_text = yaml.dump({"datasets": [entry]}, sort_keys=False)  # type: ignore[arg-type]
    path = tmp_path / "manifest.yaml"
    path.write_text(manifest_text, encoding="utf-8")

    manifest = load_dataset_manifest(path)
    assert isinstance(manifest, DatasetManifest)
    assert len(manifest.datasets) == 1
    dataset = manifest.datasets[0]
    assert dataset.identifier == "remesh-sample"
    assert dataset.schema_version == 1
    assert dataset.parameterization is not None
    assert dataset.parameterization.chart_count == 1
    assert dataset.remeshing_targets is not None
    assert pytest.approx(dataset.remeshing_targets.target_edge_length, rel=1e-6) == 0.25
    assert pytest.approx(dataset.statistics.max_surface_deviation, rel=1e-6) == 0.015
    assert pytest.approx(dataset.statistics.mean_surface_deviation, rel=1e-6) == 0.010
    assert pytest.approx(dataset.statistics.rms_surface_deviation, rel=1e-6) == 0.012


def test_load_dataset_manifest_from_json_without_parameterization(tmp_path: Path) -> None:
    entry = deepcopy(BASE_ENTRY)
    entry.pop("parameterization")
    path = tmp_path / "manifest.json"
    path.write_text(json.dumps({"datasets": [entry]}), encoding="utf-8")

    manifest = load_dataset_manifest(path)
    dataset = manifest.datasets[0]
    assert dataset.parameterization is None
    assert dataset.feature_preservation.lock_feature_edges is True
    assert dataset.output_metrics.edge_length.mean == pytest.approx(0.5)


def test_dataset_manifest_schema_flag(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    entry = deepcopy(BASE_ENTRY)
    entry.pop("schema")
    path = tmp_path / "legacy_manifest.json"
    path.write_text(json.dumps({"datasets": [entry]}), encoding="utf-8")

    monkeypatch.delenv("ENGINE_AI004_SCHEMA_V1", raising=False)
    manifest = load_dataset_manifest(path)
    dataset = manifest.datasets[0]
    assert dataset.schema_id == "ai-004.dataset"
    assert dataset.schema_version == 1

    monkeypatch.setenv("ENGINE_AI004_SCHEMA_V1", "1")
    with pytest.raises(ConfigurationSchemaError):
        load_dataset_manifest(path)


def test_invalid_dataset_identifier_raises(tmp_path: Path) -> None:
    entry = deepcopy(BASE_ENTRY)
    entry["id"] = "Invalid Name"
    path = tmp_path / "invalid.json"
    path.write_text(json.dumps({"datasets": [entry]}), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError) as exc:
        load_dataset_manifest(path)
    assert "datasets[0].id" in str(exc.value)


def test_load_full_configuration(tmp_path: Path) -> None:
    dataset_entry = deepcopy(BASE_ENTRY)
    config = {
        "datasets": [dataset_entry],
        "rendering": {
            "schema": {"id": "ai-004.rendering", "version": 1},
            "preset": "research-baseline",
            "options": {
                "shading_mode": "forward",
                "resolution": {"width": 1280, "height": 720},
                "overlays": {"normals": True, "material": True},
            },
        },
        "runtime": {
            "schema": {"id": "ai-004.runtime", "version": 1},
            "dataset": "remesh-sample",
            "scene": {"manifest": "scenes/remesh.scene", "entry_point": "main"},
            "camera": {
                "mode": "orbit",
                "position": [0.0, 1.0, 2.0],
                "target": [0.0, 0.5, 0.0],
            },
            "simulation": {"timestep_seconds": 1.0 / 60.0, "max_substeps": 4},
            "hot_reload": {"enabled": True, "watch_interval_seconds": 0.5},
        },
        "benchmarks": {
            "schema": {"id": "ai-004.benchmarks", "version": 1},
            "scenarios": [
                {
                    "id": "remesh-baseline",
                    "name": "Remesh Baseline",
                    "dataset": "remesh-sample",
                    "rendering_preset": "research-baseline",
                    "runtime_profile": "default",
                    "engine": {
                        "command": ["python", "engine.py", "{output_path}"],
                        "output": "{output_dir}/{scenario}_engine.json",
                    },
                    "reference": {
                        "command": ["python", "reference.py", "{output_path}"],
                        "output": "{output_dir}/{scenario}_reference.json",
                    },
                    "metrics": [
                        {
                            "name": "fps",
                            "higher_is_better": True,
                            "threshold": {"type": "relative", "max_regression": 0.05},
                        },
                        {
                            "name": "error",
                            "higher_is_better": False,
                            "threshold": {"type": "absolute", "max_delta": 0.1},
                        },
                    ],
                }
            ],
        },
        "telemetry": {
            "schema": {"id": "ai-004.telemetry", "version": 1},
            "outputs": [{"type": "file", "path": "telemetry/{scenario}.json"}],
            "metrics": [{"name": "frame_time", "statistic": "mean"}],
            "sampling": {"frame_interval": 2, "include_debug_overlays": True},
        },
    }

    path = tmp_path / "configuration.json"
    path.write_text(json.dumps(config), encoding="utf-8")

    configuration = load_configuration(path)
    assert isinstance(configuration, Ai004Configuration)
    assert isinstance(configuration.rendering, RenderingConfig)
    assert configuration.rendering.shading_mode == "forward"
    assert configuration.rendering.width == 1280
    assert isinstance(configuration.runtime, RuntimeConfig)
    assert configuration.runtime.dataset == "remesh-sample"
    assert configuration.runtime.hot_reload.enabled is True
    assert configuration.runtime.hot_reload.watch_interval_seconds == pytest.approx(0.5)
    assert configuration.benchmarks is not None
    scenario = configuration.benchmarks.scenarios[0]
    assert scenario.metrics[0].threshold.mode == "relative"
    assert configuration.telemetry is not None
    assert isinstance(configuration.telemetry, TelemetryConfig)
    assert configuration.telemetry.outputs[0].path == "telemetry/{scenario}.json"
    assert configuration.telemetry.sampling is not None
    assert configuration.telemetry.sampling.frame_interval == 2


def test_configuration_schema_flag(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    dataset_entry = deepcopy(BASE_ENTRY)
    dataset_entry.pop("schema")
    config = {
        "datasets": [dataset_entry],
        "rendering": {"preset": "research-baseline"},
        "runtime": {"dataset": "remesh-sample"},
    }
    path = tmp_path / "legacy_config.json"
    path.write_text(json.dumps(config), encoding="utf-8")

    monkeypatch.delenv("ENGINE_AI004_SCHEMA_V1", raising=False)
    configuration = load_configuration(path)
    assert configuration.rendering is not None
    assert configuration.rendering.schema_version == 1

    monkeypatch.setenv("ENGINE_AI004_SCHEMA_V1", "true")
    with pytest.raises(ConfigurationSchemaError):
        load_configuration(path)


def test_invalid_rendering_schema_id(tmp_path: Path) -> None:
    config = {
        "rendering": {
            "schema": {"id": "ai-004.invalid", "version": 1},
            "preset": "research-baseline",
        }
    }
    path = tmp_path / "invalid_rendering.json"
    path.write_text(json.dumps(config), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError) as exc:
        load_configuration(path)
    assert "rendering.schema.id" in str(exc.value)


def test_invalid_benchmark_threshold_type(tmp_path: Path) -> None:
    config = {
        "benchmarks": {
            "schema": {"id": "ai-004.benchmarks", "version": 1},
            "scenarios": [
                {
                    "id": "remesh-baseline",
                    "name": "Remesh Baseline",
                    "engine": {"output": "{output_dir}/{scenario}.json"},
                    "reference": {"output": "{output_dir}/{scenario}.json"},
                    "metrics": [
                        {
                            "name": "fps",
                            "higher_is_better": True,
                            "threshold": {"type": "unknown", "max_regression": 0.05},
                        }
                    ],
                }
            ],
        }
    }
    path = tmp_path / "invalid_benchmark.json"
    path.write_text(json.dumps(config), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError) as exc:
        load_configuration(path)
    assert "benchmarks.scenarios[0].metrics[0].threshold.type" in str(exc.value)
