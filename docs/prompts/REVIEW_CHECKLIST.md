# Code Review Checklist Prompt

Use this prompt to perform consistent, high-signal reviews for Test Engine pull requests and follow-up audits.

## When to Use
- Evaluating any pull request before merge.
- Post-merge audits when architectural drift is suspected.
- Release readiness passes that require comprehensive verification.

## Prerequisites Checklist
- [ ] Read [`../NAVIGATION.md`](../NAVIGATION.md) for reviewer expectations and documentation structure.
- [ ] Refresh invariants in [`../ARCHITECTURE.md`](../ARCHITECTURE.md), workflow phases in [`../../AGENTS.md`](../../AGENTS.md), and coding/testing standards in
      [`../../CONTRIBUTION.md`](../../CONTRIBUTION.md).
- [ ] Open module READMEs/ROADMAPs for affected subsystems.
- [ ] Locate referenced tasks in [`../../hybrid_workflow/backlog/`](../../hybrid_workflow/backlog/) (verify gates and owners in YAML frontmatter) and consult [`../archive/backlog/legacy/tasks/`](../archive/backlog/legacy/tasks/) plus archived prints when historical context is required.
- [ ] Ensure access to build/test tooling to reproduce results if needed.

## Prompt Template
```
You are performing a Test Engine code review.

Do NOT reveal chain-of-thought. Provide only the requested sections with citations to files, specs, and commands.

**Global Guardrails**
- Prioritise correctness, determinism, and architecture invariants.
- Flag missing documentation, telemetry, or tests with actionable guidance.
- Reference roadmap/task IDs when identifying scope or follow-ups.

**Phase A — Context Gathering**
1. Summarise the change intent from PR description, commits, and linked tasks.
2. Identify affected modules, specs, and roadmap initiatives.
3. Capture build/test commands reported by the author.

**Phase B — Change Analysis**
1. Group modifications by module.
2. Highlight API changes, resource ownership adjustments, or new dependencies.
3. Note migrations, feature flags, or compatibility considerations.

**Phase C — Invariant Verification**
Evaluate the change against repository invariants (check each box with supporting evidence or cite violations).

### Resource Management (`AI-001`)
- [ ] New resources use `ResourcePool` with generational handles.
- [ ] Handles validated before dereference; debug checks in place.
- [ ] Ownership semantics documented.

### Error Handling (`DC-004`)
- [ ] Public APIs return `Result<T, Error>` for recoverable failures.
- [ ] Module-specific error enums used; functions marked `[[nodiscard]]`.
- [ ] Error paths covered by tests.

### Module Boundaries & Determinism
- [ ] No circular dependencies or header leakage.
- [ ] Frame-graph transitions and queue affinity explicit.
- [ ] Hot paths avoid new allocations or non-deterministic behaviour.

### Documentation Discipline
- [ ] Module READMEs/ROADMAPs updated.
- [ ] Central roadmap/root README adjustments applied if scope impacts status tables.
- [ ] Specs/tasks updated or created as needed.

**Phase D — Quality & Testing**
1. Evaluate style, naming, and commentary quality against `CONTRIBUTION.md`.
2. Confirm tests cover new behaviour, regressions, and negative cases.
3. Verify reported commands (`cmake --build`, `ctest`, `pytest`, `python scripts/validate_docs.py`) are appropriate.

**Phase E — Observability & Performance**
1. Ensure telemetry/logging additions follow schema (`CC-001`) and avoid leaking sensitive data.
2. Confirm performance expectations (geometry/physics hot loops, memory footprint) maintained or benchmarked.
3. Check concurrency/thread-safety considerations for async code.

**Phase F — Findings & Verdict**
1. Classify issues as 🔴 Critical, ⚠️ Warning, 💡 Suggestion.
2. Provide remediation guidance or follow-up tasks for each item.
3. Record documentation status and required additional tests.
4. State final verdict (✅ Approve, 🔄 Request Changes, 💬 Comment).

==============================================================================
OUTPUT SCHEMA (strict)
1. ## SUMMARY — purpose of the change and overall assessment.
2. ## ARCHITECTURAL_IMPACT — affected modules, invariants, and roadmap alignment.
3. ## FINDINGS
   - ### Critical Issues 🔴
   - ### Warnings ⚠️
   - ### Suggestions 💡
4. ## DOCUMENTATION_STATUS — checklist of files updated or missing.
5. ## TEST_COVERAGE — commands executed/required and notes on gaps.
6. ## FOLLOW_UP_WORK — tasks to file or areas to monitor.
7. ## VERDICT — final decision with justification.
==============================================================================
PROJECT STANDARDS (Test Engine)
- Enforce `engine::Result` patterns, deterministic behaviour, and handle safety.
- Require tests for new behaviour/regressions and documentation alignment.
- Cite relevant specs, tasks, and docs when raising issues.
```

## Troubleshooting Guides

### Missing Tests
**Symptom:** Behaviour change lacks regression coverage.
**Resolution:**
1. Add unit tests under `engine/<module>/tests/`.
2. Register target in the owning CMakeLists file.
3. Add integration tests for cross-module flows.
4. Document strategy in PR/task notes.

### Breaking Change Without Migration Plan
**Symptom:** Public API modified without backward compatibility.
**Resolution:**
1. Restore/deprecate previous API alongside new version.
2. Publish migration guidance in module README/spec.
3. Coordinate with runtime bindings or tooling where applicable.
4. Update roadmap/task records describing rollout.

### Architectural Invariant Violation
**Symptom:** Change bypasses established invariant (e.g., runtime depends on concrete backend).
**Resolution:**
1. Reference relevant ADR/roadmap item documenting the invariant.
2. Propose alternative aligned with architecture (e.g., plugin interface, validation).
3. File follow-up task if remediation exceeds PR scope.

## Review Workflow Reminders
1. Confirm PR references a task (`hybrid_workflow/backlog/<ID>-*.md`) and acceptance criteria (gates and owners defined in YAML frontmatter).
2. Review documentation updates for affected modules and central artefacts.
3. Execute targeted tests locally when behaviour is complex or determinism is at risk.
4. Record findings in `docs/reviews/` if performing post-merge audits.
