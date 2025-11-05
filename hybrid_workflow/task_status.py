#!/usr/bin/env python3
"""Hybrid workflow task status dashboard.

This module provides a command-line utility that parses the backlog metadata
files under :mod:`hybrid_workflow/backlog` and renders a concise table or
summary.  It mirrors the repository documentation so contributors can discover
high-priority tasks quickly.

Usage
-----

The tool accepts optional filters for status, priority, area, and blocker
metadata:

.. code-block:: bash

   python hybrid_workflow/task_status.py [--status STATUS] [--priority PRIORITY]
                                        [--area AREA] [--relates-to TAG ...]
                                        [--blocked | --unblocked]

Invoke ``--summary`` for aggregate statistics or ``--detail`` with a task ID to
inspect a single record.  Combine ``--blocked`` or ``--unblocked`` with the
other filters to audit blocker status rapidly.
"""

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Union


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


def load_all_tasks(backlog_dir: Path) -> List[Task]:
    """Load all tasks from backlog directory (excluding archive and template)."""
    tasks = []
    
    for md_file in backlog_dir.glob('*.md'):
        if md_file.name.startswith('000-'):
            continue  # Skip template
        
        task = load_task(md_file)
        if task:
            tasks.append(task)
    
    return tasks


def filter_tasks(
    tasks: List[Task],
    status: Optional[str] = None,
    priority: Optional[str] = None,
    area: Optional[str] = None,
    relates_to: Optional[List[str]] = None,
    *,
    blocked_only: Optional[bool] = None,
) -> List[Task]:
    """Filter tasks by criteria."""
    filtered = tasks

    if status:
        filtered = [t for t in filtered if t.status == status]

    if priority:
        filtered = [t for t in filtered if t.priority == priority]

    if area:
        filtered = [t for t in filtered if t.area == area]

    if relates_to:
        relates_lower = {tag.lower() for tag in relates_to}
        filtered = [
            t
            for t in filtered
            if relates_lower.intersection({value.lower() for value in t.relates_to})
        ]

    if blocked_only is True:
        filtered = [t for t in filtered if t.blocked_on]
    elif blocked_only is False:
        filtered = [t for t in filtered if not t.blocked_on]

    return filtered


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
    for task in sorted(tasks, key=lambda t: (t.priority, t.status, t.id)):
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
    for status, count in sorted(status_counts.items()):
        print(f"  {status:15} {count:3} ({count*100//total if total else 0}%)")
    
    print(f"\nBy Priority:")
    for priority, count in sorted(priority_counts.items()):
        print(f"  {priority:15} {count:3} ({count*100//total if total else 0}%)")
    
    print(f"\nBlocked tasks: {blocked_count}")
    print("="*50 + "\n")


def build_parser() -> argparse.ArgumentParser:
    """Construct the argument parser for the CLI."""

    parser = argparse.ArgumentParser(description="Query hybrid workflow task status")
    parser.add_argument('--status', help="Filter by status (new, ready, in_progress, review, done)")
    parser.add_argument('--priority', help="Filter by priority (P0, P1, P2, P3)")
    parser.add_argument('--area', help="Filter by area (rendering, geometry, runtime, etc.)")
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

    return parser


def main():
    parser = build_parser()

    args = parser.parse_args()
    
    # Find backlog directory
    script_dir = Path(__file__).parent
    backlog_dir = script_dir / 'backlog'
    
    if not backlog_dir.exists():
        print(f"Error: Backlog directory not found at {backlog_dir}")
        return 1
    
    # Load tasks
    tasks = load_all_tasks(backlog_dir)
    
    if not tasks:
        print("No tasks found in backlog directory.")
        return 0
    
    # Show details for specific task
    if args.detail:
        task = next((t for t in tasks if t.id == args.detail), None)
        if task:
            print_task_details(task)
        else:
            print(f"Task {args.detail} not found.")
        return 0
    
    # Filter tasks
    relates_to = None
    if args.relates_to:
        relates_to = [tag for group in args.relates_to for tag in group]

    filtered = filter_tasks(
        tasks,
        args.status,
        args.priority,
        args.area,
        relates_to,
        blocked_only=True if args.blocked else False if args.unblocked else None,
    )
    
    # Show summary
    if args.summary:
        if not filtered:
            print("No tasks match the provided filters.")
        else:
            print_summary(filtered)
        return 0
    
    # Show table
    print_task_table(filtered)
    print(f"\nShowing {len(filtered)} of {len(tasks)} tasks")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())

