#!/usr/bin/env python3
"""Validate that documentation links resolve to files in the repository."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS_DIR = ROOT / "docs"
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def _should_skip(target: str) -> bool:
    prefixes = ("http://", "https://", "mailto:", "#")
    return target.startswith(prefixes)


def _validate_markdown(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    issues: list[str] = []
    for match in LINK_RE.finditer(text):
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

    if failures:
        print("Documentation link validation failed:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("All documentation links resolved successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
