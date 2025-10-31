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
    assert summary["source_generator"] == "sample_assets"
    assert summary["statistics"]["splits"] == 2
    assert summary["statistics"]["collapses"] == 1
    assert summary["statistics"]["triangle_quality"]["max"] == pytest.approx(0.99, rel=1e-6)
    assert summary["parameterization"]["mode"] == "reuse_existing"
    assert summary["parameterization"]["charts"][0]["boundary_length"] == pytest.approx(4.0)
    assert summary["provenance"]["license"]["name"] == "CC0 1.0 Universal"
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


def test_cli_rejects_duplicate_dataset_identifiers(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    manifest = _sample_manifest_path()

    from scripts.datasets import ingest_dataset as ingest_module

    with pytest.raises(SystemExit) as exc:
        ingest_module.main(
            [
                str(manifest),
                str(manifest),
                "--output",
                str(tmp_path / "datasets"),
            ]
        )

    assert exc.value.code == 2
    captured = capsys.readouterr()
    assert "dataset identifier 'remesh-unit-square'" in captured.err
