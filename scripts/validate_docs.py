#!/usr/bin/env python3
"""Validate that documentation links resolve to files in the repository."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS_DIR = ROOT / "docs"
MODULES_DIR = DOCS_DIR / "modules"
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
CODE_BLOCK_RE = re.compile(r"```[\s\S]*?```", re.M)
ROADMAP_ID_RE = re.compile(
    r"\b(?:AI|AN|AS|BS|CC|CO|CR|DC|DI|GE|IO|MA|MC|PH|PL|PY|RE|RT|SC|TI|TL)-\d+\b"
)
ROADMAP_LINK_TOKEN = "../../ROADMAP.md"
TODO_SECTION_RE = re.compile(r"## TODO / Next Steps(?P<body>.*?)(?:\n## |\Z)", re.S)
REQUIRED_SECTIONS = ("## Current State", "## Usage", "## TODO / Next Steps")


def _should_skip(target: str) -> bool:
    prefixes = ("http://", "https://", "mailto:", "#")
    return target.startswith(prefixes)


def _split_table_row(row: str) -> list[str]:
    return [cell.strip() for cell in row.strip().strip("|").split("|")]


def _is_divider_row(cells: list[str]) -> bool:
    if not cells:
        return False
    for cell in cells:
        if not cell:
            continue
        if set(cell) - {"-", ":"}:
            return False
    return True


def _extract_active_roadmap_ids(text: str) -> set[str]:
    active: set[str] = set()
    table_lines: list[str] = []
    lines = text.splitlines()
    for line in lines + [""]:
        if line.startswith("|"):
            table_lines.append(line)
            continue

        if not table_lines:
            continue

        header_cells = _split_table_row(table_lines[0])
        lowered_header = [cell.lower() for cell in header_cells]
        status_index = lowered_header.index("status") if "status" in lowered_header else -1
        if status_index != -1:
            for row in table_lines[1:]:
                cells = _split_table_row(row)
                if not cells or _is_divider_row(cells):
                    continue
                if len(cells) <= status_index:
                    continue
                identifier = cells[0].strip("`")
                match = re.match(r"([A-Z]{2,}-\d+)", identifier)
                if not match:
                    continue
                status_cell = cells[status_index].strip()
                if status_cell.startswith("✅"):
                    continue
                active.add(match.group(1))

        table_lines = []

    return active


def _index_task_files(tasks_dir: Path) -> dict[str, list[Path]]:
    index: dict[str, list[Path]] = {}
    if not tasks_dir.exists():
        return index

    for path in tasks_dir.rglob("*.md"):
        if path.name.lower() == "readme.md":
            continue
        match = re.match(r"([A-Z]{2,}-\d+)", path.stem)
        if not match:
            continue
        index.setdefault(match.group(1), []).append(path)

    return index


def _validate_markdown(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    sanitized = CODE_BLOCK_RE.sub("", text)
    issues: list[str] = []
    for match in LINK_RE.finditer(sanitized):
        target = match.group(1).strip()
        if not target or _should_skip(target):
            continue
        link, _, _anchor = target.partition("#")
        if not link:
            continue
        resolved = (path.parent / link).resolve()
        try:
            resolved.relative_to(ROOT)
        except ValueError:
            issues.append(f"{path.relative_to(ROOT)} -> {target} (outside repository)")
            continue
        if not resolved.exists():
            issues.append(f"{path.relative_to(ROOT)} -> {target} (missing)")
    return issues


def main() -> int:
    if not DOCS_DIR.exists():
        print("docs/ directory not found", file=sys.stderr)
        return 1

    failures: list[str] = []
    for markdown in DOCS_DIR.rglob("*.md"):
        failures.extend(_validate_markdown(markdown))

    if MODULES_DIR.exists():
        for module_readme in sorted(MODULES_DIR.glob("*/README.md")):
            text = module_readme.read_text(encoding="utf-8")
            missing = [section for section in REQUIRED_SECTIONS if section not in text]
            if missing:
                failures.append(
                    f"{module_readme.relative_to(ROOT)} missing sections: {', '.join(missing)}"
                )
                continue

            todo_match = TODO_SECTION_RE.search(text)
            if not todo_match:
                failures.append(
                    f"{module_readme.relative_to(ROOT)} missing '## TODO / Next Steps' section"
                )
                continue

            todo_body = todo_match.group("body").strip()
            todo_bullets = [line for line in todo_body.splitlines() if line.lstrip().startswith("-")]
            if not todo_bullets:
                failures.append(
                    f"{module_readme.relative_to(ROOT)} TODO section missing bullet list"
                )

            if ROADMAP_LINK_TOKEN not in todo_body:
                failures.append(
                    f"{module_readme.relative_to(ROOT)} TODO section missing link to docs/ROADMAP.md"
                )

            if not ROADMAP_ID_RE.search(todo_body):
                failures.append(
                    f"{module_readme.relative_to(ROOT)} TODO section missing roadmap identifier"
                )

    roadmap_path = DOCS_DIR / "ROADMAP.md"
    readme_path = ROOT / "README.md"
    tasks_dir = DOCS_DIR / "tasks"

    if roadmap_path.exists():
        roadmap_text = roadmap_path.read_text(encoding="utf-8")
        roadmap_ids = set(ROADMAP_ID_RE.findall(roadmap_text))

        if readme_path.exists():
            readme_text = readme_path.read_text(encoding="utf-8")
            readme_ids = set(ROADMAP_ID_RE.findall(readme_text))
            missing_in_roadmap = sorted(readme_ids - roadmap_ids)
            if missing_in_roadmap:
                formatted = ", ".join(missing_in_roadmap)
                failures.append(
                    "README.md references IDs missing from docs/ROADMAP.md: "
                    f"{formatted}"
                )

        active_ids = _extract_active_roadmap_ids(roadmap_text)
        task_index = _index_task_files(tasks_dir)
        for identifier in sorted(active_ids):
            if identifier not in task_index:
                failures.append(
                    "docs/ROADMAP.md references active task "
                    f"{identifier} without matching docs/tasks/{identifier}-*.md"
                )

    if failures:
        print("Documentation link validation failed:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("All documentation links resolved successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
