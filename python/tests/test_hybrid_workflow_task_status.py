from __future__ import annotations

from pathlib import Path
import sys


_TESTS_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _TESTS_DIR.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

_REPO_ROOT = _PROJECT_ROOT.parent
if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

from hybrid_workflow import task_status  # noqa: E402


def _make_task(
    task_id: str,
    *,
    blocked: bool,
    status: str = "in_progress",
    relates_to: list[str] | None = None,
    owner: str = "tools-team",
) -> task_status.Task:
    """Helper to create a task with deterministic defaults for testing."""
    blocked_on = ["dependency"] if blocked else []
    return task_status.Task(
        id=task_id,
        title=f"Task {task_id}",
        status=status,
        priority="P1",
        area="tools",
        owner=owner,
        blocked_on=blocked_on,
        relates_to=relates_to or [],
    )


def test_filter_tasks_returns_blocked_tasks_when_requested() -> None:
    tasks = [
        _make_task("A", blocked=True),
        _make_task("B", blocked=False),
        _make_task("C", blocked=True, status="review"),
    ]

    filtered = task_status.filter_tasks(tasks, blocked_only=True)

    assert [task.id for task in filtered] == ["A", "C"]


def test_filter_tasks_preserves_all_tasks_by_default() -> None:
    tasks = [_make_task("A", blocked=True), _make_task("B", blocked=False)]

    filtered = task_status.filter_tasks(tasks)

    assert {task.id for task in filtered} == {"A", "B"}


def test_filter_tasks_excludes_blocked_when_requested() -> None:
    tasks = [_make_task("A", blocked=True), _make_task("B", blocked=False)]

    filtered = task_status.filter_tasks(tasks, blocked_only=False)

    assert [task.id for task in filtered] == ["B"]


def test_filter_tasks_filters_by_owner() -> None:
    tasks = [
        _make_task("A", blocked=False, owner="tools-team"),
        _make_task("B", blocked=False, owner="runtime-lead"),
    ]

    filtered = task_status.filter_tasks(tasks, owner="runtime-lead")

    assert [task.id for task in filtered] == ["B"]


def test_build_parser_supports_unblocked_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--unblocked'])

    assert args.unblocked is True
    assert args.blocked is False


def test_build_parser_supports_owner_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--owner', 'docs-devrel'])

    assert args.owner == 'docs-devrel'


def test_build_parser_supports_relates_to_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--relates-to', 'bundle:A', 'bundle:C'])

    assert args.relates_to == [['bundle:A', 'bundle:C']]


def test_filter_tasks_supports_relates_to_matching() -> None:
    tasks = [
        _make_task('A', blocked=False, relates_to=['bundle:A', 'bundle:D']),
        _make_task('B', blocked=False, relates_to=['bundle:B']),
        _make_task('C', blocked=False, relates_to=[]),
    ]

    filtered = task_status.filter_tasks(tasks, relates_to=['bundle:B'])

    assert [task.id for task in filtered] == ['B']


def test_filter_tasks_relates_to_is_case_insensitive() -> None:
    tasks = [
        _make_task('A', blocked=False, relates_to=['Bundle:X']),
        _make_task('B', blocked=False, relates_to=['bundle:y']),
    ]

    filtered = task_status.filter_tasks(tasks, relates_to=['bundle:x'])

    assert [task.id for task in filtered] == ['A']


def test_load_task_parses_multiline_lists(tmp_path: Path) -> None:
    task_file = tmp_path / 'TL-332-task-status-multiline.md'
    task_file.write_text(
        """---
id: TL-332
title: Task status multiline parsing
status: done
priority: P3
area: tools
gates: [tests]
relates_to: [bundle:C]
blocked_on:
  - "dependency-A"
  - dependency-B
links:
  - "docs/ROADMAP.md"
  - hybrid_workflow/ROADMAP.md
---
""",
        encoding='utf-8',
    )

    task = task_status.load_task(task_file)

    assert task is not None
    assert task.blocked_on == ['dependency-A', 'dependency-B']
    assert task.links == ['docs/ROADMAP.md', 'hybrid_workflow/ROADMAP.md']
