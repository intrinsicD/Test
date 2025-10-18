# Implementation + Self-Review Prompt

Use this prompt to run a full implementer plus independent reviewer loop for Test Engine changes. It replaces the legacy prompt
previously stored under `docs/prints/`.

## When to Use
- Developing a new feature or bug fix that affects the engine, runtime tooling, or documentation.
- Executing roadmap tasks from `docs/tasks/` that require both implementation and follow-up validation.
- Practising the dual-phase implementer/reviewer workflow prior to automating it in CI.

## Prerequisites Checklist
Before invoking the prompt, confirm:
- [ ] You have read [`../README.md`](../README.md) and [`../../README.md`](../../README.md) for the current workspace snapshot.
- [ ] The relevant task record(s) in [`../tasks/`](../tasks/) are open in another tab (e.g., `T-0104`, `T-0112`, `T-0115`,
      `T-0116`, `T-0117`).
- [ ] Module-specific READMEs and roadmaps in [`../modules/`](../modules/) are reviewed for impacted subsystems.
- [ ] Applicable specs or ADRs in [`../specs/`](../specs/) are identified.
- [ ] Build and validation presets from [`../../README.md#build--test-workflow`](../../README.md#build--test-workflow) are
      available in your environment.

## Prompt Template
```
You are operating in two sequential phases for the Test Engine repository:
  • Phase A — Implementer
  • Phase B — Independent Reviewer (treat Phase A output as authored by a teammate)

Do NOT disclose chain-of-thought or internal deliberations. Respond only with the requested artefacts, concise rationales, and
checklists.

**Global Guardrails**
- Security-first: exclude secrets; validate all external input; default to least privilege.
- Correctness: deterministic behaviour; explicit error handling via `engine::Result`; no silent failures; tests accompany every
  behaviour change.
- Maintainability: cohesive diffs; expressive naming; comments explain *why* and *how*; avoid duplication.
- Performance: respect hot paths (frame-graph, geometry kernels, physics solvers); avoid unbounded allocations.
- Observability: emit actionable logs/metrics without leaking PII/PHI; prefer structured logging.
- Documentation discipline: keep README/ROADMAP/spec entries in sync with the change set.
- Compliance: follow [`CODING_STYLE.md`](../../CODING_STYLE.md), [`docs/conventions.md`](../conventions.md), and
  [`docs/agents.md`](../agents.md).

**Repository Context**
- Review the subsystem overview in [`../../README.md`](../../README.md) and invariants in [`../architecture.md`](../architecture.md).
- Inspect the active backlog via [`../tasks/README.md`](../tasks/README.md) with emphasis on:
  - `T-0104 Runtime frame-graph integration`
  - `T-0112 Geometry/IO round-trip hardening`
  - `T-0113 Animation runtime skinning`
  - `T-0115 Assets async streaming MVP`
  - `T-0116 Rendering Vulkan resource translation`
  - `T-0117 Physics contact manifolds`
- Honour non-goals described in [`../design/async_streaming.md#non-goals`](../design/async_streaming.md#non-goals) unless a task
  explicitly lifts them.

**Tooling & Quality Gates**
- Build with CMake presets; run `cmake --preset <name>` and `cmake --build --preset <name>`.
- Execute tests: `ctest --preset <name>` and `pytest python/tests scripts/tests` when Python changes.
- Validate docs with `python scripts/validate_docs.py`.

==============================================================================
PHASE A — IMPLEMENTER

1. PRIORITIZE
   - Score each candidate task using the rubric below and select ONE to implement.
   - Rubric (0–5 each; sum scores; tie-breaker = earliest deadline, then lower effort):
     • Deadline/urgency
     • User/business impact (breadth × severity)
     • Unblocking power (dependency removal)
     • Risk of not doing (security, data loss, SLO breach)
     • Effort (inverse; 5 = trivial change)
     • Strategic alignment (roadmap/OKR fit)
   - Emit only the decision log requested by the output schema.

2. DESIGN
   - Capture problem statement, acceptance criteria, API contracts, data flow, invariants, migration/compatibility plan,
     security/performance considerations, and the test strategy.

3. IMPLEMENT
   - Provide unified diffs (`git` format) for code, tests, and documentation.
   - Keep changes cohesive, reversible, and aligned with the selected task.

4. VERIFY
   - List all build/test commands executed, expected outcomes, and any manual QA steps.

==============================================================================
PHASE B — INDEPENDENT REVIEWER

- Assume a new engineer is auditing Phase A deliverables.
- Apply the checklist:
  • Correctness — acceptance criteria, error paths, concurrency
  • Security — validation, secret handling, dependency risk
  • Compatibility — API/ABI stability, migrations
  • Quality & Style — naming, readability, documentation
  • Performance — hot-path regressions, Big-O concerns
  • Tests — coverage depth, negative cases, flake risk
  • Observability — logging/metrics hygiene
  • Docs — README/ROADMAP/spec updates present
- Suggest targeted follow-up patches when issues are discovered.

==============================================================================
OUTPUT SCHEMA (strict; no extra prose)

1. ## PRIORITY_DECISION
   - Selected Task: <title or ID>
   - Score Table:
     | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
     | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
   - Tie-break Rationale: <sentence, if applicable>
   - Decision Rationale (≤5 bullets)

2. ## DESIGN_BRIEF
   <problem, acceptance criteria, interfaces, data flow, invariants, compatibility/migrations, security/performance/edge cases,
    test plan>

3. ## PATCH
   <unified git diffs for all changed/added files>

4. ## TESTS
   <diffs for test files/fixtures>

5. ## DOCS
   <diffs for README/CHANGELOG/API docs or "N/A">

6. ## VERIFY
   <commands executed and expected outcomes>

7. ## REVIEW_FINDINGS
   <checklist-based review notes, highlighting issues>

8. ## REVIEW_PATCHES
   <additional diffs from the reviewer, or "N/A">

9. ## FINAL_PATCH
   <authoritative diffs after applying review patches>

10. ## FOLLOW_UP_TODOS
    - [ ] <near-term task> (owner: TBD, priority, rationale)
    - [ ] <observability/monitoring task>
    - [ ] <longer-term refactor/tech debt>
    - [ ] <documentation or runbook improvement>

==============================================================================
PROJECT STANDARDS (Test Engine)
- C++20: follow `CODING_STYLE.md`; prefer `engine::Result` over exceptions; document ownership semantics; enforce
  `[[nodiscard]]` on fallible APIs.
- Python: adhere to PEP 8/257; type annotate (PEP 484); format with `black`; lint with `ruff`; run `pytest`.
- Build: integrate new targets with CMake presets and CTest; avoid hard-coded absolute paths.
- Security: keep secrets out of the repo; use parameterised queries; validate file/network inputs.
- Observability: use structured logging; tag telemetry with frame IDs when instrumenting runtime paths.
```

