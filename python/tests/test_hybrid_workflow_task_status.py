from __future__ import annotations

import json
from pathlib import Path
import sys

import pytest


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
    priority: str = "P1",
    area: str = "tools",
    gates: list[str] | None = None,
) -> task_status.Task:
    """Helper to create a task with deterministic defaults for testing."""
    blocked_on = ["dependency"] if blocked else []
    return task_status.Task(
        id=task_id,
        title=f"Task {task_id}",
        status=status,
        priority=priority,
        area=area,
        owner=owner,
        gates=gates or [],
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


def test_filter_tasks_filters_by_gate() -> None:
    tasks = [
        _make_task("A", blocked=False, gates=["tests", "docs"]),
        _make_task("B", blocked=False, gates=["docs"]),
        _make_task("C", blocked=False, gates=["perf"]),
    ]

    filtered = task_status.filter_tasks(tasks, gates=["docs"])

    assert [task.id for task in filtered] == ["A", "B"]


def test_filter_tasks_gate_filter_requires_all_requested_gates() -> None:
    tasks = [
        _make_task("A", blocked=False, gates=["tests", "docs"]),
        _make_task("B", blocked=False, gates=["tests"]),
        _make_task("C", blocked=False, gates=["docs", "perf"]),
    ]

    filtered = task_status.filter_tasks(tasks, gates=["tests", "docs"])

    assert [task.id for task in filtered] == ["A"]


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


def test_build_parser_supports_gate_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--gate', 'tests', 'docs', '--gate', 'perf'])

    assert args.gate == [['tests', 'docs'], ['perf']]


def test_build_parser_supports_format_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--format', 'json'])

    assert args.format == 'json'


def test_build_parser_supports_include_archived_flag() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--include-archived'])

    assert args.include_archived is True


def test_build_parser_supports_next_actions_flags() -> None:
    parser = task_status.build_parser()

    args = parser.parse_args(['--next-actions', '--limit', '3'])

    assert args.next_actions is True
    assert args.limit == 3


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


def test_load_all_tasks_optionally_includes_archive(tmp_path: Path) -> None:
    backlog_dir = tmp_path
    archive_dir = backlog_dir / 'archive'
    archive_dir.mkdir()

    (backlog_dir / 'TL-310-editor-foundations.md').write_text(
        """---
id: TL-310
title: Editor foundations
status: in_progress
priority: P2
area: tools
---
""",
        encoding='utf-8',
    )

    (archive_dir / 'TL-210-experiment-sandbox.md').write_text(
        """---
id: TL-210
title: Experiment sandbox
status: archived
priority: P3
area: tools
---
""",
        encoding='utf-8',
    )

    active_only = task_status.load_all_tasks(backlog_dir)
    assert {task.id for task in active_only} == {'TL-310'}

    with_archive = task_status.load_all_tasks(backlog_dir, include_archived=True)
    assert {task.id for task in with_archive} == {'TL-310', 'TL-210'}


def test_select_next_actions_prefers_ready_tasks() -> None:
    tasks = [
        _make_task('A', blocked=False, status='ready', priority='P2'),
        _make_task('B', blocked=False, status='ready', priority='P1'),
        _make_task('C', blocked=False, status='new', priority='P0'),
    ]

    selected = task_status.select_next_actions(tasks, 5)

    assert [task.id for task in selected] == ['B', 'A']


def test_select_next_actions_falls_back_to_new_when_no_ready() -> None:
    tasks = [
        _make_task('A', blocked=False, status='new', priority='P1'),
        _make_task('B', blocked=False, status='in_progress', priority='P0'),
        _make_task('C', blocked=False, status='new', priority='P2'),
    ]

    selected = task_status.select_next_actions(tasks, 3)

    assert [task.id for task in selected] == ['A', 'C']


def test_select_next_actions_respects_filters_and_limit() -> None:
    tasks = [
        _make_task(
            'A',
            blocked=False,
            status='ready',
            priority='P0',
            owner='tools',
            area='tools',
            gates=['tests', 'docs'],
        ),
        _make_task(
            'B',
            blocked=False,
            status='ready',
            priority='P1',
            owner='runtime',
            area='runtime',
            gates=['tests'],
        ),
        _make_task(
            'C',
            blocked=True,
            status='ready',
            priority='P2',
            owner='tools',
            area='tools',
            gates=['docs'],
        ),
        _make_task(
            'D',
            blocked=False,
            status='new',
            priority='P1',
            owner='tools',
            area='tools',
            gates=['tests', 'docs'],
        ),
    ]

    selected = task_status.select_next_actions(
        tasks,
        1,
        owner='tools',
        area='tools',
        gates=['tests', 'docs'],
        blocked_only=False,
    )

    assert [task.id for task in selected] == ['A']


def test_select_next_actions_raises_for_invalid_limit() -> None:
    with pytest.raises(ValueError):
        task_status.select_next_actions([], 0)


def test_render_json_tasks_serialises_expected_fields() -> None:
    task_a = _make_task(
        'A',
        blocked=True,
        status='ready',
        priority='P0',
        relates_to=['bundle:A'],
        gates=['tests'],
    )
    task_a.links = ['docs/ROADMAP.md']
    task_a.file_path = Path('hybrid_workflow/backlog/A-task.md')

    task_b = _make_task('B', blocked=False, status='new', priority='P2')

    payload = json.loads(
        task_status.render_json_tasks([task_a, task_b], total_loaded=3)
    )

    assert payload['counts']['total'] == 2
    assert payload['counts']['available'] == 3
    assert payload['counts']['blocked'] == 1
    first = payload['tasks'][0]
    assert first['id'] == 'A'
    assert first['blocked_on'] == ['dependency']
    assert first['links'] == ['docs/ROADMAP.md']
    assert first['gates'] == ['tests']
    assert first['file'] == 'hybrid_workflow/backlog/A-task.md'


def test_render_json_summary_reports_counts() -> None:
    tasks = [
        _make_task('A', blocked=True, status='ready', priority='P1'),
        _make_task('B', blocked=False, status='in_progress', priority='P2'),
    ]

    payload = json.loads(task_status.render_json_summary(tasks, total_loaded=4))

    assert payload['counts']['total'] == 2
    assert payload['counts']['available'] == 4
    assert payload['counts']['blocked'] == 1
    assert payload['counts']['by_status']['ready'] == 1
    assert payload['counts']['by_priority']['P1'] == 1


def test_render_json_detail_includes_full_metadata() -> None:
    task = _make_task('Z', blocked=True, status='review', priority='P3')
    task.relates_to = ['bundle:Z']
    task.links = ['docs/ROADMAP.md']

    payload = json.loads(task_status.render_json_detail(task))

    assert payload['task']['id'] == 'Z'
    assert payload['task']['status'] == 'review'
    assert payload['task']['blocked_on'] == ['dependency']
    assert payload['task']['relates_to'] == ['bundle:Z']
