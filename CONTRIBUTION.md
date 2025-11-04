# Contribution Standards

This document centralises coding style, naming conventions, validation expectations, and documentation policies for the
Test Engine workspace. It supersedes the legacy `CODING_STYLE.md` and dispersed guardrails.

## Core Principles

1. **Correctness first** – Preserve architectural invariants defined in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
   and follow the workflow in [`AGENTS.md`](AGENTS.md).
2. **Traceability** – Every change cites roadmap/backlog items and updates the associated documentation.
3. **Reproducibility** – Use the central build workflow defined in [`AGENTS.md`](AGENTS.md) unless explicitly justified.

## Naming & Formatting

| Concern                    | Convention                                                    | Example                                             |
|----------------------------|---------------------------------------------------------------|-----------------------------------------------------|
| C++ Types / Classes        | `PascalCase`                                                  | `SurfaceMesh`, `RuntimeHost`                        |
| C++ Functions              | `PascalCase` for APIs, `snake_case` for internal helpers      | `CreateSceneGraph`, `build_registry()`              |
| C++ Variables              | `snake_case`                                                  | `vertex_count`, `frame_timer`                       |
| C++ Constants & Macros     | `UPPER_SNAKE_CASE`                                            | `MAX_ITERATIONS`, `ENGINE_RUNTIME_EXPORTS`          |
| Namespaces                 | lower-case, scoped by module                                  | `engine::scene`, `engine::geometry`                 |
| Python Modules & Functions | `snake_case`                                                  | `load_scene`, `compute_normals`                     |
| Python Classes             | `PascalCase`                                                  | `MeshLoader`, `TelemetryReport`                     |
| Python Constants           | `UPPER_SNAKE_CASE`                                            | `DEFAULT_TIMEOUT`                                   |
| Markdown files             | `UPPER_SNAKE_CASE`                                            | `README.md`, `CONTRIBUTION.md`                      |
| Files & Directories        | lower-case with hyphens/underscores unless part of public API | `engine/scene/systems`, `python/engine3g/loader.py` |

Format C++ sources with four spaces per indent and prefer brace-initialisation. Python must comply
with [PEP 8](https://peps.python.org/pep-0008/).

## C++ Guidelines

- Target **C++20**; avoid non-standard extensions.
- Place declarations under `engine/<module>/include/engine/<module>/` and implementations under `engine/<module>/src/`.
- Use non-owning views (`std::string_view`, `std::span`) for shared data.
- Guard headers with `#pragma once` and minimal includes.
- Propagate recoverable failures with `engine::Result<T, ErrorCode>`; never throw from public API entry points.
- Wire new targets through existing module helpers (`engine_apply_module_defaults`) and keep CMake presets updated.

## Python Guidelines

- Require **Python 3.12+** with type annotations for public APIs.
- Structure reusable logic under `python/engine3g/` and curate exports in `__init__.py`.
- Prefer `pathlib.Path` over raw strings and keep imports side-effect free.
- Document public functions/classes with docstrings describing arguments, return values, and error semantics.

## Testing & Quality

1. **C++** – Register tests with CTest under the owning module. Every bugfix includes a regression test.
2. **Python** – Add tests to `python/tests/` or `scripts/tests/`; run `pytest python/tests scripts/tests`.
3. **Documentation** – Update module READMEs, roadmap items, and ADRs alongside code. Validate links with
   `python scripts/validate_docs.py`.
4. **Benchmarks** – Record performance baselines and diffs in task briefs; regressions >2% require mitigation before
   approval.
5. **Error Handling** – Follow [`docs/design/ERROR_HANDLING_MIGRATION.md`](docs/design/ERROR_HANDLING_MIGRATION.md) for
   status/result patterns.

## Review Checklist

- [ ] Code conforms to the conventions above (naming, formatting, error handling).
- [ ] Tests cover new behaviour and run cleanly via the central build workflow.
- [ ] Documentation updates are complete and cross-linked in `docs/NAVIGATION.md` when necessary.
- [ ] Architectural invariants remain intact or changes are documented in ADRs.
- [ ] Risks, follow-ups, and deviations from standards are noted in the task brief and PR description.

Update this document whenever standards change. Reference its sections explicitly inside task briefs, context packages,
and quality reports.
