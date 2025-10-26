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
    ConfigurationSchemaError,
    DatasetManifest,
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


def test_invalid_dataset_identifier_raises(tmp_path: Path) -> None:
    entry = deepcopy(BASE_ENTRY)
    entry["id"] = "Invalid Name"
    path = tmp_path / "invalid.json"
    path.write_text(json.dumps({"datasets": [entry]}), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError) as exc:
        load_dataset_manifest(path)
    assert "datasets[0].id" in str(exc.value)
