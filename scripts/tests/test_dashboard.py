from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
import sys

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts.workflow import dashboard


def _make_task(identifier: str, **overrides: object) -> dashboard.TaskRecord:
    defaults = dict(
        title="Sample",
        status="ready",
        priority="P1",
        area="tools",
        owner="owner",
        size="M",
        gates=["docs"],
        relates_to=["bundle:C"],
        blocked_on=[],
        links=[],
        path=dashboard.REPO_ROOT / "hybrid_workflow" / "backlog" / f"{identifier}.md",
    )
    defaults.update(overrides)
    return dashboard.TaskRecord(identifier=identifier, **defaults)


def test_compute_summary_tracks_counts() -> None:
    tasks = [
        _make_task("TL-001", status="ready", blocked_on=[]),
        _make_task("TL-002", status="in_progress", priority="P2", blocked_on=["RT-410"]),
    ]

    summary = dashboard.compute_summary(tasks)

    assert summary.total_tasks == 2
    assert summary.by_status["ready"] == 1
    assert summary.by_priority["P1"] == 1
    assert summary.blocked_tasks == 1


def test_render_html_contains_task_rows() -> None:
    tasks = [
        _make_task("TL-010", title="Dashboard"),
        _make_task("TL-011", status="review", blocked_on=["Dependency"]),
    ]
    summary = dashboard.compute_summary(tasks)
    rendered = dashboard.render_html(
        tasks,
        summary,
        datetime(2025, 1, 1, tzinfo=timezone.utc),
        include_archived=False,
    )

    assert "Hybrid Workflow Task Dashboard" in rendered
    assert "TL-010" in rendered
    assert "Dependency" in rendered


def test_write_outputs_creates_files(tmp_path: Path) -> None:
    tasks = [_make_task("TL-050"), _make_task("TL-051", blocked_on=["Task"])]
    summary = dashboard.compute_summary(tasks)
    generated_at = datetime(2024, 12, 25, 12, 0, tzinfo=timezone.utc)
    payload = dashboard.build_json_payload(tasks, summary, generated_at, include_archived=True)
    html_document = dashboard.render_html(tasks, summary, generated_at, include_archived=True)

    html_path, json_path = dashboard.write_outputs(tmp_path, html_document, payload)

    assert html_path.exists()
    data = json.loads(json_path.read_text(encoding="utf-8"))
    assert data["include_archived"] is True
    assert data["summary"]["total_tasks"] == 2
    assert len(data["tasks"]) == 2


def test_load_tasks_returns_entries() -> None:
    tasks = dashboard.load_tasks(include_archived=False)
    assert tasks, "Expected at least one active hybrid workflow task"


def test_main_writes_default_location(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(dashboard, "REPO_ROOT", tmp_path)
    backlog_root = tmp_path / "hybrid_workflow" / "backlog"
    backlog_root.mkdir(parents=True)
    monkeypatch.setattr(dashboard, "BACKLOG_ROOT", backlog_root)
    task_file = backlog_root / "TL-999-sample.md"
    task_file.write_text(
        """---
id: TL-999
title: Sample task
status: ready
priority: P1
area: tools
size: M
owner: agent
gates: [docs]
relates_to: [bundle:C]
blocked_on: []
links: []
---
""",
        encoding="utf-8",
    )
    args = ["--output-dir", str(tmp_path / "output")]  # explicit output directory
    monkeypatch.setattr(sys, "argv", ["dashboard"] + args)

    exit_code = dashboard.main()

    assert exit_code == 0
    output_dir = tmp_path / "output"
    assert (output_dir / "index.html").exists()
    assert (output_dir / "tasks.json").exists()


def test_main_supports_output_outside_repo(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    repo_root = tmp_path / "repo"
    monkeypatch.setattr(dashboard, "REPO_ROOT", repo_root)
    backlog_root = repo_root / "hybrid_workflow" / "backlog"
    backlog_root.mkdir(parents=True)
    monkeypatch.setattr(dashboard, "BACKLOG_ROOT", backlog_root)
    task_file = backlog_root / "TL-100-sample.md"
    task_file.write_text(
        """---
id: TL-100
title: Sample task
status: ready
priority: P1
area: tools
size: M
owner: agent
gates: [docs]
relates_to: [bundle:C]
blocked_on: []
links: []
---
""",
        encoding="utf-8",
    )

    outside_dir = tmp_path / "external" / "dashboard"
    args = ["--output-dir", str(outside_dir)]
    monkeypatch.setattr(sys, "argv", ["dashboard"] + args)

    exit_code = dashboard.main()

    assert exit_code == 0
    assert (outside_dir / "index.html").exists()
    assert (outside_dir / "tasks.json").exists()
    captured = capsys.readouterr()
    assert str(outside_dir) in captured.out
