#!/usr/bin/env python3
"""Summarise hybrid workflow tasks using their YAML frontmatter.

The script scans `hybrid_workflow/backlog/` (and optionally its `archive/`
subdirectory) and prints a tabular summary grouped by task status.
"""

from __future__ import annotations

import argparse
import ast
import collections
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

REPO_ROOT = Path(__file__).resolve().parents[2]
BACKLOG_ROOT = REPO_ROOT / "hybrid_workflow" / "backlog"
STATUS_ORDER = {
    "new": 0,
    "ready": 1,
    "in_progress": 2,
    "review": 3,
    "done": 4,
    "archived": 5,
}


@dataclass
class TaskMetadata:
    """Minimal metadata extracted from a hybrid workflow task file."""

    path: Path
    identifier: str
    title: str
    status: str
    priority: str
    owner: str

    @property
    def relative_path(self) -> Path:
        return self.path.relative_to(REPO_ROOT)


def parse_frontmatter(path: Path) -> Dict[str, object]:
    """Parse the YAML frontmatter section of a task file.

    The template uses simple key-value pairs and inline lists; to avoid an
    additional dependency we rely on ``ast.literal_eval`` for list parsing.
    """

    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        raise ValueError(f"{path} does not start with YAML frontmatter")

    data: Dict[str, object] = {}
    for line in lines[1:]:
        stripped = line.strip()
        if stripped == "---":
            break
        if not stripped or stripped.startswith("#"):
            continue
        if ":" not in stripped:
            continue
        key, raw_value = stripped.split(":", 1)
        key = key.strip()
        raw_value = raw_value.strip()
        if not raw_value:
            data[key] = ""
            continue
        if raw_value.startswith("[") and raw_value.endswith("]"):
            try:
                data[key] = ast.literal_eval(raw_value)
            except (SyntaxError, ValueError):
                items = [item.strip() for item in raw_value[1:-1].split(",") if item.strip()]
                data[key] = items
            continue
        if raw_value.startswith(('"', "'")) and raw_value.endswith(('"', "'")):
            raw_value = raw_value[1:-1]
        data[key] = raw_value
    return data


def collect_tasks(include_archived: bool) -> List[TaskMetadata]:
    """Load task metadata from backlog files."""

    candidates: List[Path] = sorted(BACKLOG_ROOT.glob("*.md"))
    if include_archived:
        candidates.extend(sorted((BACKLOG_ROOT / "archive").glob("*.md")))

    tasks: List[TaskMetadata] = []
    for path in candidates:
        if path.name == "000-template.md":
            continue
        try:
            meta = parse_frontmatter(path)
        except ValueError as exc:  # pragma: no cover - defensive branch
            raise SystemExit(str(exc))
        tasks.append(
            TaskMetadata(
                path=path,
                identifier=str(meta.get("id", "")),
                title=str(meta.get("title", "")),
                status=str(meta.get("status", "")).strip(),
                priority=str(meta.get("priority", "")).strip(),
                owner=str(meta.get("owner", "")).strip(),
            )
        )
    return tasks


def filter_tasks(
    tasks: Iterable[TaskMetadata],
    status: Optional[str],
    priority: Optional[str],
) -> List[TaskMetadata]:
    """Apply CLI filters to the task list."""

    filtered: List[TaskMetadata] = []
    for task in tasks:
        if status and task.status != status:
            continue
        if priority and task.priority != priority:
            continue
        filtered.append(task)
    return filtered


def sort_key(task: TaskMetadata) -> tuple:
    return (
        STATUS_ORDER.get(task.status, len(STATUS_ORDER)),
        task.priority,
        task.identifier,
    )


def render(tasks: List[TaskMetadata]) -> str:
    """Format the task summary as a human-readable table."""

    if not tasks:
        return "No tasks matched the supplied filters."

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
    for status, count in sorted(counter.items(), key=lambda item: STATUS_ORDER.get(item[0], len(STATUS_ORDER))):
        lines.append(f"  {status}: {count}")
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--status", help="Filter by exact status (e.g. ready, in_progress)")
    parser.add_argument("--priority", help="Filter by exact priority (e.g. P1, P2)")
    parser.add_argument(
        "--include-archived",
        action="store_true",
        help="Include tasks from hybrid_workflow/backlog/archive/ in the summary.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    tasks = collect_tasks(include_archived=args.include_archived)
    filtered = filter_tasks(tasks, args.status, args.priority)
    print(render(filtered))


if __name__ == "__main__":
    main()
