"""Tests for the documentation validation script."""

from __future__ import annotations

from importlib import import_module
from pathlib import Path
import sys

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

validate_docs = import_module("scripts.validate_docs")


def test_should_skip_detects_external_targets() -> None:
    assert validate_docs._should_skip("https://example.com")
    assert validate_docs._should_skip("http://example.com")
    assert validate_docs._should_skip("mailto:info@example.com")
    assert validate_docs._should_skip("#section")


def test_validate_markdown_flags_missing_and_outside(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    repo_root = tmp_path
    docs_dir = repo_root / "docs"
    docs_dir.mkdir()
    nested = docs_dir / "guide"
    nested.mkdir()

    markdown = nested / "sample.md"
    markdown.write_text(
        "\n".join(
            [
                "# Sample",
                "Missing link [Missing](missing.md)",
                "Outside link [Outside](../../../outside.md)",
            ]
        ),
        encoding="utf-8",
    )

    monkeypatch.setattr(validate_docs, "ROOT", repo_root)
    monkeypatch.setattr(validate_docs, "DOCS_DIR", docs_dir)

    issues = validate_docs._validate_markdown(markdown)

    relative_path = markdown.relative_to(repo_root)
    expected = {
        f"{relative_path} -> missing.md (missing)",
        f"{relative_path} -> ../../../outside.md (outside repository)",
    }

    assert set(issues) == expected


def test_main_flags_readme_identifiers_not_in_roadmap(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path
    docs_dir = repo_root / "docs"
    docs_dir.mkdir()
    tasks_dir = docs_dir / "backlog" / "active"
    tasks_dir.mkdir(parents=True)

    roadmap = docs_dir / "ROADMAP.md"
    roadmap.write_text("# Roadmap\n", encoding="utf-8")

    readme = repo_root / "README.md"
    readme.write_text("Next task `DC-999`", encoding="utf-8")

    monkeypatch.setattr(validate_docs, "ROOT", repo_root)
    monkeypatch.setattr(validate_docs, "DOCS_DIR", docs_dir)
    monkeypatch.setattr(validate_docs, "MODULES_DIR", docs_dir / "modules")

    exit_code = validate_docs.main()
    captured = capsys.readouterr().out

    assert exit_code == 1
    assert "README.md references IDs missing from docs/ROADMAP.md: DC-999" in captured


def test_main_flags_missing_task_file_for_active_roadmap_entry(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path
    docs_dir = repo_root / "docs"
    tasks_dir = docs_dir / "backlog" / "active"
    tasks_dir.mkdir(parents=True)

    roadmap = docs_dir / "ROADMAP.md"
    roadmap.write_text(
        "\n".join(
            [
                "| Priority | Backlog | Intent | Owner | Status |",
                "| --- | --- | --- | --- | --- |",
                "| 1 | [`DC-999`](backlog/active/DC-999-ai-004-example.md) | Example | Tools | Planned |",
            ]
        ),
        encoding="utf-8",
    )

    readme = repo_root / "README.md"
    readme.write_text("Tracking `DC-999`", encoding="utf-8")

    monkeypatch.setattr(validate_docs, "ROOT", repo_root)
    monkeypatch.setattr(validate_docs, "DOCS_DIR", docs_dir)
    monkeypatch.setattr(validate_docs, "MODULES_DIR", docs_dir / "modules")

    exit_code = validate_docs.main()
    captured = capsys.readouterr().out

    assert exit_code == 1
    expected_messages = [
        "docs/ROADMAP.md references active task DC-999 without matching docs/backlog/active/DC-999-*.md",
        "docs/ROADMAP.md references active task DC-999 without matching docs/backlog/active/DC_999_*.md",
    ]
    assert any(message in captured for message in expected_messages)


def test_main_passes_when_documents_are_synchronised(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    repo_root = tmp_path
    docs_dir = repo_root / "docs"
    tasks_dir = docs_dir / "backlog" / "active"
    tasks_dir.mkdir(parents=True)

    roadmap = docs_dir / "ROADMAP.md"
    roadmap.write_text(
        "\n".join(
            [
                "| Priority | Backlog | Intent | Owner | Status |",
                "| --- | --- | --- | --- | --- |",
                "| 1 | [`DC-999`](backlog/active/DC-999-ai-004-example.md) | Example | Tools | Planned |",
            ]
        ),
        encoding="utf-8",
    )

    readme = repo_root / "README.md"
    readme.write_text("Tracking `DC-999`", encoding="utf-8")

    task_file = tasks_dir / "DC-999-ai-004-example.md"
    task_file.write_text("# DC-999", encoding="utf-8")

    monkeypatch.setattr(validate_docs, "ROOT", repo_root)
    monkeypatch.setattr(validate_docs, "DOCS_DIR", docs_dir)
    monkeypatch.setattr(validate_docs, "MODULES_DIR", docs_dir / "modules")

    exit_code = validate_docs.main()
    captured = capsys.readouterr().out

    assert exit_code == 0
    assert "All documentation links resolved successfully." in captured
