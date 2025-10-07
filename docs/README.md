# Documentation Hub

The `docs/` subtree aggregates written documentation for the engine project.

## Structure
- [`api/`](api/README.md) – High-level API references and module descriptions.
  - [Math Module](api/math.md)
  - [Geometry Module](api/geometry.md)
  - [Scene Module](api/scene.md)
- [`design/`](design/README.md) – Architectural sketches, diagrams, and exploratory design documents.
  - [Engine Architecture Overview](design/architecture.md)
- Validation utilities – `scripts/validate_docs.py` traverses Markdown files and verifies that relative links
  resolve inside the repository.

Each nested directory provides its own README for finer-grained navigation.

## Contributing to the documentation
- Update or create Markdown pages alongside any code change that affects the documented behaviour. Include
  links to the relevant headers or source files so reviewers can trace the implementation.
- Keep cross-links relative (e.g., `../../engine/...`) to ensure they remain valid when the repository is
  relocated.
- Before submitting a change, run `python scripts/validate_docs.py` from the repository root to make sure all
  documentation links resolve.

_Last updated: 2025-02-14_
