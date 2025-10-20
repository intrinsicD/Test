from __future__ import annotations

from collections import Counter
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.append(str(ROOT))

from scripts.lint import error_handling


def test_diff_sites_reports_new_entries() -> None:
    actual = {"engine/foo.cpp": Counter({"throw std::runtime_error();": 1})}
    allowlist: dict[str, Counter[str]] = {}
    result = error_handling.diff_sites(actual, allowlist)
    assert not result.success
    assert any("unauthorised" in message for message in result.messages)


def test_collect_throw_sites_skips_comments(tmp_path: Path) -> None:
    source_dir = tmp_path / "engine"
    source_dir.mkdir()
    cpp_file = source_dir / "example.cpp"
    cpp_file.write_text(
        """
        // throw std::runtime_error("comment");
        void example() {
            throw std::runtime_error("active"); // inline comment mentioning throw
        }
        """,
        encoding="utf-8",
    )

    sites = error_handling.collect_throw_sites(tmp_path, ("engine",))
    assert sites == {
        "engine/example.cpp": Counter({"throw std::runtime_error(\"active\"); // inline comment mentioning throw": 1})
    }


def test_run_check_matches_repository_allowlist() -> None:
    allowlist_path = ROOT / "scripts" / "lint" / "legacy_error_allowlist.json"
    result = error_handling.run_check(ROOT, allowlist_path, ("engine",))
    assert result.success, "\n".join(result.messages)
