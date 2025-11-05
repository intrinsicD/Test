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


def _make_task(task_id: str, *, blocked: bool, status: str = "in_progress") -> task_status.Task:
    """Helper to create a task with deterministic defaults for testing."""
    blocked_on = ["dependency"] if blocked else []
    return task_status.Task(
        id=task_id,
        title=f"Task {task_id}",
        status=status,
        priority="P1",
        area="tools",
        blocked_on=blocked_on,
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


def test_build_parser_supports_unblocked_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--unblocked'])

    assert args.unblocked is True
    assert args.blocked is False
