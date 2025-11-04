# Contributing

## Code Style
- Language: C++20; treat warnings as errors across presets.
- Formatting: `clang-format` (see repository `.clang-format`).
- Lint: `clang-tidy` (wired through CI smoke jobs).
- Files: `snake_case` filenames; `PascalCase` types; `camelCase` methods and variables.

## Project Layout
- Public headers → `engine/<module>/include/engine/<module>/`
- Sources → `engine/<module>/src/`
- Tests → `engine/<module>/tests/` + `python/tests/`, `scripts/tests/`
- Benchmarks → `engine/<module>/bench/`
- Demos/Tools → `engine/<module>/samples/`, `tools/examples/`

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
- Regressions > **2%** block merges unless waived with mitigation in task brief + quality report.
- Store raw results in `docs/backlog/<task>/` or task artefacts; link evidence in PR/task brief.

## Git & PR Rules
- One PR per task; reference the **`docs/backlog/...`** file path in PR body.
- Branch naming: `feat|fix|refactor/NNN-kebab-title` aligned with backlog IDs.
- Keep diffs <400 LOC; split otherwise.
- Commit messages: imperative mood; mention roadmap/backlog IDs.

## Docs
- Update **workflow/README.md** and root docs if user-facing behaviour changes.
- API changes → refresh module READMEs under `docs/modules/` and navigation tables.
- Run `python scripts/validate_docs.py` after doc edits to keep cross-links green.

## Security & Safety (if applicable)
- For file I/O, plugins, scripting, or network changes: follow safety checklist in the task brief.
- Validate untrusted inputs and document threat model updates in ADRs or module docs.

## CI Expectations
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug`
- `ctest --preset linux-gcc-debug`
- `pytest python/tests scripts/tests`
- `python scripts/validate_docs.py`

> See **[AGENTS.md](./AGENTS.md)** for the full agent workflow and artefact hand-offs.
