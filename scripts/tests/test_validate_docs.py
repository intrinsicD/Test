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
