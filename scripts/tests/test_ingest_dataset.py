import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scripts.datasets.ingest_dataset import (  # noqa: E402
    DatasetIngestionError,
    ingest_manifest,
)


def _sample_manifest_path() -> Path:
    return Path(__file__).resolve().parents[2] / "assets/datasets/remesh_sample/manifest.json"


def test_ingest_dataset_writes_summary_and_copies_files(tmp_path: Path) -> None:
    destination = tmp_path / "datasets"

    results = ingest_manifest(_sample_manifest_path(), destination, copy_assets=True)

    assert len(results) == 1
    dataset_dir = destination / "remesh-unit-square"
    summary_path = dataset_dir / "ingestion.json"
    assert summary_path.exists()

    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    assert summary["id"] == "remesh-unit-square"
    assert summary["files"]["source"]["copied_to"].endswith("source_mesh.obj")
    assert summary["files"]["output"]["copied_to"].endswith("output_mesh.obj")

    source_copy = dataset_dir / "source_mesh.obj"
    original_source = _sample_manifest_path().parent / "source_mesh.obj"
    assert source_copy.read_bytes() == original_source.read_bytes()


def test_ingest_dataset_dry_run_skips_writes(tmp_path: Path) -> None:
    destination = tmp_path / "datasets"

    ingest_manifest(_sample_manifest_path(), destination, dry_run=True, copy_assets=True)

    assert not destination.exists()


def test_ingest_dataset_missing_file_raises(tmp_path: Path) -> None:
    manifest_text = _sample_manifest_path().read_text(encoding="utf-8").replace(
        "output_mesh.obj", "missing_mesh.obj",
    )
    manifest_path = tmp_path / "invalid_manifest.json"
    manifest_path.write_text(manifest_text, encoding="utf-8")

    with pytest.raises(DatasetIngestionError):
        ingest_manifest(manifest_path, tmp_path / "out")
