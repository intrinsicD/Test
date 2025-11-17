#!/usr/bin/env python3
"""Hybrid workflow task status dashboard.

This module provides a command-line utility that parses the backlog metadata
files under :mod:`hybrid_workflow/backlog` and renders a concise table or
summary.  It mirrors the repository documentation so contributors can discover
high-priority tasks quickly.

Usage
-----

The tool accepts optional filters for status, priority, area, quality gates,
and blocker metadata:

.. code-block:: bash

   python hybrid_workflow/task_status.py [--status STATUS] [--priority PRIORITY]
                                         [--area AREA] [--owner OWNER]
                                         [--link LINK]
                                         [--gate GATE ...]
                                         [--relates-to TAG ...]
                                         [--search TERM ...]
                                         [--blocked | --unblocked]
                                         [--format {table,json}]

Invoke ``--summary`` for aggregate statistics, ``--detail`` with a task ID to
inspect a single record, and ``--format json`` to emit machine-readable
responses for automation.  Combine ``--blocked`` or ``--unblocked`` with the
other filters to audit blocker status rapidly.  ``--search`` matches across
ID, title, owner, area, gates, relates_to, **blocked_on**, and link metadata so
blocked dependencies are easy to locate, while ``--link`` restricts results to
tasks referencing specific documents or tracking links.
"""

import argparse
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Union


STATUS_ORDER = {
    "new": 0,
    "ready": 1,
    "in_progress": 2,
    "review": 3,
    "done": 4,
    "archived": 5,
}

PRIORITY_ORDER = {
    "P0": 0,
    "P1": 1,
    "P2": 2,
    "P3": 3,
    "P4": 4,
    "P5": 5,
}


def _task_sort_key(task: "Task") -> tuple[int, int, str]:
    """Return a deterministic ordering tuple for tasks."""

    return (
        _priority_rank(task.priority),
        _status_rank(task.status),
        task.id,
    )


@dataclass
class Task:
    """Task metadata parsed from frontmatter."""
    
    id: str = ""
    title: str = ""
    status: str = "new"
    priority: str = "P3"
    area: str = ""
    size: str = "M"
    owner: str = "unassigned"
    gates: List[str] = field(default_factory=list)
    relates_to: List[str] = field(default_factory=list)
    blocked_on: List[str] = field(default_factory=list)
    links: List[str] = field(default_factory=list)
    file_path: Optional[Path] = None


def _strip_inline_comment(value: str) -> str:
    """Remove inline YAML comments from a value string."""

    if '#' in value:
        value = value.split('#', 1)[0]

    return value.strip()


def _strip_quotes(value: str) -> str:
    """Remove matching single or double quotes from a value."""

    if (value.startswith('"') and value.endswith('"')) or (
        value.startswith("'") and value.endswith("'")
    ):
        return value[1:-1]

    return value


def parse_frontmatter(content: str) -> Dict[str, object]:
    """Extract YAML frontmatter from markdown content."""

    lines = content.split('\n')

    if not lines or lines[0].strip() != '---':
        return {}

    frontmatter: Dict[str, object] = {}
    index = 1

    while index < len(lines):
        line = lines[index]
        if line.strip() == '---':
            break

        if ':' not in line:
            index += 1
            continue

        key, raw_value = line.split(':', 1)
        key = key.strip()
        value = _strip_inline_comment(raw_value)

        if not value:
            # Attempt to parse a block list with leading "- " entries.
            items: List[str] = []
            lookahead = index + 1

            while lookahead < len(lines):
                candidate = lines[lookahead]
                stripped = candidate.strip()

                if not stripped:
                    lookahead += 1
                    continue

                if stripped.startswith('- '):
                    item = _strip_inline_comment(stripped[2:].strip())
                    items.append(_strip_quotes(item))
                    lookahead += 1
                    continue

                break

            if items:
                frontmatter[key] = items
                index = lookahead
                continue

            frontmatter[key] = value
            index += 1
            continue

        frontmatter[key] = _strip_quotes(value)
        index += 1

    return frontmatter


def parse_list_field(value: Union[str, List[str]]) -> List[str]:
    """Parse a list field like '[item1, item2]' or '[]'."""
    if isinstance(value, list):
        return [item.strip().strip('"').strip("'") for item in value if item.strip()]

    if not value:
        return []

    value = value.strip()

    if value == '[]':
        return []

    # Remove brackets and quotes
    value = value.strip('[]')
    items = []

    for item in value.split(','):
        stripped = item.strip()

        if not stripped:
            continue

        stripped = _strip_inline_comment(stripped)
        stripped = _strip_quotes(stripped)

        if stripped:
            items.append(stripped)

    return items


def _normalise_filter_values(value: Optional[Union[str, Iterable[str]]]) -> List[str]:
    """Normalise CLI filter values into a list of non-empty strings."""
    if value is None:
        return []

    items: List[str] = []

    if isinstance(value, str):
        for part in value.split(','):
            cleaned = part.strip()
            if cleaned:
                items.append(cleaned)
    else:
        for element in value:
            if isinstance(element, str):
                for part in element.split(','):
                    cleaned = part.strip()
                    if cleaned:
                        items.append(cleaned)
            elif element is not None:
                cleaned = str(element).strip()
                if cleaned:
                    items.append(cleaned)

    return items



def _priority_rank(priority: str) -> int:
    """Return a stable ordering key for priorities."""

    return PRIORITY_ORDER.get(priority, len(PRIORITY_ORDER))


def _status_rank(status: str) -> int:
    """Return a stable ordering key for task status values."""

    return STATUS_ORDER.get(status, len(STATUS_ORDER))


def _task_to_dict(task: Task) -> Dict[str, Any]:
    """Serialise a task into a JSON-compatible dictionary."""

    payload: Dict[str, Any] = {
        "id": task.id,
        "title": task.title,
        "status": task.status,
        "priority": task.priority,
        "area": task.area,
        "size": task.size,
        "owner": task.owner,
        "gates": list(task.gates),
        "relates_to": list(task.relates_to),
        "blocked_on": list(task.blocked_on),
        "links": list(task.links),
    }

    if task.file_path is not None:
        payload["file"] = str(task.file_path)

    return payload


def _status_counts(tasks: List[Task]) -> Dict[str, int]:
    counts: Dict[str, int] = {}
    for task in tasks:
        counts[task.status] = counts.get(task.status, 0) + 1

    return {status: counts[status] for status in sorted(counts, key=_status_rank)}


def _priority_counts(tasks: List[Task]) -> Dict[str, int]:
    counts: Dict[str, int] = {}
    for task in tasks:
        counts[task.priority] = counts.get(task.priority, 0) + 1

    return {priority: counts[priority] for priority in sorted(counts, key=_priority_rank)}


def render_json_tasks(tasks: List[Task], *, total_loaded: int) -> str:
    """Return a JSON payload describing ``tasks``."""

    payload = {
        "tasks": [
            _task_to_dict(task) for task in sorted(tasks, key=_task_sort_key)
        ],
        "counts": {
            "by_status": _status_counts(tasks),
            "by_priority": _priority_counts(tasks),
            "blocked": sum(1 for task in tasks if task.blocked_on),
            "total": len(tasks),
            "available": total_loaded,
        },
    }

    return json.dumps(payload, indent=2, sort_keys=True)


def render_json_summary(tasks: List[Task], *, total_loaded: int) -> str:
    """Return a JSON summary for ``tasks``."""

    payload = {
        "counts": {
            "by_status": _status_counts(tasks),
            "by_priority": _priority_counts(tasks),
            "blocked": sum(1 for task in tasks if task.blocked_on),
            "total": len(tasks),
            "available": total_loaded,
        }
    }

    return json.dumps(payload, indent=2, sort_keys=True)


def render_json_detail(task: Task) -> str:
    """Return a JSON payload representing ``task``."""

    return json.dumps({"task": _task_to_dict(task)}, indent=2, sort_keys=True)


def load_task(file_path: Path) -> Optional[Task]:
    """Load task metadata from a markdown file."""
    try:
        content = file_path.read_text()
        fm = parse_frontmatter(content)
        
        if not fm:
            return None
        
        task = Task(
            id=fm.get('id', ''),
            title=fm.get('title', ''),
            status=fm.get('status', 'new'),
            priority=fm.get('priority', 'P3'),
            area=fm.get('area', ''),
            size=fm.get('size', 'M'),
            owner=fm.get('owner', 'unassigned'),
            gates=parse_list_field(fm.get('gates', [])),
            relates_to=parse_list_field(fm.get('relates_to', [])),
            blocked_on=parse_list_field(fm.get('blocked_on', [])),
            links=parse_list_field(fm.get('links', [])),
            file_path=file_path
        )
        
        return task
    except Exception as e:
        print(f"Error loading {file_path}: {e}")
        return None


def _iter_backlog_files(backlog_dir: Path) -> List[Path]:
    """Return markdown task files in ``backlog_dir`` (excluding the template)."""

    return [
        path
        for path in backlog_dir.glob('*.md')
        if not path.name.startswith('000-')
    ]


def load_all_tasks(backlog_dir: Path, *, include_archived: bool = False) -> List[Task]:
    """Load tasks from ``backlog_dir`` and optionally from its ``archive`` folder."""
    tasks: List[Task] = []

    for md_file in _iter_backlog_files(backlog_dir):
        task = load_task(md_file)
        if task:
            tasks.append(task)

    if include_archived:
        archive_dir = backlog_dir / 'archive'
        if archive_dir.exists():
            for md_file in _iter_backlog_files(archive_dir):
                task = load_task(md_file)
                if task:
                    tasks.append(task)

    return tasks


def filter_tasks(
    tasks: List[Task],
    status: Optional[Union[str, Iterable[str]]] = None,
    priority: Optional[Union[str, Iterable[str]]] = None,
    area: Optional[Union[str, Iterable[str]]] = None,
    size: Optional[Union[str, Iterable[str]]] = None,
    owner: Optional[Union[str, Iterable[str]]] = None,
    relates_to: Optional[List[str]] = None,
    gates: Optional[List[str]] = None,
    links: Optional[Union[str, Iterable[str]]] = None,
    search_terms: Optional[Iterable[str]] = None,
    *,
    blocked_only: Optional[bool] = None,
) -> List[Task]:
    """Filter tasks by criteria (case-insensitive for textual metadata)."""
    filtered = tasks

    status_values = {value.lower() for value in _normalise_filter_values(status)}
    if status_values:
        filtered = [t for t in filtered if t.status.lower() in status_values]

    priority_values = {value.lower() for value in _normalise_filter_values(priority)}
    if priority_values:
        filtered = [t for t in filtered if t.priority.lower() in priority_values]

    area_values = {value.lower() for value in _normalise_filter_values(area)}
    if area_values:
        filtered = [t for t in filtered if t.area.lower() in area_values]

    size_values = {value.lower() for value in _normalise_filter_values(size)}
    if size_values:
        filtered = [t for t in filtered if t.size.lower() in size_values]

    owner_values = {value.lower() for value in _normalise_filter_values(owner)}
    if owner_values:
        filtered = [t for t in filtered if t.owner.lower() in owner_values]

    if gates:
        required = {gate.lower() for gate in gates if gate}
        if required:
            filtered = [
                t
                for t in filtered
                if required.issubset({value.lower() for value in t.gates})
            ]

    if relates_to:
        relates_lower = {tag.lower() for tag in relates_to}
        filtered = [
            t
            for t in filtered
            if relates_lower.intersection({value.lower() for value in t.relates_to})
        ]

    link_values = {value.lower() for value in _normalise_filter_values(links)}
    if link_values:
        filtered = [
            t
            for t in filtered
            if link_values.issubset({value.lower() for value in t.links})
        ]

    if search_terms:
        normalised_terms = [term.strip().lower() for term in search_terms if term]

        if normalised_terms:
            def _matches(term: str, task: Task) -> bool:
                haystack_parts = [
                    task.id,
                    task.title,
                    task.area,
                    task.owner,
                    task.status,
                    task.priority,
                    " ".join(task.relates_to),
                    " ".join(task.gates),
                    " ".join(task.blocked_on),
                    " ".join(task.links),
                ]
                haystack = " ".join(part for part in haystack_parts if part).lower()
                return term in haystack

            filtered = [
                task
                for task in filtered
                if all(_matches(term, task) for term in normalised_terms)
            ]

    if blocked_only is True:
        filtered = [t for t in filtered if t.blocked_on]
    elif blocked_only is False:
        filtered = [t for t in filtered if not t.blocked_on]

    return filtered


def select_next_actions(
    tasks: List[Task],
    limit: int,
    *,
    priority: Optional[Union[str, Iterable[str]]] = None,
    area: Optional[Union[str, Iterable[str]]] = None,
    size: Optional[Union[str, Iterable[str]]] = None,
    owner: Optional[Union[str, Iterable[str]]] = None,
    gates: Optional[List[str]] = None,
    relates_to: Optional[List[str]] = None,
    links: Optional[Union[str, Iterable[str]]] = None,
    search_terms: Optional[Iterable[str]] = None,
    blocked_only: Optional[bool] = None,
) -> List[Task]:
    """Return the highest-priority ready tasks, falling back to new tasks."""

    if limit <= 0:
        raise ValueError("limit must be greater than zero")

    filtered = filter_tasks(
        tasks,
        status=None,
        priority=priority,
        area=area,
        size=size,
        owner=owner,
        gates=gates,
        relates_to=relates_to,
        links=links,
        search_terms=search_terms,
        blocked_only=blocked_only,
    )

    ready = [task for task in filtered if task.status == "ready"]
    pool = ready if ready else [task for task in filtered if task.status == "new"]

    ordered = sorted(pool, key=lambda task: (_priority_rank(task.priority), task.id))

    return ordered[:limit]


def print_task_table(tasks: List[Task]):
    """Print tasks in a formatted table."""
    if not tasks:
        print("No tasks found.")
        return
    
    # Calculate column widths
    id_width = max(len(t.id) for t in tasks) + 2
    title_width = min(max(len(t.title) for t in tasks) + 2, 50)
    status_width = max(len(t.status) for t in tasks) + 2
    priority_width = 4
    area_width = max(len(t.area) for t in tasks) + 2
    owner_width = min(max(len(t.owner) for t in tasks) + 2, 20)
    
    # Header
    header = (f"{'ID':<{id_width}} "
             f"{'Title':<{title_width}} "
             f"{'Status':<{status_width}} "
             f"{'Pri':<{priority_width}} "
             f"{'Area':<{area_width}} "
             f"{'Owner':<{owner_width}}")
    print(header)
    print("-" * len(header))
    
    # Rows
    for task in sorted(tasks, key=_task_sort_key):
        title = task.title[:47] + "..." if len(task.title) > 50 else task.title
        owner = task.owner[:17] + "..." if len(task.owner) > 20 else task.owner

        blocked = " 🚫" if task.blocked_on else ""
        
        row = (f"{task.id:<{id_width}} "
               f"{title:<{title_width}} "
               f"{task.status:<{status_width}} "
               f"{task.priority:<{priority_width}} "
               f"{task.area:<{area_width}} "
               f"{owner:<{owner_width}}{blocked}")
        print(row)


def print_task_details(task: Task):
    """Print detailed information about a task."""
    print(f"\n{'='*70}")
    print(f"Task: {task.id} — {task.title}")
    print(f"{'='*70}")
    print(f"Status:       {task.status}")
    print(f"Priority:     {task.priority}")
    print(f"Area:         {task.area}")
    print(f"Size:         {task.size}")
    print(f"Owner:        {task.owner}")
    print(f"Gates:        {', '.join(task.gates) if task.gates else 'none'}")
    print(f"Relates to:   {', '.join(task.relates_to) if task.relates_to else 'none'}")
    print(f"Blocked on:   {', '.join(task.blocked_on) if task.blocked_on else 'none'}")
    print(f"Links:        {len(task.links)} link(s)")
    print(f"File:         {task.file_path.name}")
    print(f"{'='*70}\n")


def print_summary(tasks: List[Task]):
    """Print summary statistics."""
    total = len(tasks)
    
    # Count by status
    status_counts = {}
    for task in tasks:
        status_counts[task.status] = status_counts.get(task.status, 0) + 1
    
    # Count by priority
    priority_counts = {}
    for task in tasks:
        priority_counts[task.priority] = priority_counts.get(task.priority, 0) + 1
    
    # Count blocked
    blocked_count = sum(1 for t in tasks if t.blocked_on)
    
    print("\n" + "="*50)
    print("TASK SUMMARY")
    print("="*50)
    print(f"Total tasks: {total}")
    print(f"\nBy Status:")
    for status, count in sorted(
        status_counts.items(), key=lambda item: _status_rank(item[0])
    ):
        print(f"  {status:15} {count:3} ({count*100//total if total else 0}%)")

    print(f"\nBy Priority:")
    for priority, count in sorted(
        priority_counts.items(), key=lambda item: _priority_rank(item[0])
    ):
        print(f"  {priority:15} {count:3} ({count*100//total if total else 0}%)")

    print(f"\nBlocked tasks: {blocked_count}")
    print("="*50 + "\n")


def build_parser() -> argparse.ArgumentParser:
    """Construct the argument parser for the CLI."""

    parser = argparse.ArgumentParser(description="Query hybrid workflow task status")
    parser.add_argument(
        '--status',
        action='append',
        metavar='STATUS',
        help=(
            "Filter by status (new, ready, in_progress, review, done). "
            "Repeat or provide comma-separated values to match multiple statuses."
        ),
    )
    parser.add_argument('--priority', help="Filter by priority (P0, P1, P2, P3)")
    parser.add_argument(
        '--area',
        metavar='AREA',
        action='append',
        help=(
            "Filter by area (rendering, geometry, runtime, etc.). Repeat the flag or "
            "provide comma-separated values to match multiple areas (case-insensitive)."
        ),
    )
    parser.add_argument(
        '--size',
        metavar='SIZE',
        action='append',
        help=(
            "Filter by task size (XS, S, M, L, XL). Repeat or provide comma-separated "
            "values to match multiple sizes (case-insensitive)."
        ),
    )
    parser.add_argument(
        '--owner',
        metavar='OWNER',
        action='append',
        help=(
            "Filter by owner (e.g. docs-devrel, runtime-lead). Repeat the flag or "
            "use comma-separated values to match multiple owners (case-insensitive)."
        ),
    )
    parser.add_argument(
        '--gate',
        metavar='GATE',
        action='append',
        nargs='+',
        help=(
            "Filter by quality gate metadata. Provide one or more gates; tasks must "
            "declare all requested gates (case-insensitive)."
        ),
    )
    parser.add_argument(
        '--relates-to',
        metavar='TAG',
        action='append',
        nargs='+',
        help=(
            "Filter by roadmap bundle tag in the relates_to metadata. "
            "Provide one or more tags; the filter matches tasks containing any tag."
        ),
    )
    parser.add_argument(
        '--link',
        metavar='LINK',
        action='append',
        help=(
            "Require tasks to reference specific link metadata (case-insensitive). Repeat the "
            "flag or provide comma-separated values to match multiple links."
        ),
    )
    parser.add_argument(
        '--search',
        metavar='TERM',
        action='append',
        help=(
            "Filter tasks whose metadata contains all provided terms. Matches "
            "against ID, title, owner, area, status, priority, gates, and relates_to."
        ),
    )

    blocker_group = parser.add_mutually_exclusive_group()
    blocker_group.add_argument(
        '--blocked',
        action='store_true',
        help="Limit results to tasks with blockers",
    )
    blocker_group.add_argument(
        '--unblocked',
        action='store_true',
        help="Limit results to tasks without blockers",
    )

    parser.add_argument('--summary', action='store_true', help="Show summary statistics")
    parser.add_argument('--detail', help="Show details for specific task ID")
    parser.add_argument(
        '--include-archived',
        action='store_true',
        help="Include tasks stored in the backlog archive",
    )
    parser.add_argument(
        '--next-actions',
        action='store_true',
        help=(
            "Display the highest-priority ready tasks; falls back to new tasks when "
            "no ready items exist (ignores --status)."
        ),
    )
    parser.add_argument(
        '--limit',
        type=int,
        default=5,
        help=(
            "Maximum number of tasks to show with --next-actions (default: 5). Values "
            "<= 0 are rejected."
        ),
    )
    parser.add_argument(
        '--format',
        choices=('table', 'json'),
        default='table',
        help="Output format for task listings and summaries (default: table).",
    )

    return parser


def main():
    parser = build_parser()

    args = parser.parse_args()
    output_format = args.format
    
    # Find backlog directory
    script_dir = Path(__file__).parent
    backlog_dir = script_dir / 'backlog'
    
    if not backlog_dir.exists():
        print(f"Error: Backlog directory not found at {backlog_dir}")
        return 1
    
    # Load tasks
    tasks = load_all_tasks(backlog_dir, include_archived=args.include_archived)
    
    if not tasks:
        print("No tasks found in backlog directory.")
        return 0
    
    # Show details for specific task
    if args.detail:
        task = next((t for t in tasks if t.id == args.detail), None)
        if task:
            if output_format == 'json':
                print(render_json_detail(task))
            else:
                print_task_details(task)
        else:
            if output_format == 'json':
                print(json.dumps({"error": f"Task {args.detail} not found."}, indent=2, sort_keys=True))
            else:
                print(f"Task {args.detail} not found.")
        return 0
    
    # Filter tasks
    status_filter = _normalise_filter_values(args.status)
    area_filter = _normalise_filter_values(args.area)

    relates_to = None
    if args.relates_to:
        relates_to = [tag for group in args.relates_to for tag in group]

    gates = None
    if args.gate:
        gates = [gate for group in args.gate for gate in group]

    search_terms = _normalise_filter_values(args.search)
    link_filter = _normalise_filter_values(args.link)
    owner_filter = _normalise_filter_values(args.owner)

    blocked_only = True if args.blocked else False if args.unblocked else None

    if args.next_actions:
        try:
            filtered = select_next_actions(
                tasks,
                args.limit,
                priority=args.priority,
                area=area_filter,
                size=args.size,
                owner=owner_filter,
                gates=gates,
                relates_to=relates_to,
                links=link_filter,
                search_terms=search_terms,
                blocked_only=blocked_only,
            )
        except ValueError as exc:
            print(f"Error: {exc}")
            return 1
    else:
        filtered = filter_tasks(
            tasks,
            status=status_filter,
            priority=args.priority,
            area=area_filter,
            size=args.size,
            owner=owner_filter,
            relates_to=relates_to,
            gates=gates,
            links=link_filter,
            search_terms=search_terms,
            blocked_only=blocked_only,
        )

    # Show summary
    if args.summary:
        if output_format == 'json':
            print(render_json_summary(filtered, total_loaded=len(tasks)))
        elif not filtered:
            print("No tasks match the provided filters.")
        else:
            print_summary(filtered)
        return 0

    if output_format == 'json':
        print(render_json_tasks(filtered, total_loaded=len(tasks)))
        return 0

    # Show table
    print_task_table(filtered)
    print(f"\nShowing {len(filtered)} of {len(tasks)} tasks")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

