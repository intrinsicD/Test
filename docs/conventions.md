# Conventions

This file distils the coding, testing, and documentation practices that complement `CODING_STYLE.md` and module-specific READMEs.

## C++ & Build
- Target C++20. Mirror the `clang-22`, `gcc-12`, or MSVC 19.3x configuration described in the root `AGENTS.md`.
- Use the CMake presets in `CMakePresets.json`. Add new ones under `scripts/build/` and document them here and in module READMEs.
- Prefer `engine_apply_module_defaults` helpers when wiring new targets so include paths stay scoped.
- Avoid exceptions for control flow; propagate errors through `Result<T, Error>` once `DC-004` lands.

## Python & Tooling
- Python utilities must support 3.12+. Declare dependencies in `python/requirements.txt` (add if missing) and update `python/README.md` when behaviour changes.
- Tests live under `python/tests/` or `scripts/tests/` and run via `pytest python/tests scripts/tests`.

## Testing Policy
- Every bugfix includes a regression test.
- C++ tests integrate with CTest. Register new suites in the owning module's `CMakeLists.txt` and ensure `ctest --preset <name>` passes.
- Benchmark updates belong in `docs/tasks/` acceptance criteria with captured numbers.
- For GPU/backend changes, add integration tests or validation layers that run in CI-compatible configurations.

## Documentation
- Use relative links (`[Example](specs/ADR-0003-runtime-frame-graph.md)`) to keep navigation working across contexts.
- Keep Markdown sections under ~120 lines per heading to improve retrieval quality.
- When creating a new doc, state purpose, scope, and authoritative references in the first paragraph.
- Synchronise roadmap identifiers between `docs/ROADMAP.md`, module roadmaps, and any ADR you edit.

## Review Expectations
- Summaries must cite the affected files and relevant docs.
- Call out any deviation from the invariants in [`docs/architecture.md`](architecture.md) and either provide a fix or open a task.
- Highlight follow-up work in `docs/tasks/` so reviewers can see the backlog context.

Following these conventions keeps the AI-assisted workflow predictable, reduces review churn, and ensures future contributors understand each decision.
