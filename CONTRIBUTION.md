# Contribution Guide

This document codifies the expectations for every change that lands in the repository.  It complements the workflow blueprint in [`AGENTS.md`](AGENTS.md) and the navigation index in [`docs/NAVIGATION.md`](docs/NAVIGATION.md).  Every contribution—whether code, documentation, or automation—must satisfy the requirements in this guide **and** the task-specific acceptance criteria tracked in the hybrid workflow backlog.

## 1. Core Principles

1. **Correctness.** Preserve behavioural invariants, respect validated algorithms, and keep telemetry or profiling budgets green.  Changes must be covered by tests or written reasoning that demonstrates the absence of regressions.
2. **Clarity.** Prefer self-documenting code, narrowly-scoped commits, and documentation updates that explain the rationale for new behaviour.
3. **Performance.** Maintain or improve performance characteristics.  Provide benchmark evidence when touching perf-sensitive code paths.
4. **Workflow Synchronisation.** Every behavioural change must ship alongside roadmap/backlog updates and documentation sync as described in the workflow blueprint.

## 2. Repository Hygiene

- Follow the hybrid workflow lifecycle documented in [`hybrid_workflow/AGENTS.md`](hybrid_workflow/AGENTS.md).  Capture task briefs, context packages, and quality reports for non-trivial work.
- Keep branch names in the form `feat/<id>-kebab-title`, `fix/<id>-kebab-title`, or `refactor/<id>-kebab-title`, matching the backlog identifier.
- Squash merge commits only after reviewers confirm tests and documentation are up to date.
- Commit messages should follow Conventional Commits semantics (`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`, `test:`) and mention the relevant backlog entry when applicable.

## 3. Coding Standards

### 3.1 C++

The engine targets **C++20** (or later when explicitly stated).  Apply the following rules across all modules:

- Prefer explicit, RAII-driven ownership.  Use `std::unique_ptr`, `std::shared_ptr`, or `engine::UniqueHandle` instead of raw pointers for lifetime management.
- Use `engine::Result<T>` and `engine::Status` for recoverable errors.  Reserve exceptions for catastrophic failures at module boundaries.
- Avoid silent failure paths.  Propagate errors with contextual logging via `ENGINE_ERROR`, `ENGINE_WARN`, `ENGINE_INFO`, or `ENGINE_DEBUG` depending on severity.
- Keep headers self-contained.  **Never** wrap `#include` directives in `try/catch` blocks.
- Maintain consistent naming: PascalCase for types, snake_case for variables/functions, SCREAMING_SNAKE_CASE for constants, and suffix member variables with `_` when required for disambiguation.
- Prefer `constexpr` and `const` for compile-time or immutable values.  Use `[[nodiscard]]` when ignoring return values would likely be a bug.
- Guard platform-specific code with the existing feature toggles (e.g. `ENGINE_ENABLE_RENDERING`, `ENGINE_ENABLE_ASSETS`).
- Keep translation units under 500 logical lines where possible; refactor into helpers or modules when files grow beyond that limit.
- Structure namespaces to mirror the directory layout.  Avoid anonymous namespaces in headers.

### 3.2 Python

- Target **Python 3.11+**.  Type annotations are mandatory for public functions.  Use `typing.Protocol` or `typing.TypedDict` when interfaces are shared across modules.
- Enforce [`black`](https://black.readthedocs.io/) formatting (line length 100) and [`isort`](https://pycqa.github.io/isort/) import ordering.  Run `python -m compileall` on touched modules when practical.
- Raise explicit exceptions rather than returning sentinel values.  Log with the repository’s structured logging utilities instead of `print()`.

### 3.3 JavaScript / Front-end Assets

- Use ES2020 modules.  Prefer functional React components with hooks; avoid legacy class components unless the module explicitly requires them.
- Keep styles in CSS modules or Tailwind utilities as prescribed by the owning module’s README.

## 4. Testing Requirements

Every change must include evidence that automated tests relevant to the modified modules have been executed.

- Run the canonical command stack from [`AGENTS.md`](AGENTS.md#05-quality-instrumentation):
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Record command outputs in the task brief or quality report.  When infrastructure (e.g. GL dependencies) is unavailable in CI, document the limitation and provide compensating validation (unit tests, mocks, or targeted tooling).
- Add or update unit tests in `engine/<module>/tests/`, `python/tests/`, or `scripts/tests/` whenever behaviour changes.  Do not weaken existing assertions without reviewer approval.
- For performance-critical changes, capture benchmark data with the utilities in `scripts/benchmarks/` and attach summaries to the quality report.

## 5. Documentation Standards

- Markdown files under `docs/` must follow the **UPPER_SNAKE_CASE.md** naming convention.  Reference them using relative links to keep the documentation tree portable.
- Update `docs/NAVIGATION.md` whenever new directories or major documents are introduced.  Maintain cross-links so the documentation validator passes.
- Keep module READMEs in `docs/modules/` aligned with the code: describe new public APIs, invariants, and integration steps.
- When removing or renaming files, update backlinks throughout the repository and run `python scripts/validate_docs.py` to ensure no broken references remain.

## 6. Logging and Telemetry

- Use the logging macros in `engine/core/log.hpp`.  Avoid spamming the log on per-frame paths; instead, aggregate counters or throttle output.
- Honour telemetry policies in [`design/TELEMETRY_SCHEMA.md`](docs/design/TELEMETRY_SCHEMA.md) and [`design/TELEMETRY_INSTRUMENTATION_GUIDE.md`](docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md).  Capture trace spans with Tracy (`PROFILE_SCOPE`) for performance-sensitive code.

## 7. Asset and Rendering Guidelines

- Synchronise asset schema changes with the caches in `engine/assets/`.  Update validators and serialization logic together.
- Rendering code must respect the frame-graph architecture.  Declare new passes in the appropriate module and document them in [`docs/modules/rendering/`](docs/modules/rendering/README.md).
- When integrating new backends or resource providers, extend the Research Baseline configuration (`engine/rendering/pipeline/research_baseline.hpp`) and update related docs.

## 8. Review Checklist

Before requesting review:

1. Code compiles (or build failure is understood and documented, e.g. missing external dependencies in the container image).
2. Tests and documentation validation pass locally.
3. New APIs are documented (header comments, READMEs, migration notes).
4. Telemetry and logging are updated to reflect new behaviours.
5. Task brief, context package, and quality report are linked in the PR description when required.

Reviewers will apply the checklist in [`docs/prompts/REVIEW_CHECKLIST.md`](docs/prompts/REVIEW_CHECKLIST.md).  Be prepared to justify deviations or request follow-up tasks.

## 9. Licensing and Third-Party Code

- Ensure third-party assets are tracked under `third_party/` with explicit license metadata.  Update SPDX headers when necessary.
- External contributions must include provenance: origin repository, commit hash, license, and any local modifications.

## 10. Communication

- Summarise work in the PR description, referencing backlog IDs and affected modules.
- Use the task brief decision log for important architectural choices or deviations.
- Escalate blockers through the channels defined in [`agents/ROLES.md`](agents/ROLES.md).

Adhering to this guide keeps the repository coherent, reviewable, and ready for downstream automation.  When in doubt, discuss changes in the task brief and propose updates to this document via the workflow described in [`AGENTS.md`](AGENTS.md#workflow-change-proposals).
