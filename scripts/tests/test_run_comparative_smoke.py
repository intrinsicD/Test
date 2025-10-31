from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).resolve().parents[1] / "ci" / "run_comparative_smoke.py"
SPEC = importlib.util.spec_from_file_location("run_comparative_smoke", MODULE_PATH)
assert SPEC and SPEC.loader
module = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = module
SPEC.loader.exec_module(module)


@pytest.mark.usefixtures("tmp_path")
def test_comparative_smoke(tmp_path: Path) -> None:
    repo_root = Path(__file__).resolve().parents[2]
    exit_code = module.main(["--workspace-root", str(repo_root)])
    assert exit_code == 0

    output_dir = tmp_path / "reports"
    exit_code = module.main(
        [
            "--workspace-root",
            str(repo_root),
            "--output-dir",
            str(output_dir),
        ]
    )
    assert exit_code == 0
    assert (output_dir / "comparative_summary.json").is_file()
    assert any((output_dir / "plots").glob("*.svg"))
