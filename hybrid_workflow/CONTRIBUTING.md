# Contributing

## Code Style

- Language: C++20; treat warnings as errors across presets.
- Formatting: `clang-format` (see repository `.clang-format`).
- Lint: `clang-tidy` (wired through CI smoke jobs).
- Files: `snake_case` filenames; `PascalCase` types; `camelCase` methods and variables.

## Naming Conventions

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
| Markdown files             | `UPPER_SNAKE_CASE`                                            | `README.md`, `CONTRIBUTING.md`                      |
| Files & Directories        | lower-case with hyphens/underscores unless part of public API | `engine/scene/systems`, `python/engine3g/loader.py` |

## Project Layout

- Public headers → `engine/<module>/include/engine/<module>/`
- Sources → `engine/<module>/src/`
- Tests → `engine/<module>/tests/` + `python/tests/`, `scripts/tests/`
- Benchmarks → `engine/<module>/bench/`
- Demos/Tools → `engine/<module>/samples/`, `tools/examples/`

## C++ Guidelines

- Target **C++20**; avoid non-standard extensions.
- Use non-owning views (`std::string_view`, `std::span`) for shared data.
- Guard headers with `#pragma once` and minimal includes.
- Propagate recoverable failures with `engine::Result<T, ErrorCode>`; never throw from public API entry points.
- Wire new targets through existing module helpers (`engine_apply_module_defaults`) and keep CMake presets updated.

## Python Guidelines

- Require **Python 3.12+** with type annotations for public APIs.
- Structure reusable logic under `python/engine3g/` and curate exports in `__init__.py`.
- Prefer `pathlib.Path` over raw strings and keep imports side-effect free.
- Document public functions/classes with docstrings describing arguments, return values, and error semantics.

## Data-Oriented Defaults

- Prefer **SoA** in hot paths; avoid virtual dispatch inside tight loops.
- No global singletons; pass explicit context/handles.
- Keep subsystem interfaces narrow and deterministic.

## Build & Tooling

- CMake ≥ 3.20; use presets from `CMakePresets.json` / `scripts/build/presets/`.
- Optional SDKs: Vulkan SDK 1.3.x, CUDA toolkit, GLFW desktop dependencies.
- Profiling: Capture Tracy zones around profiled hot paths and commit configs alongside code.

## Testing

- Framework: GoogleTest + CTest orchestration; Python uses `pytest`.
- Coverage: target ≥80% where instrumentation exists; add regression coverage for every bug fix.
- Every public API change must add/adjust tests across native and Python bindings.

## Performance

- Framework: GoogleBenchmark and bespoke telemetry harnesses.
- Regressions > **2%** block merges unless waived with mitigation in task file + quality report.
- Store raw results in task evidence sections or `telemetry/`; link in PR descriptions.

## Git & PR Rules

- One PR per task; reference the **`hybrid_workflow/backlog/...`** file path in PR body.
- Branch naming: `feat|fix|refactor/NNN-kebab-title` aligned with backlog IDs.
- Keep diffs <400 LOC; split otherwise.
- Commit messages: imperative mood; mention roadmap/backlog IDs.

## Documentation

- Update **hybrid_workflow/README.md** and root docs if user-facing behavior changes.
- API changes → refresh module READMEs under `docs/modules/` and navigation tables.
- Run `python scripts/validate_docs.py` after doc edits to keep cross-links green.
- Keep task files updated with design decisions and evidence as work progresses.

## Security & Safety

- For file I/O, plugins, scripting, or network changes: follow safety checklist in the task file.
- Validate untrusted inputs and document threat model updates in ADRs or module docs.
- Run sanitizers when `safety` gate is specified: `cmake --preset linux-clang-debug-asan`

## CI Expectations

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

## Error Handling

- Follow [`docs/design/ERROR_HANDLING_MIGRATION.md`](../docs/design/ERROR_HANDLING_MIGRATION.md) for status/result patterns.
- Prefer `engine::Result<T, ErrorCode>` over exceptions in public APIs.
- Document error conditions in function/class headers.

## Review Checklist

Before requesting review, verify:

- [ ] Code conforms to naming and formatting conventions above.
- [ ] Tests cover new behavior and run cleanly via canonical build workflow.
- [ ] Documentation updates are complete (module READMEs, task file, cross-links validated).
- [ ] Architectural invariants remain intact or changes documented in ADRs.
- [ ] Task frontmatter reflects current status and all gates are addressed.
- [ ] Performance benchmarks captured if `perf` gate specified.
- [ ] Risks and deviations noted in task file and PR description.

---

> See **[AGENTS.md](./AGENTS.md)** for the full workflow and task lifecycle.

