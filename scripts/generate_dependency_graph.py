"""Generate a module dependency graph for the Test Engine workspace."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, Mapping

ROOT = Path(__file__).resolve().parents[1]
_CMAKE_FILE = ROOT / "CMakeLists.txt"
_ENGINE_DIR = ROOT / "engine"


class DependencyGraphError(RuntimeError):
    """Raised when dependency graph generation fails."""


def _load_module_names() -> list[str]:
    content = _CMAKE_FILE.read_text(encoding="utf-8")
    match = re.search(
        r"set\s*\(\s*ENGINE_MODULES\s*(?P<body>.*?)\)", content, re.DOTALL | re.IGNORECASE
    )
    if not match:
        raise DependencyGraphError("Unable to locate ENGINE_MODULES definition in CMakeLists.txt")

    tokens = re.findall(r"[A-Za-z0-9_]+", match.group("body"))
    if not tokens:
        raise DependencyGraphError("ENGINE_MODULES definition is empty")
    return tokens


def _normalise_target_name(raw: str, resolved_target: str) -> str:
    raw = raw.strip()
    if raw == resolved_target:
        return resolved_target
    if raw.startswith("${") and raw.endswith("}"):
        inner = raw[2:-1]
        if inner == "target_name":
            return resolved_target
    return raw


def _extract_target_name(contents: str, module: str) -> str:
    match = re.search(r"set\s*\(\s*target_name\s+([^\s)]+)", contents)
    if match:
        return match.group(1).strip()
    return f"engine_{module}"


def _parse_dependencies(modules: list[str]) -> dict[str, set[str]]:
    module_set = set(modules)
    dependencies: dict[str, set[str]] = {module: set() for module in modules}

    for module in modules:
        cmake_path = _ENGINE_DIR / module / "CMakeLists.txt"
        contents = cmake_path.read_text(encoding="utf-8")
        target_name = _extract_target_name(contents, module)

        for match in re.finditer(
            r"target_link_libraries\s*\(\s*([^\s)]+)\s*(?P<body>[^)]*)\)",
            contents,
            re.IGNORECASE | re.DOTALL,
        ):
            called_target = _normalise_target_name(match.group(1), target_name)
            if called_target != target_name:
                continue

            body = match.group("body")
            tokens = re.findall(r"[A-Za-z0-9_:+./-]+", body)
            for token in tokens:
                token_lower = token.lower()
                if token_lower in {"public", "private", "interface", "optimized", "debug"}:
                    continue
                if not token.startswith("engine_"):
                    continue
                module_candidate = token[len("engine_") :]
                if module_candidate in module_set:
                    if module_candidate == module:
                        continue
                    dependencies[module].add(module_candidate)

    return dependencies


_SPECIAL_LABELS = {"io": "IO"}


def _format_label(module: str) -> str:
    return _SPECIAL_LABELS.get(module, module.replace("_", " ").title())


def _render_dot(modules: Iterable[str], dependencies: Mapping[str, Iterable[str]]) -> str:
    lines: list[str] = [
        "digraph ModuleDependencies {",
        '    graph [rankdir=LR, fontsize=12];',
        '    node [shape=box, style="rounded,filled", fillcolor="#1f2933", fontcolor="#f9fafb", penwidth=1.2];',
        '    edge [color="#4b5563", penwidth=1.2, arrowsize=0.85];',
    ]

    for module in modules:
        label = _format_label(module)
        lines.append(f'    "{module}" [label="{label}"];')

    for module in modules:
        deps = sorted(set(dependencies.get(module, [])))
        for dep in deps:
            lines.append(f'    "{module}" -> "{dep}";')

    lines.append("}")
    return "\n".join(lines) + "\n"


def _write_output(dot_source: str, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    suffix = output_path.suffix.lower()

    if suffix in {".dot", ".gv"}:
        output_path.write_text(dot_source, encoding="utf-8")
        return

    if suffix in {".png", ".svg", ".pdf"}:
        dot_executable = shutil.which("dot")
        if not dot_executable:
            raise DependencyGraphError(
                "Graphviz 'dot' executable not found. Install graphviz or choose a .dot/.gv output."
            )

        fmt = suffix[1:]
        process = subprocess.run(
            [dot_executable, f"-T{fmt}", "-o", str(output_path)],
            input=dot_source.encode("utf-8"),
            check=False,
        )
        if process.returncode != 0:
            raise DependencyGraphError("Graphviz failed to render the dependency graph")
        return

    raise DependencyGraphError(f"Unsupported output extension: {suffix}")


def build_dependency_graph() -> tuple[list[str], dict[str, set[str]]]:
    modules = _load_module_names()
    dependencies = _parse_dependencies(modules)
    return modules, dependencies


def generate_dot() -> str:
    modules, dependencies = build_dependency_graph()
    return _render_dot(modules, dependencies)


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Generate the module dependency graph.")
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output path (supports .dot, .gv, .png, .svg, .pdf).",
    )

    args = parser.parse_args(argv)

    try:
        modules, dependencies = build_dependency_graph()
        dot_source = _render_dot(modules, dependencies)
        _write_output(dot_source, args.output)
    except DependencyGraphError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
