#!/usr/bin/env python3
"""Static lint to catch legacy error-handling patterns (DC-004.3)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

DEFAULT_SCAN_ROOTS: tuple[str, ...] = ("engine",)
CHECK_EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ipp",
    ".inl",
    ".mm",
}


@dataclass
class CheckResult:
    success: bool
    messages: list[str]

    @property
    def needs_allowlist_update(self) -> bool:
        return any("allowlist" in message for message in self.messages) or any(
            "missing entirely" in message for message in self.messages
        )


def project_root_from_file(path: Path) -> Path:
    return path.resolve().parents[2]


def is_checked_file(path: Path) -> bool:
    return path.suffix.lower() in CHECK_EXTENSIONS


def collect_throw_sites(root: Path, subdirectories: Iterable[str]) -> dict[str, Counter[str]]:
    result: dict[str, Counter[str]] = {}
    for relative_dir in subdirectories:
        scan_root = root / relative_dir
        if not scan_root.exists():
            continue
        for file_path in scan_root.rglob("*"):
            if not file_path.is_file() or not is_checked_file(file_path):
                continue
            rel_path = file_path.relative_to(root).as_posix()
            counter: Counter[str] = Counter()
            try:
                contents = file_path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            for line in contents.splitlines():
                stripped = line.strip()
                if not stripped or stripped.startswith("//"):
                    continue
                if "throw" in stripped:
                    counter[stripped] += 1
            if counter:
                result[rel_path] = counter
    return result


def load_allowlist(path: Path) -> dict[str, Counter[str]]:
    if not path.exists():
        return {}
    data = json.loads(path.read_text(encoding="utf-8"))
    allowlist: dict[str, Counter[str]] = {}
    for rel_path, entries in data.items():
        allowlist[rel_path] = Counter(entries)
    return allowlist


def write_allowlist(path: Path, sites: dict[str, Counter[str]]) -> None:
    serializable: dict[str, dict[str, int]] = {}
    for rel_path in sorted(sites):
        counter = sites[rel_path]
        serializable[rel_path] = {text: counter[text] for text in sorted(counter)}
    path.write_text(json.dumps(serializable, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def diff_sites(actual: dict[str, Counter[str]], allowlist: dict[str, Counter[str]]) -> CheckResult:
    messages: list[str] = []
    success = True

    for rel_path in sorted(actual):
        current = actual[rel_path]
        allowed = allowlist.get(rel_path, Counter())
        new_entries = current - allowed
        if new_entries:
            success = False
            for text, count in sorted(new_entries.items()):
                messages.append(
                    f"{rel_path}: found {count} unauthorised throw statement(s): {text}"
                )
        missing_entries = allowed - current
        if missing_entries:
            success = False
            for text, count in sorted(missing_entries.items()):
                messages.append(
                    f"{rel_path}: allowlist missing {count} occurrence(s) currently recorded: {text}"
                )

    for rel_path in sorted(allowlist):
        if rel_path not in actual:
            success = False
            messages.append(
                f"{rel_path}: allowlisted file missing entirely; run with --update if removal is intentional"
            )

    return CheckResult(success=success, messages=messages)


def run_check(root: Path, allowlist_path: Path, subdirectories: Iterable[str]) -> CheckResult:
    sites = collect_throw_sites(root, subdirectories)
    allowlist = load_allowlist(allowlist_path)
    return diff_sites(sites, allowlist)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=project_root_from_file(Path(__file__)),
        help="Project root (defaults to repository root)",
    )
    parser.add_argument(
        "--allowlist",
        type=Path,
        default=Path(__file__).resolve().with_name("legacy_error_allowlist.json"),
        help="Path to the allowlist JSON file",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="Regenerate the allowlist from the current repository state",
    )
    parser.add_argument(
        "paths",
        nargs="*",
        default=list(DEFAULT_SCAN_ROOTS),
        help="Relative directories to scan (defaults to engine)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])
    root = args.root
    allowlist_path = args.allowlist
    subdirectories = tuple(args.paths) if args.paths else DEFAULT_SCAN_ROOTS

    if args.update:
        sites = collect_throw_sites(root, subdirectories)
        write_allowlist(allowlist_path, sites)
        print(f"Allowlist updated: {allowlist_path.as_posix()}")
        return 0

    result = run_check(root, allowlist_path, subdirectories)
    if not result.success:
        for message in result.messages:
            print(message)
        if result.needs_allowlist_update:
            print("Run this script with --update to refresh the allowlist after intentional changes.")
        else:
            print("Resolve the legacy throw statements or add targeted allowlist entries before retrying.")
        return 1

    print("No legacy error-handling violations detected.")
    return 0


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
