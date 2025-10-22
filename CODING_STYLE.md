# Coding Style Guide

This document captures the complete set of conventions for contributing to the modular 3G rendering and geometry processing engine. It consolidates coding standards, testing policies, documentation practices, and review expectations.

## General Principles

- Write self-documenting code and complement it with concise comments when behaviour is non-obvious.
- Keep public APIs minimal and focused. Prefer free functions in headers that forward to implementation files.
- Ensure every new feature includes build or usage instructions in the relevant documentation.
- Propagate recoverable failures with `engine::Result<T, ErrorCode>` and module-specific error codes instead of throwing exceptions from API entry points.
- Follow the architectural invariants documented in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

## C++ Guidelines

### Language & Compiler Standards
- Target **C++20** and avoid compiler extensions that are not part of the standard.
- Use modern toolchains: **Clang ≥22**, **GCC ≥12**, or **MSVC 19.3x** as described in `AGENTS.md`.
- Place declarations in headers under `engine/<module>/include` and implementations in `engine/<module>/src`.
- Expose C-compatible entry points for shared libraries using the `ENGINE_<MODULE>_EXPORTS` pattern so modules remain callable from Python and other languages.

### Code Organization & Style
- Prefer `std::string_view`, `std::span`, and other non-owning views instead of raw pointers when sharing data.
- Use `#pragma once` include guards and keep headers free of unnecessary includes.
- Format code with **four spaces** per indentation level and brace-initialisation for aggregates.
- Adopt consistent naming:
  - Classes and free functions: `CapitalCase`
  - Variables, member variables, and member functions: `snake_case`
  - Constants: `UPPER_CASE`
- Keep functions short and focused; extract helpers into unnamed namespaces in `.cpp` translation units when they are not part of the public API.

### Build System
- Use the CMake presets in `CMakePresets.json`. Add new ones under `scripts/build/` and document them in module READMEs.
- Prefer `engine_apply_module_defaults` helpers when wiring new targets so include paths stay scoped.
- When adding new libraries, update the corresponding `CMakeLists.txt` files to expose headers and shared exports consistently.

### Error Handling
- **Avoid exceptions for control flow.** Propagate errors through `engine::Result<T, Error>` as documented in [`docs/design/ERROR_HANDLING_MIGRATION.md`](docs/design/ERROR_HANDLING_MIGRATION.md).
- Use module-specific error codes (e.g., `GeometryIoErrorCode`, `AnimationIoResult<T>`).

---

## Python Guidelines

### Language & Version
- Target **Python 3.12+** and use modern typing features (PEP 604 unions, type annotations everywhere).
- Declare dependencies in `python/requirements.txt` and update `python/README.md` when behaviour changes.

### Code Organization
- Structure reusable logic into modules under `python/engine3g` and keep package exports curated in `__init__.py`.
- Prefer `pathlib.Path` over string path manipulation.
- Use `ctypes` or `cffi` for FFI bindings as appropriate.

### Style & Naming
- Apply [PEP 8](https://peps.python.org/pep-0008/) naming conventions:
  - Functions and variables: `snake_case`
  - Classes: `PascalCase`
  - Constants: `UPPER_CASE`
- Document public functions and classes with docstrings describing intent, arguments, and return values.
- Avoid hard-coded paths; expose configuration via environment variables or explicit arguments.

### Testing
- Tests live under `python/tests/` or `scripts/tests/` and run via `pytest python/tests scripts/tests`.
- Add usage examples or doctests when practical and keep imports side-effect free.

---

## Testing & Validation Standards

### C++ Testing
- Every bugfix includes a regression test.
- C++ tests integrate with **CTest**. Register new suites in the owning module's `CMakeLists.txt`.
- Ensure `cmake --build --preset <name>` and `ctest --preset <name>` pass before submitting changes.
- For GPU/backend changes, add integration tests or validation layers that run in CI-compatible configurations.

### Benchmarking
- Benchmark updates belong in `docs/tasks/` acceptance criteria with captured numbers.
- Use consistent hardware and configurations for reproducible results.
- Document baseline performance in module READMEs.

### Validation Workflow
1. `cmake --build --preset <preset>` — Build succeeds with zero warnings
2. `ctest --preset <preset>` — All C++ tests pass
3. `pytest python/tests scripts/tests` — All Python tests pass
4. `python scripts/validate_docs.py` — Documentation links are valid

---

## Documentation Standards

### General Principles
- Use **relative links** to keep navigation working across contexts:
  ```markdown
  [Example](docs/specs/ADR-0003-runtime-frame-graph.md)
  ```
- Keep Markdown sections under ~120 lines per heading to improve retrieval quality for AI agents.
- When creating a new document, state **purpose, scope, and authoritative references** in the first paragraph.

### Cross-Referencing
- Synchronize roadmap identifiers (e.g., `DC-004`, `AI-002`) between:
  - `docs/ROADMAP.md` (central roadmap)
  - `docs/modules/<name>/BACKLOG.md` (module backlogs)
  - `docs/specs/ADR-*.md` (architecture decision records)
  - Commit messages and PR descriptions

### Module Documentation
- Every module must have a `README.md` following the template in `docs/README_TEMPLATE.md`:
  1. **Purpose** — What the component does and how it relates to neighboring modules
  2. **Key APIs** — Code examples showing typical usage
  3. **Build & Test** — How to compile and run tests
  4. **Current Status** — Link to module backlog
  5. **Related Specs** — References to ADRs and design documents

### Documentation Maintenance
- Keep documentation in sync with implementation. Update relevant READMEs and design notes in the **same commit** as code changes.
- Run `python scripts/validate_docs.py` after editing to catch broken links.
- Cross-link from [`docs/NAVIGATION.md`](docs/NAVIGATION.md) when adding major new documents.

---

## Code Review Standards

### Submission Checklist
Before submitting for review:
- [ ] All affected README files updated with current behavior, dependencies, and status
- [ ] Code adheres to this style guide (naming, formatting, error handling)
- [ ] Complex algorithms have *why* and *how* comments, not just *what*
- [ ] All validation steps pass (build, tests, docs validation)
- [ ] No warnings or regressions introduced

### Review Expectations
- **Summaries must cite affected files and relevant docs** — Make it easy for reviewers to understand context.
- **Call out invariant deviations** — Any deviation from [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) invariants must be justified or fixed.
- **Highlight follow-up work** — Reference tasks in `docs/tasks/` so reviewers see the backlog context.
- **Link to ADRs** — When implementing architectural decisions, reference the relevant `docs/specs/ADR-*.md`.

### Review Quality Standards
1. Evaluate style, naming, and commentary quality against this guide.
2. Verify error handling follows `Result<T, Error>` patterns.
3. Check that tests cover new behavior and edge cases.
4. Ensure documentation updates are complete and accurate.
5. Validate that architectural invariants are preserved.

---

## Architectural Invariants

These invariants are defined in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and must be preserved:

### Deterministic Scheduler
Frame-graph compilation must be deterministic for identical inputs. Backends may add validation but cannot reorder resource transitions.

### Resource Ownership
Assets expose handles via `engine::headers`. Lifetime is reference-counted; releasing a handle must free GPU/CPU resources deterministically. See [`docs/design/RESOURCE_MANAGEMENT.md`](docs/design/RESOURCE_MANAGEMENT.md).

### Geometry Fidelity
Spatial structures (kd-tree, octree) must stay in sync with mesh/point-cloud mutations. All geometry changes update bounds and centroid data before publishing to other systems.

### Error Propagation
Use `engine::Result<T, ErrorCode>` for recoverable errors. Never throw exceptions from public API entry points. See [`docs/design/ERROR_HANDLING_MIGRATION.md`](docs/design/ERROR_HANDLING_MIGRATION.md).

### Documentation Discipline
Module READMEs are canonical for local behavior. Architectural shifts must also update [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and relevant ADRs in [`docs/specs/`](docs/specs/).

---

## Quick Reference

| Concern | Document |
|---------|----------|
| Overall architecture & data flow | [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) |
| Error handling patterns | [`docs/design/ERROR_HANDLING_MIGRATION.md`](docs/design/ERROR_HANDLING_MIGRATION.md) |
| Resource management | [`docs/design/RESOURCE_MANAGEMENT.md`](docs/design/RESOURCE_MANAGEMENT.md) |
| Telemetry & instrumentation | [`docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md) |
| Module documentation template | [`docs/README_TEMPLATE.md`](docs/README_TEMPLATE.md) |
| Active roadmap & backlog | [`docs/ROADMAP.md`](docs/ROADMAP.md) |
| AI agent workflow | [`AGENTS.md`](AGENTS.md) |

---

**Keep this guide current.** As workflows evolve, update this document so newcomers and AI agents can build, test, and extend the engine without surprises.

**Last updated:** 2025-10-22 (Consolidated from CONVENTIONS.md)

