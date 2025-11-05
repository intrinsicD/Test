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


def test_render_table_with_no_results_returns_message() -> None:
    output = rhs.render([], output_format="table")
    assert output == "No tasks matched the supplied filters."


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

