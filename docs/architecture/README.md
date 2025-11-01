# Architecture Visualisations

This directory houses generated artefacts that document module-level
relationships. The primary output is a Graphviz dependency graph that is
synchronised with the workspace CMake targets.

## Module Dependency Graph

- **Source:** [`module_dependency_graph.dot`](module_dependency_graph.dot)
- **Generator:** [`scripts/generate_dependency_graph.py`](../../scripts/generate_dependency_graph.py)

Regenerate the graph after changing module wiring or CMake dependencies:

```bash
python scripts/generate_dependency_graph.py \
    --output docs/architecture/module_dependency_graph.dot
```

The script emits Graphviz DOT by default. Install `graphviz` if you would like
to render images:

```bash
python scripts/generate_dependency_graph.py \
    --output docs/architecture/module_dependency_graph.svg
```

The generator fails fast when `dot` is unavailable so documentation updates
cannot silently skip the visualisation step.
