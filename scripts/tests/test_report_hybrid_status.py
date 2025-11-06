from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

from scripts.workflow import report_hybrid_status as rhs


def _make_task(
    path: Path,
    identifier: str,
    title: str,
    status: str,
    priority: str,
    owner: str,
) -> rhs.TaskMetadata:
    return rhs.TaskMetadata(
        path=path,
        identifier=identifier,
        title=title,
        status=status,
        priority=priority,
        owner=owner,
    )


def test_filter_tasks_can_match_owner() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "alpha.md",
            identifier="HW-100",
            title="Alpha",
            status="ready",
            priority="P1",
            owner="docs-devrel",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "beta.md",
            identifier="HW-101",
            title="Beta",
            status="ready",
            priority="P1",
            owner="runtime-lead",
        ),
    ]

    filtered = rhs.filter_tasks(tasks, status=None, priority=None, owner="docs-devrel")

    assert [task.identifier for task in filtered] == ["HW-100"]


def test_filter_tasks_owner_filter_composes_with_status() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "gamma.md",
            identifier="HW-102",
            title="Gamma",
            status="ready",
            priority="P1",
            owner="docs-devrel",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "delta.md",
            identifier="HW-103",
            title="Delta",
            status="in_progress",
            priority="P1",
            owner="docs-devrel",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "epsilon.md",
            identifier="HW-104",
            title="Epsilon",
            status="ready",
            priority="P2",
            owner="runtime-lead",
        ),
    ]

    filtered = rhs.filter_tasks(tasks, status="ready", priority="P1", owner="docs-devrel")

    assert [task.identifier for task in filtered] == ["HW-102"]


def test_render_table_with_no_results_returns_message() -> None:
    output = rhs.render([], output_format="table")
    assert output == (
        "No tasks matched the supplied filters.\n"
        "Tip: When the ready queue is empty, groom the highest-priority new "
        "backlog item under hybrid_workflow/backlog/ and mark it ready once scoped."
    )


def test_render_json_reports_counts_and_tasks() -> None:
    task_path = rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "example.md"
    task = _make_task(
        path=task_path,
        identifier="HW-201",
        title="Improve hybrid status reporter",
        status="ready",
        priority="P2",
        owner="tools",
    )

    payload = json.loads(rhs.render([task], output_format="json"))

    assert payload["counts"] == {"by_status": {"ready": 1}, "total": 1}
    assert payload["tasks"] == [
        {
            "file": "hybrid_workflow/backlog/example.md",
            "id": "HW-201",
            "owner": "tools",
            "priority": "P2",
            "status": "ready",
            "title": "Improve hybrid status reporter",
        }
    ]


@pytest.mark.parametrize(
    "status_values",
    [
        [],
        [
            _make_task(
                path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "alpha.md",
                identifier="HW-202",
                title="Alpha",
                status="in_progress",
                priority="P1",
                owner="agent",
            ),
            _make_task(
                path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "beta.md",
                identifier="HW-203",
                title="Beta",
                status="done",
                priority="P2",
                owner="agent",
            ),
        ],
    ],
)
def test_render_json_handles_empty_and_multiple_statuses(status_values: list[rhs.TaskMetadata]) -> None:
    payload = json.loads(rhs.render(status_values, output_format="json"))

    assert "counts" in payload
    assert "by_status" in payload["counts"]
    assert payload["counts"]["total"] == len(status_values)


def test_select_next_actions_prefers_ready_tasks() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "gamma.md",
            identifier="HW-204",
            title="Gamma",
            status="in_progress",
            priority="P0",
            owner="agent",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "delta.md",
            identifier="HW-205",
            title="Delta",
            status="ready",
            priority="P1",
            owner="agent",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "epsilon.md",
            identifier="HW-206",
            title="Epsilon",
            status="ready",
            priority="P0",
            owner="agent",
        ),
    ]

    selected = rhs.select_next_actions(tasks, limit=5)

    assert [task.identifier for task in selected] == ["HW-206", "HW-205"]


def test_select_next_actions_falls_back_to_new_when_no_ready() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "zeta.md",
            identifier="HW-207",
            title="Zeta",
            status="new",
            priority="P1",
            owner="agent",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "eta.md",
            identifier="HW-208",
            title="Eta",
            status="new",
            priority="P0",
            owner="agent",
        ),
    ]

    selected = rhs.select_next_actions(tasks, limit=5)

    assert [task.identifier for task in selected] == ["HW-208", "HW-207"]


def test_select_next_actions_respects_limit() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / f"task-{idx}.md",
            identifier=f"HW-{300 + idx}",
            title=f"Task {idx}",
            status="ready",
            priority="P2",
            owner="agent",
        )
        for idx in range(6)
    ]

    selected = rhs.select_next_actions(tasks, limit=3)

    assert len(selected) == 3


def test_select_next_actions_rejects_non_positive_limit() -> None:
    task = _make_task(
        path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "omega.md",
        identifier="HW-400",
        title="Omega",
        status="ready",
        priority="P1",
        owner="agent",
    )

    with pytest.raises(ValueError):
        rhs.select_next_actions([task], limit=0)

