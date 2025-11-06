# AGENTS — Workflow Guide

**Mission.** Ship the highest-priority tasks safely, fast, and to spec while keeping docs/backlog/roadmap in sync.

## AI Agent Priority Stack

When working as an AI agent, prioritize in this order:

1. **Correctness** – preserve invariants and task acceptance criteria.
2. **Clarity** – maintain documentation, comments, and tests that explain why decisions were made.
3. **Performance** – ensure changes respect the existing profiling budgets and telemetry.
4. **Velocity** – prefer incremental, well-scoped tasks over sweeping refactors.

---

## Task Lifecycle

```
new → ready → in_progress → review → done → archived
```

Every task follows this deterministic progression. Status is tracked in task file frontmatter and synchronized with the ROADMAP.

---

## Workflow (7 Steps)

### 1. Select Task

- Start with [`docs/ROADMAP.md`](../docs/ROADMAP.md) and pick the highest-priority item marked `status: ready`.
- If nothing is ready, groom the top `new` backlog item under `hybrid_workflow/backlog/` and mark it `ready` once scoped.
- Check the task's `blocked_on` field to ensure dependencies are clear.

### 2. Plan & Design

- Open the task file (e.g. `hybrid_workflow/backlog/NNN-task-name.md`).
- Complete the **Design/Plan** section referencing:
  - Constraints from [`hybrid_workflow/CONTRIBUTING.md`](./CONTRIBUTING.md)
  - Module READMEs under `docs/modules/`
  - Relevant ADRs under `docs/specs/`
- **Load context systematically** using the Context Ladder (see below).
- For complex tasks, create separate artifacts using templates in `agents/TEMPLATES/`.

### 3. Implement

- Branch name: `feat|fix|refactor/NNN-kebab-title` (matches backlog ID).
- Follow coding standards in [`CONTRIBUTING.md`](./CONTRIBUTING.md).
- Keep commits reviewable (<400 LOC).
- Add Tracy zones to profiled hot paths.
- Update docs, presets, and datasets alongside code when behavior changes.
- Log decisions and rationale in the task file's **Steps** section.

**Use available tools** from `engine/tools/` to accelerate implementation:
- **Profiling:** Use `PROFILE_SCOPE` for performance-critical sections (see [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md))
- **Diagnostics:** Integrate `render_diagnostics()` for runtime visualization
- **Benchmarking:** Use benchmark runners for automated performance testing
- **UI Panels:** Register panels with `PanelRegistry` for editor features
- **Configuration:** Load experiment configs with `load_summary_from_json()`

See [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md) for quick examples and [`CONTRIBUTING.md`](./CONTRIBUTING.md) §Diagnostic Tools for integration patterns.

### 4. Test

Run the canonical test stack:

```bash
- **Generate profiler reports** for tasks with `perf` gate using `global_profiler().generate_report()`
- **Run benchmark automation** via `PrototypeHarnessBenchmarkRunner` or `ComparativeBenchmarkRunner`
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
See [`TOOLS_REFERENCE.md`](./TOOLS_REFERENCE.md) for evidence collection examples.

ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

- Capture outputs in the **Evidence** section of the task file.
- Record benchmark deltas and telemetry snapshots when quality gates require it.
- For complex validation, create a Quality Report using `agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md`.

### 5. Review

- Open a PR referencing the backlog file path.
- Ensure task frontmatter lists all required `gates:` (tests, perf, docs, safety, release).
- For complex tasks, attach task brief, context package, and quality report links.
- Flag outstanding risks/blockers in the PR description.

### 6. Complete

- Set `status: done` in the task file frontmatter.
- Move task file to `hybrid_workflow/backlog/archive/`.
- Update `docs/ROADMAP.md` checkboxes and refresh README/module snapshots as needed.
- Commit artifact updates and run `python scripts/update_agents_tree.py` if structure changed.

### 7. Docs Sync

- Validate cross-links via `python scripts/validate_docs.py`.
- Ensure navigation tables (`docs/NAVIGATION.md`) reflect new assets or behavior.
- Update module READMEs with API changes or new capabilities.

---

## Context Ladder

Load references in this deterministic order before touching code or docs. Capture findings in the task file's **Design/Plan** section.

1. [`README.md`](../README.md) – workspace snapshot and module health.
2. [`docs/NAVIGATION.md`](../docs/NAVIGATION.md) – documentation index and precedence rules.
3. [`docs/ROADMAP.md`](../docs/ROADMAP.md) – strategic initiatives and priority bands.
4. Task file under [`hybrid_workflow/backlog/`](backlog/) – acceptance criteria and metadata.
5. Module README(s) under [`docs/modules/`](../docs/modules/) – subsystem invariants.
6. Binding decisions under [`docs/specs/ADR-*.md`](../docs/specs/) plus relevant design notes.
7. Historical context in [`docs/reviews/`](../docs/reviews/) or [`docs/archive/`](../docs/archive/) when risk or precedent is unclear.

**For complex tasks:** Create a Context Package using `agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md` to document findings, open questions, and knowledge gaps.

---

## Quality Gates

Tasks specify required gates in frontmatter: `gates: [tests, perf, docs, safety, release]`

| Gate | Owner Role | Criteria | Evidence |
|------|------------|----------|----------|
| **tests** | QA/Test Specialist | Tests green; coverage adequate; regression tests added | Command outputs in task Evidence section |
| **perf** | Performance Engineer | Regressions ≤2% unless mitigation accepted; benchmarks captured | Benchmark summaries in task Evidence section |
| **docs** | Docs/DevRel | Documentation updated; cross-links valid; terminology consistent | Updated files listed; validation output |
| **safety** | Safety Reviewer | Sanitizers clean; security review complete; threat model updated | Sanitizer logs; security checklist |
| **release** | Release Manager | Changelog updated; packaging verified; deployment steps documented | Release notes; packaging evidence |

**Simple tasks:** Record evidence inline in the task file's **Evidence** section.  
**Complex tasks:** Create a separate Quality Report using the template.

---

## Role Coordination (Optional for Complex Tasks)

For tasks requiring multi-role coordination, refer to [`agents/ROLES.md`](../agents/ROLES.md) for:

- Role responsibilities matrix
- Escalation paths
- Conflict resolution procedures
- Sign-off requirements

**Simple tasks** can skip formal role assignment. **Complex tasks** should populate a role roster in a Task Brief.

---

## Blockers

- Add `blocked_on:` entries in task frontmatter when dependencies arise.
- Update task status to reflect the blocker.
- For urgent blockers, escalate to Agent Orchestrator (see `agents/ROLES.md`).
- Document the blocker in the task file's decision log.

---

## Documentation Integration Checklist

For every task, confirm and log in the task file:

1. **Roadmap Alignment:** Task metadata includes `relates_to:` linking to ROADMAP bundles.
2. **Module Documentation:** Module READMEs updated if behavior changes.
3. **Architecture Records:** ADRs amended or added when invariants shift.
4. **Navigation Update:** New documents registered in `docs/NAVIGATION.md`.
5. **Contribution Standards:** Relevant `CONTRIBUTING.md` sections cited.

---

## Guardrails

### Do Not:
- ❌ Introduce APIs contradicting ADRs without filing a replacement ADR.
- ❌ Merge changes until backlog status, documentation, and templates are updated together.
- ❌ Add dependencies without documenting installation/runtime implications.
- ❌ Skip tests for behavioral changes.

### Do:
- ✅ Keep tests in lockstep with changes (C++ under `engine/<module>/tests/`, Python under `python/tests/`).
- ✅ Cite sources (files, commands, telemetry) in task files and PRs.
- ✅ Update task frontmatter status as work progresses.
- ✅ Run validation before marking tasks done.

---

## Task File Structure

Every task uses structured frontmatter for automation:

```yaml
---
id: NNN
title: Short imperative title
status: new            # new | ready | in_progress | review | done | archived
priority: P1           # P0 | P1 | P2
area: rendering        # module/domain tag
size: M                # XS | S | M | L
owner: agent           # assigned owner
gates: [tests]         # required quality gates
relates_to: [bundle:A] # ROADMAP bundle tags
blocked_on: []         # dependencies
links: []              # PRs, ADRs, docs
---
```

See `hybrid_workflow/backlog/000-template.md` for the complete template.

---

## Workflow Variants by Task Complexity

### Simple Task (Single contributor, <2 days)
1. Use task file only
2. Follow 7 steps above
3. Record evidence inline
4. No separate artifacts needed

### Medium Task (Multiple contributors or >2 days)
1. Use task file + optional Task Brief
2. Populate role roster if needed
3. Create Context Package if research-heavy
4. Follow 7 steps with coordination notes

### Complex Task (Cross-module, high risk, >1 week)
1. Use full artifact suite:
   - Task Brief (`agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md`)
   - Context Package (`agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md`)
   - Quality Report (`agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md`)
2. Assign formal roles from `agents/ROLES.md`
3. Use phase checklists for coordination
4. Document escalation paths

---

## Maintenance

- Review this workflow quarterly and update when practices evolve.
- Keep workflow changes in sync with `CONTRIBUTING.md`, `ROADMAP.md`, and templates.
- Run `python scripts/update_agents_tree.py` after structural changes.
- Propose workflow improvements as backlog items tagged `workflow`.

---

> **Quick Start:** New to the workflow? Read this file top-to-bottom, then check the task template at `hybrid_workflow/backlog/000-template.md` and select your first task from `docs/ROADMAP.md`.

