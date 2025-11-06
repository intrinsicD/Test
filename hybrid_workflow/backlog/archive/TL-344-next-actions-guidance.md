---
id: TL-344
title: Next-actions guidance for empty ready queue
status: done
priority: P3
area: tooling
size: S
owner: docs-devrel
gates: [docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - "scripts/workflow/report_hybrid_status.py"
---

# Task TL-344 — Next-actions Guidance for Empty Ready Queue

## Intent

Ensure the hybrid workflow reporter explains how to proceed when no `ready` tasks
exist by pointing contributors to groom the highest-priority `new` item.

---

## Context

**Current State (before change):**
- `python -m scripts.workflow.report_hybrid_status --next-actions` emitted
  `No tasks matched the supplied filters.` when no `ready`/`new` tasks existed.
- The message did not reinforce the workflow instruction to groom the top `new`
  backlog item under `hybrid_workflow/backlog/`.
- Agents reviewing the dashboard received no inline reminder about the manual
  fallback and had to re-open `hybrid_workflow/AGENTS.md` for guidance.

**Desired State:**
- The reporter keeps returning a friendly message when nothing matches but also
  cites the fallback procedure mandated by the hybrid workflow guide.
- Contributors immediately know to groom a `new` backlog item and mark it
  `ready` once scoped, reducing churn during planning.
- Documentation captures the change so the roadmap and backlog stay synchronized.

**References:**
- [`hybrid_workflow/AGENTS.md`](../../AGENTS.md) — Step 1 fallback instructions.
- [`scripts/workflow/report_hybrid_status.py`](../../../scripts/workflow/report_hybrid_status.py)
  — CLI implementation updated in this task.
- [`scripts/tests/test_report_hybrid_status.py`](../../../scripts/tests/test_report_hybrid_status.py)
  — Regression coverage for reporter output.

---

## Design / Plan

### Constraints
- Preserve existing CLI behaviour and return codes for downstream automation.
- Avoid adding third-party dependencies; update formatting inline.
- Keep the message succinct enough for terminal output (<2 lines).

### Implementation Outline
- Extend `render_table` to append workflow guidance when no tasks match.
- Adjust reporter tests to assert the new explanatory text.
- Synchronise roadmap documentation with the completed task.

### Edge Cases & Validation
- Ensure the enhanced message renders identically for standard queries and
  `--next-actions` invocations.
- Confirm JSON output remains unchanged (message is table-only path).

---

## Steps

1. [x] 2025-06-05 — Scoped fallback guidance and drafted wording referencing
       `hybrid_workflow/AGENTS.md`.
2. [x] 2025-06-05 — Implemented reporter message update and refreshed tests.
3. [x] 2025-06-05 — Updated hybrid roadmap to record completion.

---

## Evidence

### Commands

```bash
pytest scripts/tests/test_report_hybrid_status.py
python scripts/validate_docs.py
```

Both commands passed after the change, confirming reporter behaviour and link
integrity.

---

## Completion Checklist

- [x] Roadmap updated (`hybrid_workflow/ROADMAP.md`, `docs/ROADMAP.md`).
- [x] Backlog archive entry captured with context and evidence.
- [x] Tests refreshed to cover messaging change.
---
