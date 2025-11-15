#!/usr/bin/env python3
"""Report hybrid workflow task status.

This script queries the hybrid workflow backlog (``hybrid_workflow/backlog/``
subdirectory) and prints either a human readable table or JSON summary grouped
by task status.
"""

from __future__ import annotations

import argparse
import collections
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple, Union

from hybrid_workflow import task_status as hw_task_status

REPO_ROOT = Path(__file__).resolve().parents[2]
BACKLOG_ROOT = REPO_ROOT / "hybrid_workflow" / "backlog"
STATUS_ORDER = hw_task_status.STATUS_ORDER
PRIORITY_ORDER = hw_task_status.PRIORITY_ORDER


@dataclass
class TaskMetadata:
    """Minimal metadata extracted from a hybrid workflow task file."""

    path: Path
    identifier: str
    title: str
    status: str
    priority: str
    owner: str
    area: str = ""
    size: str = ""
    relates_to: Tuple[str, ...] = ()
    gates: Tuple[str, ...] = ()
    blocked_on: Tuple[str, ...] = ()

    @property
    def relative_path(self) -> Path:
        return self.path.relative_to(REPO_ROOT)

    def to_dict(self) -> Dict[str, object]:
        """Return a JSON-serialisable representation of the task."""

        return {
            "id": self.identifier,
            "title": self.title,
            "status": self.status,
            "priority": self.priority,
            "owner": self.owner,
            "area": self.area,
            "size": self.size,
            "gates": list(self.gates),
            "blocked_on": list(self.blocked_on),
            "file": str(self.relative_path),
            "relates_to": list(self.relates_to),
        }


def _task_from_metadata(task: TaskMetadata) -> hw_task_status.Task:
    """Convert :class:`TaskMetadata` into :class:`hw_task_status.Task`."""

    hw_task = hw_task_status.Task(
        id=task.identifier,
        title=task.title,
        status=task.status,
        priority=task.priority,
        area=task.area,
        size=task.size,
        owner=task.owner,
        gates=list(task.gates),
        relates_to=list(task.relates_to),
        blocked_on=list(task.blocked_on),
    )
    hw_task.file_path = task.path
    return hw_task


def _metadata_from_task(task: hw_task_status.Task) -> TaskMetadata:
    """Convert :class:`hw_task_status.Task` into :class:`TaskMetadata`."""

    if task.file_path is None:
        raise ValueError("task.file_path must be set to convert to TaskMetadata")

    return TaskMetadata(
        path=task.file_path,
        identifier=task.id,
        title=task.title,
        status=task.status,
        priority=task.priority,
        owner=task.owner,
        area=task.area,
        size=task.size,
        relates_to=tuple(task.relates_to),
        gates=tuple(task.gates),
        blocked_on=tuple(task.blocked_on),
    )


def collect_tasks(include_archived: bool) -> List[TaskMetadata]:
    """Load task metadata from backlog files."""

    tasks = hw_task_status.load_all_tasks(
        BACKLOG_ROOT,
        include_archived=include_archived,
    )

    return [_metadata_from_task(task) for task in tasks if task.file_path]


def filter_tasks(
    tasks: Iterable[TaskMetadata],
    status: Optional[Union[str, Sequence[str]]],
    priority: Optional[str],
    owner: Optional[str],
    relates_to: Optional[Sequence[str]],
    *,
    area: Optional[str] = None,
    size: Optional[str] = None,
    gates: Optional[Sequence[str]] = None,
    blocked_only: Optional[bool] = None,
) -> List[TaskMetadata]:
    """Apply CLI filters to the task list."""

    hw_tasks = [_task_from_metadata(task) for task in tasks]

    filtered = hw_task_status.filter_tasks(
        hw_tasks,
        status=status,
        priority=priority,
        area=area,
        size=size,
        owner=owner,
        relates_to=list(relates_to) if relates_to else None,
        gates=list(gates) if gates else None,
        blocked_only=blocked_only,
    )

    return [_metadata_from_task(task) for task in filtered]


def _priority_rank(priority: str) -> int:
    return PRIORITY_ORDER.get(priority, len(PRIORITY_ORDER))


def sort_key(task: TaskMetadata) -> tuple:
    return (
        STATUS_ORDER.get(task.status, len(STATUS_ORDER)),
        _priority_rank(task.priority),
        task.identifier,
    )


def select_next_actions(
    tasks: Iterable[TaskMetadata],
    limit: int,
    *,
    priority: Optional[str] = None,
    owner: Optional[str] = None,
    relates_to: Optional[Sequence[str]] = None,
    area: Optional[str] = None,
    size: Optional[str] = None,
    gates: Optional[Sequence[str]] = None,
    blocked_only: Optional[bool] = None,
) -> List[TaskMetadata]:
    """Return the highest-priority ready tasks (or new tasks if none are ready)."""

    if limit <= 0:
        raise ValueError("limit must be greater than zero")

    hw_tasks = [_task_from_metadata(task) for task in tasks]

    selected = hw_task_status.select_next_actions(
        hw_tasks,
        limit,
        priority=priority,
        area=area,
        size=size,
        owner=owner,
        relates_to=list(relates_to) if relates_to else None,
        gates=list(gates) if gates else None,
        blocked_only=blocked_only,
    )

    return [_metadata_from_task(task) for task in selected]


def render_table(tasks: List[TaskMetadata]) -> str:
    """Format the task summary as a human-readable table."""

    if not tasks:
        return (
            "No tasks matched the supplied filters.\n"
            "Tip: When the ready queue is empty, groom the highest-priority new "
            "backlog item under hybrid_workflow/backlog/ and mark it ready once scoped."
        )

    counter = collections.Counter(task.status for task in tasks)
    header = ["Status", "Priority", "ID", "Owner", "Title", "File"]
    rows: List[List[str]] = []
    for task in sorted(tasks, key=sort_key):
        rows.append(
            [
                task.status or "—",
                task.priority or "—",
                task.identifier or "—",
                task.owner or "—",
                task.title or "—",
                str(task.relative_path),
            ]
        )

    widths = [max(len(str(row[idx])) for row in ([header] + rows)) for idx in range(len(header))]

    def format_row(row: List[str]) -> str:
        return "  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row))

    lines = [format_row(header), format_row(["=" * w for w in widths])]
    lines.extend(format_row(row) for row in rows)
    lines.append("")
    lines.append("Status counts:")
    for status, count in sorted(
        counter.items(), key=lambda item: STATUS_ORDER.get(item[0], len(STATUS_ORDER))
    ):
        lines.append(f"  {status}: {count}")
    return "\n".join(lines)


def render_json(tasks: List[TaskMetadata]) -> str:
    """Format the task summary as JSON for automation."""

    if not tasks:
        payload = {"tasks": [], "counts": {"by_status": {}, "total": 0}}
        return json.dumps(payload, indent=2, sort_keys=True)

    counter = collections.Counter(task.status for task in tasks)
    payload = {
        "tasks": [task.to_dict() for task in sorted(tasks, key=sort_key)],
        "counts": {
            "by_status": dict(
                sorted(counter.items(), key=lambda item: STATUS_ORDER.get(item[0], len(STATUS_ORDER)))
            ),
            "total": len(tasks),
        },
    }
    return json.dumps(payload, indent=2, sort_keys=True)


def render(tasks: List[TaskMetadata], output_format: str) -> str:
    if output_format == "json":
        return render_json(tasks)
    return render_table(tasks)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--status", help="Filter by exact status (e.g. ready, in_progress)")
    parser.add_argument("--priority", help="Filter by exact priority (e.g. P1, P2)")
    parser.add_argument("--owner", help="Filter by exact owner (e.g. docs-devrel, runtime-lead)")
    parser.add_argument("--area", help="Filter by task area tag (e.g. rendering, docs)")
    parser.add_argument("--size", help="Filter by task size classification (e.g. S, M, L)")
    parser.add_argument(
        "--relates-to",
        metavar="TAG",
        action="append",
        nargs="+",
        help=(
            "Filter by roadmap bundle metadata stored in relates_to; accepts one or more "
            "tags and matches tasks containing any case-insensitive tag."
        ),
    )
    parser.add_argument(
        "--gate",
        metavar="GATE",
        action="append",
        nargs="+",
        help=(
            "Require tasks to include all specified quality gates; accepts multiple entries "
            "and matches tasks containing every provided gate."
        ),
    )
    blocked_group = parser.add_mutually_exclusive_group()
    blocked_group.add_argument(
        "--blocked",
        action="store_true",
        help="Show only tasks with one or more blocked_on entries.",
    )
    blocked_group.add_argument(
        "--unblocked",
        action="store_true",
        help="Show only tasks with an empty blocked_on list.",
    )
    parser.add_argument(
        "--include-archived",
        action="store_true",
        help="Include tasks from hybrid_workflow/backlog/archive/ in the summary.",
    )
    parser.add_argument(
        "--format",
        choices=("table", "json"),
        default="table",
        help="Output format for the summary (default: table).",
    )
    parser.add_argument(
        "--next-actions",
        action="store_true",
        help=(
            "Show the highest-priority ready tasks; fall back to new tasks when no ready "
            "items exist (ignores --status/--priority filters)."
        ),
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=5,
        help="Maximum number of tasks to display when using --next-actions (default: 5).",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    tasks = collect_tasks(include_archived=args.include_archived)

    relates_to_filter: Optional[List[str]] = None
    if args.relates_to:
        relates_to_filter = [tag for group in args.relates_to for tag in group]

    gates_filter: Optional[List[str]] = None
    if args.gate:
        gates_filter = [gate for group in args.gate for gate in group]

    blocked_only: Optional[bool] = None
    if args.blocked:
        blocked_only = True
    elif args.unblocked:
        blocked_only = False

    if args.next_actions:
        filtered = select_next_actions(
            tasks,
            args.limit,
            priority=args.priority,
            area=args.area,
            size=args.size,
            owner=args.owner,
            relates_to=relates_to_filter,
            gates=gates_filter,
            blocked_only=blocked_only,
        )
    else:
        filtered = filter_tasks(
            tasks,
            args.status,
            args.priority,
            args.owner,
            relates_to_filter,
            area=args.area,
            size=args.size,
            gates=gates_filter,
            blocked_only=blocked_only,
        )
    print(render(filtered, args.format))


if __name__ == "__main__":
    main()
