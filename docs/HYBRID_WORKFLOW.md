# Hybrid Agentic Workflow

**Status:** ✅ Active

This document is the single source of truth for how humans and AI agents collaborate on the Test Engine. It merges the specialised roles defined in `agents/` with pragmatic engineering habits so contributors can ship reliable features quickly.

---

## 1. Principles

1. **Shared Context First.** Every session begins with the context pack: `README.md`, `docs/NAVIGATION.md`, this workflow, the roadmap, and the relevant backlog file.
2. **Plan Before You Code.** Convert requests into backlog items with explicit definition of done and link them from the roadmap.
3. **Tight Handoffs.** Every role leaves artefacts—notes, diffs, test results—so the next agent continues without guesswork.
4. **Docs-as-Code.** Behaviour changes, tests, and documentation updates ship in the same commit.
5. **Quality Gates Everywhere.** Build, test, benchmark, and documentation validation are mandatory before merge.

---

## 2. Roles and Responsibilities

| Phase | Primary Roles | Responsibilities |
| --- | --- | --- |
| Discovery | Product Manager, Agent Orchestrator | Clarify intent, scope backlog item, assemble context pack. |
| Architecture | Chief Architect, Research Scientist, Tech Lead | Confirm invariants, author ADRs, design APIs. |
| Implementation | Module Engineers (Rendering, Geometry, Physics, Runtime, Tools, etc.) | Build features, write tests, update docs. |
| Quality | QA/Test Engineer, Performance Engineer, Security Gate, Reviewer | Run validation suites, enforce Definition of Done, review diffs. |
| Release | Release Manager, Docs/DevRel | Tag releases, publish notes, update tutorials/examples. |

Every specialist follows the common guardrails in `agents/00-COMMON-GUARDRAILS.md` plus their role file.

---

## 3. Operating Cycle

### 3.1 Discover & Plan
1. Product Manager records the opportunity as a backlog item under [`docs/backlog/active/`](backlog/active/) using the template.
2. Agent Orchestrator gathers the latest context and assigns the work to the appropriate specialist(s).
3. If architectural changes are required, Chief Architect files an ADR before coding starts.

### 3.2 Execute
1. Engineer reviews: roadmap entry → backlog file → module README → relevant ADRs/tests.
2. Implement incrementally. Keep commits small and reversible.
3. Update docs and tests in parallel with code changes.
4. Capture telemetry/performance baselines when behaviour changes.

### 3.3 Validate
1. Run the standard commands:
   ```bash
   cmake --preset <debug|release|sanitize>
   cmake --build --preset <debug|release|sanitize>
   ctest --preset <debug|release|sanitize> --output-on-failure
   pytest python/tests scripts/tests
   python scripts/validate_docs.py
   ```
2. QA/Test Engineer adds regression coverage for discovered bugs.
3. Performance Engineer benchmarks hotspots when the backlog item lists performance acceptance criteria.

### 3.4 Review & Merge
1. Reviewer checks code style, documentation, invariants, and test evidence.
2. Security/Safety Gate audits dependencies if required by the backlog item.
3. Once approved, merge only after the roadmap, backlog entry, and module docs reflect the new state.

### 3.5 Release & Follow-up
1. Release Manager tags milestones, updates changelog, and distributes artefacts.
2. Docs/DevRel publishes tutorials/examples if new workflows were introduced.
3. Product Manager moves the backlog item to `backlog/archive/` and queues follow-ups if gaps remain.

---

## 4. Handoff Checklist

Before handing work to another role:
- Provide an updated backlog file with status, progress notes, and outstanding questions.
- Attach diffs, logs, benchmark outputs, or screenshots needed for the next step.
- Record blockers and mitigation options.
- Notify the Agent Orchestrator when ownership changes.

---

## 5. Document Map

| Artifact | Purpose |
| --- | --- |
| [`README.md`](../README.md) | Workspace snapshot and module health. |
| [`docs/NAVIGATION.md`](NAVIGATION.md) | Directory map and doc routing. |
| [`docs/ROADMAP.md`](ROADMAP.md) | Application-readiness plan with priority bands. |
| [`docs/backlog/`](backlog/) | Authoritative backlog items (active + archive). |
| [`docs/modules/<module>/README.md`](modules/) | Module-specific status, build/test instructions. |
| [`docs/specs/ADR-*.md`](specs/) | Architectural decision records. |
| [`agents/`](../agents) | Role definitions, templates, coordination guidance. |

---

## 6. Quality Gates (Definition of Done)

A backlog item is complete only when all boxes are checked:
- ✅ Build succeeds with zero warnings using relevant presets.
- ✅ Unit/integration tests cover new behaviour (≥85% on touched lines).
- ✅ Benchmarks meet documented thresholds; regressions explained or mitigated.
- ✅ Documentation updated (roadmap, backlog, module README, tutorials as needed).
- ✅ Telemetry, logging, and diagnostics remain coherent.
- ✅ PR description cites backlog ID(s) and test evidence.

---

## 7. Continuous Improvement

- Auto-Improver monitors friction points and proposes process refinements.
- Community Maintainer collects contributor feedback and updates onboarding.
- Schedule retrospectives after each roadmap phase to record lessons learned and adjust backlog priorities.

**Keep this workflow updated whenever roles or validation steps change.**
