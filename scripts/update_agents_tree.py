"""Synchronise the repository hierarchy section in ``AGENTS.md``."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Iterable, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
AGENTS_PATH = REPO_ROOT / "AGENTS.md"
BEGIN_MARKER = "<!-- BEGIN GENERATED FILE TREE -->"
END_MARKER = "<!-- END GENERATED FILE TREE -->"

EXCLUDED_NAMES = {
    ".git",
    ".github",
    "build",
    "cmake-build-debug",
    "cmake-build-release",
    "out",
    "tmp",
    "venv",
}


def _should_skip(path: Path) -> bool:
    name = path.name
    if name.startswith('.'):
        return True
    return name in EXCLUDED_NAMES


def _iter_directory(path: Path) -> tuple[list[Path], list[Path]]:
    directories: list[Path] = []
    files: list[Path] = []
    for entry in path.iterdir():
        if _should_skip(entry):
            continue
        if entry.is_dir():
            directories.append(entry)
        else:
            files.append(entry)
    key = lambda candidate: candidate.name.lower()
    directories.sort(key=key)
    files.sort(key=key)
    return directories, files


def generate_tree(root: Path) -> list[str]:
    """Return the repository hierarchy as a list of display lines."""

    lines: list[str] = ["."]

    def _walk(current: Path, depth: int) -> None:
        directories, files = _iter_directory(current)
        indent = "    " * depth
        for file in files:
            lines.append(f"{indent}{file.name}")
        for directory in directories:
            lines.append(f"{indent}{directory.name}/")
            _walk(directory, depth + 1)

    _walk(root, 1)
    return lines


def render_tree_block(lines: Sequence[str]) -> str:
    """Return the fenced code block containing the hierarchy."""

    body = "\n".join(lines)
    return "```text\n" + body + "\n```"


def replace_tree_block(original: str, block: str) -> str:
    """Replace the generated block inside ``AGENTS.md`` contents."""

    before, marker, remainder = original.partition(BEGIN_MARKER)
    if not marker:
        raise ValueError("begin marker not found in AGENTS.md")
    _, end_marker, after = remainder.partition(END_MARKER)
    if not end_marker:
        raise ValueError("end marker not found in AGENTS.md")

    replacement = BEGIN_MARKER + "\n" + block + "\n" + END_MARKER

    suffix = after
    if after and not after.startswith("\n"):
        suffix = "\n" + after
    return before + replacement + suffix


def update_agents(check_only: bool = False) -> int:
    tree_lines = generate_tree(REPO_ROOT)
    block = render_tree_block(tree_lines)

    original = AGENTS_PATH.read_text(encoding="utf-8")
    updated = replace_tree_block(original, block)

    if updated == original:
        if check_only:
            return 0
        print("AGENTS.md already up to date.")
        return 0

    if check_only:
        print("AGENTS.md hierarchy is out of date.")
        return 1

    AGENTS_PATH.write_text(updated, encoding="utf-8")
    print("Updated AGENTS.md with current repository hierarchy.")
    return 0


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail if ``AGENTS.md`` is not synchronised without writing changes.",
    )
    return parser.parse_args(list(argv) if argv is not None else None)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)
    return update_agents(check_only=args.check)


if __name__ == "__main__":
    raise SystemExit(main())
