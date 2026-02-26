#!/usr/bin/env python3
"""Synchronise root TODO.md and DONE.md from hybrid workflow backlog metadata."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence

from scripts.workflow import report_hybrid_status as rhs

REPO_ROOT = Path(__file__).resolve().parents[2]
TODO_PATH = REPO_ROOT / "TODO.md"
DONE_PATH = REPO_ROOT / "DONE.md"

ACTIVE_STATUSES: tuple[str, ...] = ("new", "ready", "in_progress", "review")
DONE_STATUSES: tuple[str, ...] = ("done", "archived")


@dataclass(frozen=True)
class BacklogItem:
    identifier: str
    title: str
    status: str
    priority: str
    owner: str
    relative_path: str



def _to_items(tasks: Iterable[rhs.TaskMetadata]) -> list[BacklogItem]:
    items: list[BacklogItem] = []
    for task in tasks:
        items.append(
            BacklogItem(
                identifier=task.identifier,
                title=task.title,
                status=task.status,
                priority=task.priority,
                owner=task.owner,
                relative_path=str(task.relative_path),
            )
        )
    return items


def _status_rank(status: str) -> int:
    return rhs.STATUS_ORDER.get(status, len(rhs.STATUS_ORDER))


def _priority_rank(priority: str) -> int:
    return rhs.PRIORITY_ORDER.get(priority, len(rhs.PRIORITY_ORDER))


def _sort_key(item: BacklogItem) -> tuple[int, int, str]:
    return (_status_rank(item.status), _priority_rank(item.priority), item.identifier)


def _render(title: str, subtitle: str, items: Sequence[BacklogItem], statuses: Sequence[str]) -> str:
    generated_at = datetime.now(timezone.utc).isoformat(timespec="seconds")
    grouped: dict[str, list[BacklogItem]] = defaultdict(list)
    for item in sorted(items, key=_sort_key):
        grouped[item.status].append(item)

    lines: list[str] = [
        f"# {title}",
        "",
        subtitle,
        "",
        f"_Generated automatically by `python -m scripts.workflow.sync_todo_done` on {generated_at}._",
        "",
    ]

    if not items:
        lines.extend(["No entries currently match this view.", ""])
        return "\n".join(lines)

    for status in statuses:
        section_items = grouped.get(status, [])
        if not section_items:
            continue
        lines.extend([f"## {status}", ""])
        for item in section_items:
            lines.append(
                f"- [{item.identifier}] {item.title} "
                f"(priority: {item.priority}, owner: {item.owner}) "
                f"— `{item.relative_path}`"
            )
        lines.append("")

    return "\n".join(lines)


def sync_todo_done() -> tuple[Path, Path]:
    active_tasks = rhs.collect_tasks(include_archived=False)
    historical_tasks = rhs.collect_tasks(include_archived=True)

    active_items = [item for item in _to_items(active_tasks) if item.status in ACTIVE_STATUSES]
    done_items = [item for item in _to_items(historical_tasks) if item.status in DONE_STATUSES]

    TODO_PATH.write_text(
        _render(
            "TODO",
            "Active backlog work only (items not yet completed).",
            active_items,
            ACTIVE_STATUSES,
        ),
        encoding="utf-8",
    )
    DONE_PATH.write_text(
        _render(
            "DONE",
            "Completed backlog history (done and archived items).",
            done_items,
            DONE_STATUSES,
        ),
        encoding="utf-8",
    )
    return TODO_PATH, DONE_PATH


def main() -> int:
    todo_path, done_path = sync_todo_done()
    print(f"Updated {todo_path.relative_to(REPO_ROOT)}")
    print(f"Updated {done_path.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
