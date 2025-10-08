# Documentation Hub

The `docs/` subtree stores architecture notes, API references, and other long-form documentation that accompanies the
engine codebase. Individual directories provide additional READMEs for drill-down context.

## Purpose

- Capture the intent and rationale for engine modules so implementation details remain discoverable.
- Provide API-level descriptions that guide users of the runtime and scripting layers.
- Centralise diagrams and design studies that influence ongoing development.

## Directory Structure

- [`api/`](api/README.md) – High-level API references and module descriptions.
  - [Math Module](api/math.md)
  - [Geometry Module](api/geometry.md)
  - [Scene Module](api/scene.md)
- [`design/`](design/README.md) – Architectural sketches, diagrams, and exploratory design documents.
  - [Engine Architecture Overview](design/architecture.md)

## Tooling and Dependencies

- **Python 3.12+** – Required to run the validation script.
- **`markdown` files** – Authored using UTF-8 encoding and GitHub-Flavoured Markdown conventions.
- Optional diagramming sources (e.g., `.drawio`, `.plantuml`) can be stored alongside the rendered artefacts with clear
  export instructions inside the respective documents.

## Build/Test Commands

- `python scripts/validate_docs.py` – Ensures relative links resolve within the repository.
- `pytest` – Execute from the repository root if documentation changes include Python code snippets that are tested in
  the Python suite.

## Contribution Checklist

1. Update or create Markdown pages alongside any behavioural change.
2. Reference relevant headers or source files so reviewers can trace the implementation.
3. Prefer relative links (e.g., `../../engine/...`) to keep documents relocatable.
4. Run `python scripts/validate_docs.py` before opening a pull request to avoid broken links.

_Last updated: 2025-02-14_
