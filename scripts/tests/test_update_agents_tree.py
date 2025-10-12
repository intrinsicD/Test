from __future__ import annotations

from pathlib import Path

import sys

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts import update_agents_tree


def test_generate_tree_orders_files_before_directories(tmp_path: Path) -> None:
    root = tmp_path
    (root / "file_b.txt").write_text("", encoding="utf-8")
    (root / "file_a.txt").write_text("", encoding="utf-8")
    nested = root / "nested"
    nested.mkdir()
    (nested / "inner.txt").write_text("", encoding="utf-8")

    lines = update_agents_tree.generate_tree(root)

    assert lines[:4] == [
        ".",
        "    file_a.txt",
        "    file_b.txt",
        "    nested/",
    ]
    assert lines[4:] == [
        "        inner.txt",
    ]


def test_update_agents_synchronises_tree(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    repo_root = tmp_path
    agents = repo_root / "AGENTS.md"
    agents.write_text(
        "\n".join(
            [
                "Header",
                "<!-- BEGIN GENERATED FILE TREE -->",
                "```text",
                ".",
                "```",
                "<!-- END GENERATED FILE TREE -->",
                "Footer",
            ]
        ),
        encoding="utf-8",
    )
    (repo_root / "alpha.txt").write_text("", encoding="utf-8")
    data_dir = repo_root / "data"
    data_dir.mkdir()
    (data_dir / "beta.txt").write_text("", encoding="utf-8")

    monkeypatch.setattr(update_agents_tree, "REPO_ROOT", repo_root)
    monkeypatch.setattr(update_agents_tree, "AGENTS_PATH", agents)

    assert update_agents_tree.update_agents(check_only=True) == 1
    assert update_agents_tree.update_agents() == 0
    content = agents.read_text(encoding="utf-8")
    assert "    alpha.txt" in content
    assert "    data/" in content
    assert "        beta.txt" in content
    assert update_agents_tree.update_agents(check_only=True) == 0
