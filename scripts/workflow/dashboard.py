"""Generate an HTML + JSON dashboard for hybrid workflow tasks.

The dashboard surfaces summary statistics and a sortable table so stakeholders
can review task health without invoking multiple command-line queries.
"""

from __future__ import annotations

import argparse
import json
import html
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Sequence

from .report_hybrid_status import (
    BACKLOG_ROOT,
    REPO_ROOT,
    STATUS_ORDER,
    parse_frontmatter,
)

# Priority ordering keeps the dashboard deterministic even when metadata uses
# alternative casing or unexpected values.
PRIORITY_ORDER = {f"P{index}": index for index in range(5)}


@dataclass(frozen=True)
class TaskRecord:
    """Normalised representation of a hybrid workflow task."""

    identifier: str
    title: str
    status: str
    priority: str
    area: str
    owner: str
    size: str
    gates: List[str]
    relates_to: List[str]
    blocked_on: List[str]
    links: List[str]
    path: Path

    @property
    def relative_path(self) -> Path:
        return self.path.relative_to(REPO_ROOT)

    def to_json(self) -> Dict[str, object]:
        return {
            "id": self.identifier,
            "title": self.title,
            "status": self.status,
            "priority": self.priority,
            "area": self.area,
            "owner": self.owner,
            "size": self.size,
            "gates": list(self.gates),
            "relates_to": list(self.relates_to),
            "blocked_on": list(self.blocked_on),
            "links": list(self.links),
            "file": str(self.relative_path),
        }


@dataclass(frozen=True)
class DashboardSummary:
    total_tasks: int
    by_status: Dict[str, int]
    by_priority: Dict[str, int]
    blocked_tasks: int


def _ensure_list(value: object) -> List[str]:
    if isinstance(value, (list, tuple)):
        return [str(item) for item in value if str(item)]
    if value in (None, ""):
        return []
    return [str(value)]


def _normalise_string(value: object, default: str = "") -> str:
    if value is None:
        return default
    return str(value).strip() or default


def load_tasks(include_archived: bool) -> List[TaskRecord]:
    """Parse task metadata from backlog markdown files."""

    backlog_files: List[Path] = sorted(BACKLOG_ROOT.glob("*.md"))
    if include_archived:
        backlog_files.extend(sorted((BACKLOG_ROOT / "archive").glob("*.md")))

    tasks: List[TaskRecord] = []
    for path in backlog_files:
        if path.name == "000-template.md":
            continue
        data = parse_frontmatter(path)
        tasks.append(
            TaskRecord(
                identifier=_normalise_string(data.get("id")),
                title=_normalise_string(data.get("title")),
                status=_normalise_string(data.get("status"), "new"),
                priority=_normalise_string(data.get("priority"), "P3"),
                area=_normalise_string(data.get("area")),
                owner=_normalise_string(data.get("owner"), "unassigned"),
                size=_normalise_string(data.get("size"), "M"),
                gates=_ensure_list(data.get("gates")),
                relates_to=_ensure_list(data.get("relates_to")),
                blocked_on=_ensure_list(data.get("blocked_on")),
                links=_ensure_list(data.get("links")),
                path=path,
            )
        )
    return tasks


def compute_summary(tasks: Iterable[TaskRecord]) -> DashboardSummary:
    """Aggregate counts for the dashboard header."""

    total = 0
    by_status: Dict[str, int] = {}
    by_priority: Dict[str, int] = {}
    blocked = 0

    for task in tasks:
        total += 1
        by_status[task.status] = by_status.get(task.status, 0) + 1
        by_priority[task.priority] = by_priority.get(task.priority, 0) + 1
        if task.blocked_on:
            blocked += 1

    return DashboardSummary(total, by_status, by_priority, blocked)


def _priority_sort_key(priority: str) -> tuple[int, str]:
    return (PRIORITY_ORDER.get(priority, len(PRIORITY_ORDER)), priority)


def _task_sort_key(task: TaskRecord) -> tuple:
    return (
        STATUS_ORDER.get(task.status, len(STATUS_ORDER)),
        _priority_sort_key(task.priority),
        task.identifier,
    )


def render_html(
    tasks: Sequence[TaskRecord],
    summary: DashboardSummary,
    generated_at: datetime,
    include_archived: bool,
) -> str:
    """Render an accessible HTML dashboard."""

    timestamp = generated_at.astimezone(timezone.utc).isoformat()
    status_rows = "\n".join(
        f"<tr><th scope=\"row\">{html.escape(status or '—')}</th><td>{count}</td></tr>"
        for status, count in sorted(
            summary.by_status.items(), key=lambda item: STATUS_ORDER.get(item[0], len(STATUS_ORDER))
        )
    ) or "<tr><td colspan=\"2\">No tasks available.</td></tr>"

    priority_rows = "\n".join(
        f"<tr><th scope=\"row\">{html.escape(priority or '—')}</th><td>{count}</td></tr>"
        for priority, count in sorted(summary.by_priority.items(), key=lambda item: _priority_sort_key(item[0]))
    ) or "<tr><td colspan=\"2\">No priorities recorded.</td></tr>"

    def _format_list(values: Sequence[str]) -> str:
        if not values:
            return "—"
        return ", ".join(html.escape(value) for value in values)

    task_rows = "\n".join(
        """
        <tr class="task-row{blocked_class}">
            <td><span class="status status-{status}">{status}</span></td>
            <td>{priority}</td>
            <td>{identifier}</td>
            <td>{title}</td>
            <td>{owner}</td>
            <td>{area}</td>
            <td>{size}</td>
            <td>{blocked}</td>
            <td>{gates}</td>
            <td>{relates}</td>
            <td><a href="{link}" target="_blank" rel="noopener">{file}</a></td>
        </tr>
        """.strip().format(
            blocked_class=" blocked" if task.blocked_on else "",
            status=html.escape(task.status or "—"),
            priority=html.escape(task.priority or "—"),
            identifier=html.escape(task.identifier or "—"),
            title=html.escape(task.title or "—"),
            owner=html.escape(task.owner or "—"),
            area=html.escape(task.area or "—"),
            size=html.escape(task.size or "—"),
            blocked=_format_list(task.blocked_on),
            gates=_format_list(task.gates),
            relates=_format_list(task.relates_to),
            link=html.escape(str(task.relative_path).replace("\\", "/")),
            file=html.escape(str(task.relative_path)),
        )
        for task in sorted(tasks, key=_task_sort_key)
    ) or "<tr><td colspan=\"11\">No tasks found.</td></tr>"

    archived_note = (
        "<p class=\"note\">Archived tasks are included in this snapshot.</p>"
        if include_archived
        else ""
    )

    return f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"utf-8\">
  <title>Hybrid Workflow Task Dashboard</title>
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
  <style>
    :root {{
      color-scheme: light dark;
    }}
    body {{
      font-family: "Inter", "Segoe UI", sans-serif;
      margin: 2rem;
      background: #f9fbfd;
      color: #111827;
      line-height: 1.5;
    }}
    h1, h2 {{
      font-weight: 600;
      margin-bottom: 0.5rem;
    }}
    table {{
      border-collapse: collapse;
      width: 100%;
      margin-bottom: 2rem;
    }}
    th, td {{
      border: 1px solid #d1d5db;
      padding: 0.5rem 0.75rem;
      text-align: left;
      vertical-align: top;
    }}
    tbody tr:nth-child(even) {{
      background-color: #f3f4f6;
    }}
    caption {{
      font-weight: 600;
      margin-bottom: 0.5rem;
      text-align: left;
    }}
    .task-row.blocked {{
      background-color: #fef2f2;
    }}
    .status {{
      display: inline-block;
      padding: 0.1rem 0.5rem;
      border-radius: 999px;
      font-size: 0.85rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.03em;
    }}
    .status-new {{ background: #e0f2fe; color: #0369a1; }}
    .status-ready {{ background: #dcfce7; color: #166534; }}
    .status-in_progress {{ background: #ede9fe; color: #5b21b6; }}
    .status-review {{ background: #fef3c7; color: #92400e; }}
    .status-done {{ background: #d1fae5; color: #047857; }}
    .status-archived {{ background: #e5e7eb; color: #374151; }}
    .summary-grid {{
      display: grid;
      gap: 1rem;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      margin-bottom: 2rem;
    }}
    .summary-card {{
      background: #ffffff;
      border: 1px solid #e5e7eb;
      border-radius: 0.75rem;
      padding: 1rem;
      box-shadow: 0 1px 2px rgba(15, 23, 42, 0.08);
    }}
    .summary-card h3 {{
      margin-top: 0;
      margin-bottom: 0.25rem;
      font-size: 1rem;
    }}
    .summary-card ul {{
      margin: 0;
      padding-left: 1rem;
    }}
    .note {{
      font-size: 0.9rem;
      color: #4b5563;
    }}
    a {{
      color: #2563eb;
    }}
  </style>
</head>
<body>
  <header>
    <h1>Hybrid Workflow Task Dashboard</h1>
    <p>Generated at <time datetime=\"{timestamp}\">{timestamp}</time>.</p>
    {archived_note}
  </header>
  <section class=\"summary-grid\" aria-label=\"Task summary\">
    <div class=\"summary-card\">
      <h3>Total tasks</h3>
      <p><strong>{summary.total_tasks}</strong></p>
      <p>{summary.blocked_tasks} blocked task(s).</p>
    </div>
    <div class=\"summary-card\">
      <h3>By status</h3>
      <table>
        <tbody>
          {status_rows}
        </tbody>
      </table>
    </div>
    <div class=\"summary-card\">
      <h3>By priority</h3>
      <table>
        <tbody>
          {priority_rows}
        </tbody>
      </table>
    </div>
  </section>
  <section>
    <h2>Task details</h2>
    <table aria-describedby=\"dashboard-description\">
      <caption id=\"dashboard-description\">Sorted by status, priority, and identifier.</caption>
      <thead>
        <tr>
          <th scope=\"col\">Status</th>
          <th scope=\"col\">Priority</th>
          <th scope=\"col\">ID</th>
          <th scope=\"col\">Title</th>
          <th scope=\"col\">Owner</th>
          <th scope=\"col\">Area</th>
          <th scope=\"col\">Size</th>
          <th scope=\"col\">Blocked on</th>
          <th scope=\"col\">Gates</th>
          <th scope=\"col\">Relates to</th>
          <th scope=\"col\">File</th>
        </tr>
      </thead>
      <tbody>
        {task_rows}
      </tbody>
    </table>
  </section>
</body>
</html>
"""


def build_json_payload(
    tasks: Sequence[TaskRecord],
    summary: DashboardSummary,
    generated_at: datetime,
    include_archived: bool,
) -> Dict[str, object]:
    return {
        "generated_at": generated_at.astimezone(timezone.utc).isoformat(),
        "include_archived": include_archived,
        "summary": {
            "total_tasks": summary.total_tasks,
            "by_status": summary.by_status,
            "by_priority": summary.by_priority,
            "blocked_tasks": summary.blocked_tasks,
        },
        "tasks": [task.to_json() for task in tasks],
    }


def write_outputs(output_dir: Path, html_document: str, payload: Dict[str, object]) -> tuple[Path, Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    html_path = output_dir / "index.html"
    json_path = output_dir / "tasks.json"
    html_path.write_text(html_document, encoding="utf-8")
    json_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    return html_path, json_path


def _format_repo_relative(path: Path) -> str:
    """Return ``path`` relative to the repo when possible, otherwise absolute."""

    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path.resolve())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=REPO_ROOT / "hybrid_workflow" / "dashboard",
        help="Directory where dashboard files will be written (defaults to hybrid_workflow/dashboard).",
    )
    parser.add_argument(
        "--include-archived",
        action="store_true",
        help="Include tasks from hybrid_workflow/backlog/archive/ in the dashboard.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    tasks = load_tasks(include_archived=args.include_archived)
    summary = compute_summary(tasks)
    generated_at = datetime.now(timezone.utc)
    html_document = render_html(tasks, summary, generated_at, include_archived=args.include_archived)
    payload = build_json_payload(tasks, summary, generated_at, include_archived=args.include_archived)
    html_path, json_path = write_outputs(args.output_dir, html_document, payload)
    html_display = _format_repo_relative(html_path)
    json_display = _format_repo_relative(json_path)
    print(f"Dashboard written to {html_display} and {json_display}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
