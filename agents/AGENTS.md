# Multi-Agent Operating Manual

**Status**: ✅ Active  
**Last Updated**: 2025-10-24

---

## 🎯 Purpose

This manual explains how specialized agents collaborate on the Test Engine
project. Use it after the quick orientation in
[`AGENTS-INDEX.md`](AGENTS-INDEX.md) to understand coordination patterns,
hand-off expectations, and shared tooling across roles.

---

## 🚦 Entry Points

1. **Index** – Start every session with
   [`AGENTS-INDEX.md`](AGENTS-INDEX.md) to choose the appropriate role and
   gather the essential repository context.
2. **Quickstart** – Follow
   [`AGENTS-QUICKSTART.md`](AGENTS-QUICKSTART.md) for the condensed task
   lifecycle and standard build/test commands.
3. **Hybrid Workflow** – Review
   [`docs/HYBRID_WORKFLOW.md`](../docs/HYBRID_WORKFLOW.md) for the
   authoritative description of the hybrid process adopted across humans and
   AI agents.

Keep these three documents open while working; this manual expands on the
operational details they introduce.

---

## 🧭 Coordination Loop

All tasks progress through the loop below. The Agent Orchestrator owns the
transitions and ensures each role receives the necessary context.

```text
Roadmap / Task Request
    ↓
Product Manager → Task Card (docs/tasks/<MODULE>-<ID>.md)
    ↓
Agent Orchestrator → Context Pack → Specialist Role(s)
    ↓
Implementation → Quality Gates → Reviewer
    ↓
Release Manager → Changelog / Artifacts
```

### Context Packs

- Template: [`agents/TEMPLATES/CONTEXT_PACK.md`](TEMPLATES/CONTEXT_PACK.md)
- Minimum contents:
  - Problem summary and acceptance criteria
  - Relevant file list with excerpts
  - Build/test commands and expected presets
  - Links to roadmap items, ADRs, or active discussions
- Keep packs lean (≤ ~1.5k tokens) while covering everything the executing
  role needs. Update them whenever requirements change.

### Hand-Off Expectations

- **Product Manager → Orchestrator**: Task card created, roadmap references
  recorded, acceptance criteria explicit.
- **Orchestrator → Engineer**: Context pack delivered plus explicit success
  conditions.
- **Engineer → Quality Gates**: Implementation complete, tests/docs updated,
  outstanding questions captured in the PR description.
- **Reviewer → Release Manager**: Review checklist satisfied, follow-ups
  filed, release notes drafted.

Document unresolved questions directly in the context pack or task file so
later agents inherit the discussion trail.

---

## ✅ Quality Gates & Roles

| Gate | Primary Role | Expectations |
| ---- | ------------ | ------------ |
| Testing | [QA/Test Engineer (90)](90-QA-Test-Engineer.md) | Coverage ≥85% on touched lines, deterministic suites, failure reproduction steps documented |
| Performance | [Performance Engineer (80)](80-Performance-Engineer.md) | Benchmarks updated or reaffirmed, telemetry hooks validated, no >2% regression |
| Security & Safety | [Security Gate (15)](15-Security-Safety-Gate.md) | Sanitizers clean, dependency diffs reviewed, undefined behaviour audits complete |
| Documentation | [Docs/DevRel (95)](95-Docs-DevRel.md) | API docs and READMEs updated in same PR, examples compile |
| Review | [Reviewer (99)](99-Reviewer.md) | Definition of Done enforced, follow-ups assigned, PR template completed |
| Release | [Release Manager (98)](98-Release-Manager.md) | Version/tag decisions recorded, changelog entries merged, artifacts published |

Quality gates run in parallel once implementation stabilises. The executing
engineer coordinates with the Orchestrator to schedule specialised roles as
needed.

---

## 🛠️ Shared Tooling & Scripts

- **Build/Test Presets** – Use the `cmake --preset <name>` flow documented in
  [`README.md`](../README.md) and reinforced in the quickstart.
- **Documentation Validation** – Run
  `python scripts/validate_docs.py` before requesting review.
- **Tree Synchronisation** – Update the repository snapshot in
  [`../AGENTS.md`](../AGENTS.md) via `python scripts/update_agents_tree.py`
  whenever files move or new directories are introduced.
- **Telemetry Utilities** – Diagnostics and benchmarking scripts live under
  `scripts/diagnostics/`; reference the specific tool in the relevant task or
  context pack.

Record command outputs (or cite CI equivalents) in your PR description so the
next agent can reproduce your steps.

---

## 📚 Required References Per Task Type

| Task Type | Additional Documents |
| --------- | -------------------- |
| Feature / Enhancement | Module README (`docs/modules/<module>/README.md`), relevant ADRs (`docs/specs/ADR-*.md`), open tasks in `docs/tasks/` |
| Bug Fix | Failing test or reproduction script, error telemetry captures, regression checklist |
| Documentation | NAVIGATION.md entry for affected area, style guides (`CODING_STYLE.md`, module templates) |
| Infrastructure | Build preset definitions (`scripts/build/`), CI orchestration (`scripts/ci/`), tooling READMEs |

Keep an “Assumptions” section in your working notes (or PR description) and
reconcile with the Orchestrator if any assumption proves incorrect.

---

## 🔁 Continuous Improvement

- **Auto-Improver (14)** reviews workflow friction and proposes adjustments to
  templates, guardrails, or automation.
- **Community Maintainer (16)** aggregates feedback from human contributors
  and updates onboarding docs.
- Capture improvements or open questions in
  [`docs/AGENTIC_WORKFLOW_ENHANCEMENT.md`](../docs/AGENTIC_WORKFLOW_ENHANCEMENT.md)
  to maintain a historical record of workflow evolution.

---

## 📥 Support Channels

1. **Unclear scope or missing files?** – Ask the Agent Orchestrator for an
   updated context pack.
2. **Architecture ambiguity?** – Loop in the Chief Architect (20) and reference
   the relevant ADR.
3. **Documentation gaps?** – Coordinate with Docs/DevRel (95) and log follow-up
   tasks in `docs/tasks/`.
4. **Blocked by tooling?** – Escalate to Tools/Build/CI Engineer (70).

Always document the resolution path in the task card or PR description so other
contributors can trace decisions quickly.

---

**Remember:** Every change should leave the workflow clearer, safer, and more
traceable for the next agent.
