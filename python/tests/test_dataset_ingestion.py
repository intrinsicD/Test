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
