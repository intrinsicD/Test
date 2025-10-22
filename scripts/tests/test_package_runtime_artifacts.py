from __future__ import annotations

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import pytest

from scripts.ci.package_runtime_artifacts import (
    discover_runtime_artefacts,
    package_runtime_artefacts,
)


@pytest.fixture
def build_tree(tmp_path: Path) -> Path:
    build_dir = tmp_path / "out" / "build" / "linux-gcc-debug"
    (build_dir / "engine" / "runtime").mkdir(parents=True)
    (build_dir / "engine" / "runtime" / "libengine_runtime.so").write_bytes(b"runtime")
    (build_dir / "engine" / "runtime" / "engine_runtime.pdb").write_bytes(b"symbols")
    return build_dir


def test_discover_runtime_artefacts_finds_shared_library(build_tree: Path) -> None:
    artefacts = discover_runtime_artefacts(build_tree)
    names = [path.name for path in artefacts]
    assert "libengine_runtime.so" in names
    assert "engine_runtime.pdb" in names


def test_package_runtime_artefacts_copies_files(build_tree: Path, tmp_path: Path) -> None:
    output_dir = tmp_path / "artifacts"
    result = package_runtime_artefacts(build_tree, output_dir)

    packaged_names = sorted(path.name for path in result.artefacts)
    assert "libengine_runtime.so" in packaged_names
    assert "engine_runtime.pdb" in packaged_names
    manifest_path = output_dir / "manifest.json"
    assert manifest_path.exists()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    assert sorted(manifest["artefacts"]) == ["engine_runtime.pdb", "libengine_runtime.so"]


def test_package_runtime_artefacts_cleans_existing_files(build_tree: Path, tmp_path: Path) -> None:
    output_dir = tmp_path / "artifacts"
    output_dir.mkdir(parents=True)
    stale_file = output_dir / "stale.txt"
    stale_file.write_text("stale", encoding="utf-8")

    package_runtime_artefacts(build_tree, output_dir)

    assert not stale_file.exists()
    packaged = [path.name for path in output_dir.iterdir()]
    assert "libengine_runtime.so" in packaged


def test_discover_runtime_artefacts_requires_build_tree(tmp_path: Path) -> None:
    with pytest.raises(FileNotFoundError):
        discover_runtime_artefacts(tmp_path / "missing")


def test_package_runtime_artefacts_requires_shared_library(tmp_path: Path) -> None:
    build_dir = tmp_path / "out" / "build" / "linux-gcc-debug"
    build_dir.mkdir(parents=True)
    with pytest.raises(RuntimeError):
        package_runtime_artefacts(build_dir, tmp_path / "artifacts")
