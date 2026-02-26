from __future__ import annotations

import sys
from pathlib import Path

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts.workflow import sync_todo_done


def test_sync_todo_done_writes_expected_sections(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    repo_root = tmp_path / "repo"
    backlog_root = repo_root / "hybrid_workflow" / "backlog"
    archive_root = backlog_root / "archive"
    archive_root.mkdir(parents=True)

    (backlog_root / "TL-100-active.md").write_text(
        """---
id: TL-100
title: Active task
status: in_progress
priority: P1
owner: tools
---
""",
        encoding="utf-8",
    )
    (archive_root / "TL-200-done.md").write_text(
        """---
id: TL-200
title: Done task
status: archived
priority: P2
owner: docs
---
""",
        encoding="utf-8",
    )

    monkeypatch.setattr(sync_todo_done, "REPO_ROOT", repo_root)
    monkeypatch.setattr(sync_todo_done, "TODO_PATH", repo_root / "TODO.md")
    monkeypatch.setattr(sync_todo_done, "DONE_PATH", repo_root / "DONE.md")
    monkeypatch.setattr(sync_todo_done.rhs, "REPO_ROOT", repo_root)
    monkeypatch.setattr(sync_todo_done.rhs, "BACKLOG_ROOT", backlog_root)

    sync_todo_done.sync_todo_done()

    todo_text = (repo_root / "TODO.md").read_text(encoding="utf-8")
    done_text = (repo_root / "DONE.md").read_text(encoding="utf-8")

    assert "# TODO" in todo_text
    assert "TL-100" in todo_text
    assert "TL-200" not in todo_text

    assert "# DONE" in done_text
    assert "TL-200" in done_text
    assert "TL-100" not in done_text
