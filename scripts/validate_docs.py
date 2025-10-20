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
ROADMAP_ID_RE = re.compile(r"\b(?:AI|DC|RT|DI|BS|TI|PY|CC|MC)-\d+\b")
ROADMAP_LINK_TOKEN = "../../ROADMAP.md"
TODO_SECTION_RE = re.compile(r"## TODO / Next Steps(?P<body>.*?)(?:\n## |\Z)", re.S)
REQUIRED_SECTIONS = ("## Current State", "## Usage", "## TODO / Next Steps")


def _should_skip(target: str) -> bool:
    prefixes = ("http://", "https://", "mailto:", "#")
    return target.startswith(prefixes)


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

    if failures:
        print("Documentation link validation failed:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("All documentation links resolved successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
