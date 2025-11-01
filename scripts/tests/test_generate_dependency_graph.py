from __future__ import annotations

from importlib import import_module
from pathlib import Path
import sys

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

generate_dependency_graph = import_module("scripts.generate_dependency_graph")


def test_runtime_dependencies_include_key_modules() -> None:
    modules, dependencies = generate_dependency_graph.build_dependency_graph()

    assert "runtime" in modules
    expected = {
        "animation",
        "assets",
        "compute",
        "core",
        "geometry",
        "io",
        "math",
        "physics",
        "platform",
        "rendering",
        "scene",
    }
    assert expected.issubset(dependencies["runtime"])


def test_generate_dot_contains_labels_and_edges() -> None:
    dot = generate_dependency_graph.generate_dot()

    assert '"runtime" -> "animation";' in dot
    assert '"io" [label="IO"];' in dot
