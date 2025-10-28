# Refactor Playbook Prompt

Use this prompt to plan and execute multi-step refactors in the Test Engine repository while preserving architectural
invariants, roadmap intent, and determinism.

## When to Use
- Large structural changes spanning multiple modules or build targets.
- Tasks tied to architecture improvement plan items (DC-*, AI-*, RT-*).
- Migrations that adjust public APIs, resource ownership, or error handling policies.
- Technical-debt paydowns that require staged rollouts and regression coverage.

## Prerequisites Checklist
- [ ] Map the work to a roadmap/task identifier in [`../ROADMAP.md`](../ROADMAP.md) or [`../archive/backlog/legacy/tasks/`](../archive/backlog/legacy/tasks/).
- [ ] Review relevant specs/ADRs in [`../specs/`](../specs/) and design notes in [`../design/`](../design/).
- [ ] Collect READMEs/ROADMAPs for impacted modules under [`../modules/`](../modules/).
- [ ] Confirm rollback strategy, migration guardrails, and observability expectations.
- [ ] Ensure build/test/formatting tooling is available (CMake presets, `ctest`, `pytest`, `python scripts/validate_docs.py`).

## Prompt Template
```
You are guiding a Test Engine refactor.

Do NOT reveal chain-of-thought. Provide only the requested artefacts with citations to files, specs, and tasks.

**Global Guardrails**
- Preserve invariants in `docs/ARCHITECTURE.md` and policies in `../../CODING_STYLE.md`.
- Maintain deterministic behaviour, resource lifetime safety, and structured error handling (`engine::Result<T, Error>`).
- Keep documentation, roadmap checklists, and tasks synchronised with the refactor plan.

**Scope Definition**
- Refactor Name / ID: {e.g., DC-004 migration, AI-001 follow-up}
- Modules & Targets: {list of modules/libraries/binaries}
- Existing Constraints: {performance budgets, ABI stability, platform parity}
- Rollback Strategy: {single commit revert, feature flag, branch}

**Phase A — Context Gathering**
1. Summarise the problem statement and desired end state.
2. Cite relevant specs, tasks, and module documentation.
3. Highlight current blockers or technical debt motivating the change.

**Phase B — Impact Analysis**
1. Enumerate affected modules, dependencies, and external integrations.
2. List architectural invariants touched (handle safety, determinism, telemetry, etc.).
3. Identify required ADR updates or new design records.
4. Note compatibility risks (API/ABI, binary formats, telemetry schemas).

**Phase C — Implementation Plan**
Produce ordered steps with acceptance criteria, including:
- Source changes grouped by module.
- Test additions/updates (unit, integration, benchmarks).
- Documentation updates (READMEs, roadmaps, specs, tasks).
- Migration support (adapters, feature flags, deprecation notes).
- Rollback checkpoints or branch strategy.

**Phase D — Validation Strategy**
Define how the refactor will be verified:
- CMake presets and commands to build/test.
- Telemetry/benchmark sampling requirements.
- Static analysis or linting (clang-tidy, formatters).
- Documentation validation (`python scripts/validate_docs.py`).

**Phase E — Execution Guidance**
1. Provide per-step implementation tips or gotchas.
2. Call out concurrency, performance, or memory hot paths.
3. Note observability hooks that must be updated.
4. Recommend commit sequencing or PR slicing.

**Phase F — Change Management**
1. Describe rollout plan (feature flags, staged deployment, compatibility window).
2. List communications (release notes, module owners, docs updates).
3. Capture follow-up tasks for deferred work.

==============================================================================
OUTPUT SCHEMA (strict)
1. ## SUMMARY — concise problem statement, desired end state, and scope.
2. ## CONTEXT — references to specs, tasks, and recent changes motivating the refactor.
3. ## IMPACT_ANALYSIS — modules, dependencies, invariants, compatibility risks.
4. ## PLAN — ordered step-by-step implementation plan with acceptance criteria.
5. ## VALIDATION — build/test/benchmark/telemetry plan.
6. ## EXECUTION_NOTES — tips, risks, sequencing guidance.
7. ## CHANGE_MANAGEMENT — rollout/rollback/comms plus documentation updates.
8. ## FOLLOW_UP — explicit tasks with owners, priorities, and links.
==============================================================================
PROJECT STANDARDS (Test Engine)
- Follow `CODING_STYLE.md`, structured logging policies, and observability guidelines.
- Use `engine::Result<T, Error>`; document ownership semantics and apply `[[nodiscard]]` to fallible APIs.
- Keep public headers minimal and hide implementation details within `src/` or unnamed namespaces.
- Run `cmake --build`, `ctest`, `pytest`, and `python scripts/validate_docs.py` for affected presets.
```

## Common Refactoring Patterns

### Module Dependency Inversion (e.g., DC-001)
1. Define stable interfaces in lower-level modules.
2. Move concrete implementations into plugins.
3. Introduce discovery/registration mechanisms with validation.
4. Update orchestrators to depend only on interfaces.
5. Add build flags so implementations remain optional.

### Error Handling Migration (e.g., DC-004)
1. Introduce `Result<T, Error>` wrappers and module-specific error enums.
2. Migrate leaf functions first, updating call sites incrementally.
3. Add regression tests for success and failure paths.
4. Document migration status and error codes in module READMEs.
5. Integrate validation with static analysis or CI scripts.

### Resource Lifetime Refactoring (e.g., AI-001)
1. Adopt `ResourcePool<T, Tag>` for lifetime-tracked handles.
2. Update caches to store typed handles and recycle generations deterministically.
3. Add debug validation and telemetry for stale handle attempts.
4. Extend tests/benchmarks to cover lifecycle edge cases.
5. Document ownership rules and telemetry signals.

## Example Refactor Checklist
```
## Refactor: Introduce Async Asset Streaming (AI-002)

### Context
- [ ] Read `docs/design/ASYNC_STREAMING.md`.
- [ ] Review `docs/modules/assets/README.md` and runtime integration docs.
- [ ] Confirm dependencies: AI-001, DC-001.

### Impact Analysis
- [ ] Modules affected: Assets, IO, Runtime, Core.
- [ ] Invariants preserved: generational handles, deterministic loading, telemetry coverage.
- [ ] Breaking changes: none (additive API).
- [ ] Dependencies: requires deterministic thread pool in Core.

### Implementation Plan
1. [ ] Add `IoThreadPool` under `engine/core/threading/`.
2. [ ] Implement `AssetLoadRequest` / `AssetLoadFuture` primitives.
3. [ ] Extend caches with `schedule_async()`.
4. [ ] Wire thread pool into `RuntimeHost::initialize()`.
5. [ ] Add integration tests with deterministic fixtures.
6. [ ] Document configuration and telemetry in module READMEs.

### Validation
- [ ] Unit tests cover request lifecycle, cancellation, failure.
- [ ] Integration tests demonstrate async load during runtime tick.
- [ ] Telemetry scripts report queue depth and worker utilisation.
- [ ] `python scripts/validate_docs.py` passes.

### Rollback Strategy
- [ ] Keep new primitives behind feature flag; revert runtime wiring if instability occurs.
- [ ] Preserve tests/telemetry scaffolding for future retries.
```

## Anti-Patterns to Avoid
- ❌ Mixing refactor scope with feature work.
- ❌ Modifying public APIs without migration guidance or deprecation window.
- ❌ Skipping test/documentation updates.
- ❌ Leaving TODO comments without corresponding task records.
- ❌ Introducing hidden dependencies or circular references.

## Post-Refactor Checklist
- [ ] Implementation plan steps completed.
- [ ] Tests/benchmarks updated and passing on all required presets.
- [ ] Documentation (READMEs, roadmaps, specs, tasks) updated.
- [ ] Telemetry/observability hooks validated.
- [ ] Rollback instructions documented and verified.
- [ ] Follow-up tasks created for deferred work.
- [ ] Performance baselines recorded if applicable.

## Integration with the Architecture Improvement Plan
1. Start from the canonical roadmap entry (`../ROADMAP.md#architecture-improvement-plan`).
2. Track dependencies and prerequisites among initiatives.
3. Update roadmap tables and module READMEs as checkpoints are met.
4. Synchronise task files (`docs/backlog/active/T-XXXX-*.md`) with execution notes.
5. Reflect milestone status in the root `README.md` snapshot when major work lands.
