# Developer Scripts

Automation helpers for building and validating the workspace live under `scripts/`.

## Subdirectories
- `build/` – Scripts and notes for local build workflows.
- `ci/` – Continuous integration helpers.

## Standalone utilities

- `validate_docs.py` – Checks every Markdown file under `docs/` and reports missing or out-of-repository links.
  Execute via `python scripts/validate_docs.py` before publishing documentation changes.

Top-level scripts complement the CMake build and documentation maintenance tasks.

_Last updated: 2025-02-14_
