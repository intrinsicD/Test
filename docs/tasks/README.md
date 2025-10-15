# Task Records

Each file in this directory documents a sprint backlog or a focused piece of work. Tasks drive discussions with ChatGPT and pair programmers—link to them from PRs and commit messages whenever possible. Treat them as the actionable layer beneath the architecture improvement plan summarised in [`../ROADMAP.md`](../ROADMAP.md) and [`../README.md`](../README.md).

## How to Use Task Records

1. **Find the relevant file** before starting work. Sprint summaries follow the naming pattern `YYYY-MM-DD-sprint-XX.md`. Individual tickets use `T-####-short-title.md`.
2. **Verify acceptance criteria.** If something is unclear, add clarifying bullets before touching code.
3. **Update the checklist** as you complete deliverables. Keep benchmarks and metrics in the task file for future reference, and bubble materialised learnings back into the roadmap when they impact future priorities.
4. **Link supporting specs** (ADR, RFP) so reviewers can trace intent. Add cross-references from [`docs/architecture.md`](../architecture.md) when a decision introduces a new invariant.

## Index

- [`2025-02-17-sprint-06.md`](2025-02-17-sprint-06.md) – current sprint alignment.
- [`T-0104-runtime-frame-graph-integration.md`](T-0104-runtime-frame-graph-integration.md) – runtime/rendering bridge work.
- [`T-0112-geometry-io-roundtrip-hardening.md`](T-0112-geometry-io-roundtrip-hardening.md) – geometry/IO fidelity tasks.

Create new task files as work is planned. Archive completed tasks under a `done/` subdirectory if they remain valuable references.
