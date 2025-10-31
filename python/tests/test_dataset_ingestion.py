import json
import sys
from pathlib import Path

import pytest

_TESTS_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _TESTS_DIR.parent
_REPO_ROOT = _PROJECT_ROOT.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from scripts.datasets.ingest_dataset import DatasetIngestionError, ingest_manifest


def test_ingest_manifest_verifies_hashes(tmp_path: Path) -> None:
    manifest_path = _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    results = ingest_manifest(manifest_path, tmp_path, dry_run=True, require_schema=True)
    assert len(results) == 1
    dataset_result = results[0]
    assert all(metadata.verified for metadata in dataset_result.files)
    assert dataset_result.entry.schema_version == 2


def test_ingest_manifest_detects_hash_mismatch(tmp_path: Path) -> None:
    manifest_path = _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    manifest_data = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest_data["datasets"][0]["source"]["mesh_sha256"] = "0" * 64
    faulty_manifest = tmp_path / "faulty_manifest.json"
    faulty_manifest.write_text(json.dumps(manifest_data), encoding="utf-8")

    with pytest.raises(DatasetIngestionError):
        ingest_manifest(faulty_manifest, tmp_path, dry_run=True, require_schema=True)


def test_ingest_manifest_accepts_uppercase_hashes(tmp_path: Path) -> None:
    manifest_path = _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    manifest_data = json.loads(manifest_path.read_text(encoding="utf-8"))
    dataset = manifest_data["datasets"][0]
    dataset["source"]["mesh_sha256"] = dataset["source"]["mesh_sha256"].upper()
    dataset["outputs"]["mesh_sha256"] = dataset["outputs"]["mesh_sha256"].upper()
    dataset_dir = manifest_path.parent
    dataset["source"]["mesh"] = str((dataset_dir / dataset["source"]["mesh"]).resolve())
    dataset["outputs"]["mesh"] = str((dataset_dir / dataset["outputs"]["mesh"]).resolve())
    uppercase_manifest = tmp_path / "uppercase_manifest.json"
    uppercase_manifest.write_text(json.dumps(manifest_data), encoding="utf-8")

    results = ingest_manifest(uppercase_manifest, tmp_path, dry_run=False, require_schema=True)
    assert len(results) == 1
    dataset_result = results[0]
    assert all(metadata.verified for metadata in dataset_result.files)

    summary_path = dataset_result.summary_path
    assert summary_path is not None
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    source_file = summary["files"]["source"]
    output_file = summary["files"]["output"]
    assert source_file["expected_sha256"] == source_file["sha256"]
    assert output_file["expected_sha256"] == output_file["sha256"]


def test_ingest_manifest_writes_summary_with_extended_statistics(tmp_path: Path) -> None:
    manifest_path = _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    results = ingest_manifest(manifest_path, tmp_path, dry_run=False, require_schema=True)
    assert len(results) == 1
    summary_path = results[0].summary_path
    assert summary_path is not None
    summary = json.loads(summary_path.read_text(encoding="utf-8"))

    metrics = summary["metrics"]
    assert metrics["input"]["surface_area"] == pytest.approx(1.0)
    assert metrics["output"]["surface_area"] == pytest.approx(1.0)

    statistics = summary["statistics"]
    assert statistics["iterations"] == 6
    assert statistics["splits"] == 2
    assert statistics["collapses"] == 1
    assert statistics["duration_ms"] == 3.2
    assert statistics["triangles"] == 4
    assert statistics["triangle_quality"] == {"min": 0.64, "mean": 0.82, "max": 0.99}
    assert summary["source_generator"] == "sample_assets"


def test_ingest_manifest_supports_multiple_categories(tmp_path: Path) -> None:
    manifests = [
        _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json",
        _REPO_ROOT / "assets" / "datasets" / "rendering_sample" / "manifest.json",
        _REPO_ROOT / "assets" / "datasets" / "animation_sample" / "manifest.json",
    ]

    processed = []
    for manifest_path in manifests:
        results = ingest_manifest(manifest_path, tmp_path, dry_run=True, require_schema=True)
        processed.extend(results)

    identifiers = {result.entry.identifier for result in processed}
    kinds = {result.entry.kind for result in processed}
    tags = {tag for result in processed for tag in result.entry.tags}

    assert identifiers == {
        "remesh-unit-square",
        "rendering-quad-shading",
        "animation-walk-retarget",
    }
    assert kinds == {"geometry.remesh", "rendering.debug", "animation.retarget"}
    assert {"geometry", "rendering", "animation"}.issubset(tags)
    assert all(all(file.verified for file in result.files) for result in processed)


def test_cli_summary_generates_aggregated_payload(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    manifest_path = _REPO_ROOT / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    summary_path = tmp_path / "artifacts" / "summary.json"

    from scripts.datasets import ingest_dataset

    exit_code = ingest_dataset.main(
        [
            str(manifest_path),
            "--dry-run",
            "--summary",
            str(summary_path),
            "--require-schema",
        ]
    )

    captured = capsys.readouterr()
    assert exit_code == 0
    assert summary_path.exists()
    assert "dataset=remesh-unit-square" in captured.out

    payload = json.loads(summary_path.read_text(encoding="utf-8"))
    assert "manifests" in payload
    assert payload["manifests"][0]["datasets"][0]["id"] == "remesh-unit-square"
    files = payload["manifests"][0]["datasets"][0]["files"]
    assert "source" in files and "output" in files
