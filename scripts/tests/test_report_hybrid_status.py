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
    *,
    relates_to: tuple[str, ...] = (),
    area: str = "",
    size: str = "",
    gates: tuple[str, ...] = (),
    blocked_on: tuple[str, ...] = (),
) -> rhs.TaskMetadata:
    return rhs.TaskMetadata(
        path=path,
        identifier=identifier,
        title=title,
        status=status,
        priority=priority,
        owner=owner,
        relates_to=relates_to,
        area=area,
        size=size,
        gates=gates,
        blocked_on=blocked_on,
    )


def test_parse_frontmatter_reads_file(tmp_path: Path) -> None:
    task_file = tmp_path / "TL-999-sample.md"
    task_file.write_text(
        """---
id: TL-999
title: Sample task
status: ready
priority: P1
area: tools
---
""",
        encoding="utf-8",
    )

    metadata = rhs.parse_frontmatter(task_file)

    assert metadata["id"] == "TL-999"
    assert metadata["status"] == "ready"


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

    filtered = rhs.filter_tasks(
        tasks, status=None, priority=None, owner="docs-devrel", relates_to=None
    )

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

    filtered = rhs.filter_tasks(
        tasks,
        status="ready",
        priority="P1",
        owner="docs-devrel",
        relates_to=None,
    )

    assert [task.identifier for task in filtered] == ["HW-102"]


def test_filter_tasks_can_match_relates_to_tags() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "theta.md",
            identifier="HW-110",
            title="Theta",
            status="ready",
            priority="P1",
            owner="tools-team",
            relates_to=("bundle:A", "bundle:D"),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "iota.md",
            identifier="HW-111",
            title="Iota",
            status="ready",
            priority="P2",
            owner="tools-team",
            relates_to=("bundle:C",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "kappa.md",
            identifier="HW-112",
            title="Kappa",
            status="ready",
            priority="P2",
            owner="tools-team",
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=("bundle:A",),
    )

    assert [task.identifier for task in filtered] == ["HW-110"]


def test_filter_tasks_can_match_area() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "area-alpha.md",
            identifier="HW-120",
            title="Area Alpha",
            status="ready",
            priority="P1",
            owner="docs-devrel",
            area="rendering",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "area-beta.md",
            identifier="HW-121",
            title="Area Beta",
            status="ready",
            priority="P1",
            owner="docs-devrel",
            area="docs",
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=None,
        area="rendering",
    )

    assert [task.identifier for task in filtered] == ["HW-120"]


def test_filter_tasks_can_match_size() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "size-alpha.md",
            identifier="HW-122",
            title="Size Alpha",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            size="S",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "size-beta.md",
            identifier="HW-123",
            title="Size Beta",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            size="L",
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=None,
        size="S",
    )

    assert [task.identifier for task in filtered] == ["HW-122"]


def test_filter_tasks_require_all_requested_gates() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "gate-alpha.md",
            identifier="HW-124",
            title="Gate Alpha",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            gates=("tests", "docs"),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "gate-beta.md",
            identifier="HW-125",
            title="Gate Beta",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            gates=("tests",),
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=None,
        gates=("tests", "docs"),
    )

    assert [task.identifier for task in filtered] == ["HW-124"]


def test_filter_tasks_support_blocked_only_flag() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "blocked-alpha.md",
            identifier="HW-126",
            title="Blocked Alpha",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            blocked_on=("dependency",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "blocked-beta.md",
            identifier="HW-127",
            title="Blocked Beta",
            status="ready",
            priority="P1",
            owner="runtime-lead",
            blocked_on=(),
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=None,
        blocked_only=True,
    )

    assert [task.identifier for task in filtered] == ["HW-126"]


def test_filter_tasks_relates_to_is_case_insensitive() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "lambda.md",
            identifier="HW-113",
            title="Lambda",
            status="ready",
            priority="P2",
            owner="tools-team",
            relates_to=("bundle:X",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "mu.md",
            identifier="HW-114",
            title="Mu",
            status="ready",
            priority="P2",
            owner="tools-team",
            relates_to=("bundle:Y",),
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=("BUNDLE:x",),
    )

    assert [task.identifier for task in filtered] == ["HW-113"]


def test_filter_tasks_can_match_search_terms() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "docs-task.md",
            identifier="HW-130",
            title="Docs task",
            status="ready",
            priority="P1",
            owner="docs-devrel",
            area="docs",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "runtime-task.md",
            identifier="HW-131",
            title="Runtime task",
            status="ready",
            priority="P2",
            owner="runtime-lead",
            area="runtime",
        ),
    ]

    filtered = rhs.filter_tasks(
        tasks,
        status=None,
        priority=None,
        owner=None,
        relates_to=None,
        search_terms=("docs",),
    )

    assert [task.identifier for task in filtered] == ["HW-130"]


def test_parse_args_supports_relates_to(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "report_hybrid_status",
            "--relates-to",
            "bundle:A",
            "bundle:C",
            "--relates-to",
            "bundle:D",
        ],
    )

    args = rhs.parse_args()

    assert args.relates_to == [["bundle:A", "bundle:C"], ["bundle:D"]]


def test_parse_args_supports_area_and_size(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "report_hybrid_status",
            "--area",
            "rendering",
            "--size",
            "S",
        ],
    )

    args = rhs.parse_args()

    assert args.area == "rendering"
    assert args.size == "S"


def test_parse_args_supports_gate_and_blocked_flags(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "report_hybrid_status",
            "--gate",
            "tests",
            "docs",
            "--blocked",
        ],
    )

    args = rhs.parse_args()

    assert args.gate == [["tests", "docs"]]
    assert args.blocked is True
    assert args.unblocked is False


def test_parse_args_supports_search_terms(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "report_hybrid_status",
            "--search",
            "docs",
            "panel",
            "--search",
            "P1",
        ],
    )

    args = rhs.parse_args()

    assert args.search == [["docs", "panel"], ["P1"]]


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
        relates_to=("bundle:C",),
    )

    payload = json.loads(rhs.render([task], output_format="json"))

    assert payload["counts"] == {"by_status": {"ready": 1}, "total": 1}
    assert payload["tasks"] == [
        {
            "area": "",
            "blocked_on": [],
            "file": "hybrid_workflow/backlog/example.md",
            "gates": [],
            "id": "HW-201",
            "owner": "tools",
            "priority": "P2",
            "relates_to": ["bundle:C"],
            "size": "",
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


def test_select_next_actions_respects_owner_filter() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "theta.md",
            identifier="HW-209",
            title="Theta",
            status="ready",
            priority="P1",
            owner="runtime-lead",
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "iota.md",
            identifier="HW-210",
            title="Iota",
            status="ready",
            priority="P0",
            owner="docs-devrel",
        ),
    ]

    selected = rhs.select_next_actions(tasks, limit=5, owner="docs-devrel")

    assert [task.identifier for task in selected] == ["HW-210"]


def test_select_next_actions_filters_apply_before_fallback() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "kappa.md",
            identifier="HW-211",
            title="Kappa",
            status="ready",
            priority="P0",
            owner="runtime-lead",
            relates_to=("bundle:A",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "lambda.md",
            identifier="HW-212",
            title="Lambda",
            status="new",
            priority="P1",
            owner="docs-devrel",
            relates_to=("bundle:C",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "mu.md",
            identifier="HW-213",
            title="Mu",
            status="new",
            priority="P2",
            owner="runtime-lead",
            relates_to=("bundle:C",),
        ),
    ]

    selected = rhs.select_next_actions(tasks, limit=5, owner="docs-devrel")

    assert [task.identifier for task in selected] == ["HW-212"]


def test_select_next_actions_respects_relates_to_case_insensitive() -> None:
    tasks = [
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "nu.md",
            identifier="HW-214",
            title="Nu",
            status="ready",
            priority="P1",
            owner="tools-team",
            relates_to=("bundle:X",),
        ),
        _make_task(
            path=rhs.REPO_ROOT / "hybrid_workflow" / "backlog" / "xi.md",
            identifier="HW-215",
            title="Xi",
            status="ready",
            priority="P0",
            owner="tools-team",
            relates_to=("bundle:Y",),
        ),
    ]

    selected = rhs.select_next_actions(tasks, limit=5, relates_to=("BUNDLE:x",))

    assert [task.identifier for task in selected] == ["HW-214"]


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

