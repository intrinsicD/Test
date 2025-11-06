---
id: TL-346
title: Next-actions filter support in hybrid status reporter
status: done
priority: P3
area: tooling
size: S
owner: docs-devrel
gates: [tests]
relates_to: [bundle:C]
blocked_on: []
links:
  - "scripts/workflow/report_hybrid_status.py"
  - "scripts/tests/test_report_hybrid_status.py"
---

# Task TL-346 — Next-actions filter support in hybrid status reporter

## Intent

Enable contributors to narrow the `--next-actions` view in
`scripts.workflow.report_hybrid_status` by owner, priority, or roadmap bundle so
work can be triaged without scanning unrelated tasks.

---

## Context

### Context Ladder Notes — 2026-02-19

- [`README.md`](../../README.md) reiterates that workflow automation must stay in
  sync with roadmap priorities and encourages keeping the ready queue healthy.
- [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) directs agents to the hybrid
  workflow artefacts and scripts that automate backlog curation, confirming the
  reporter is the canonical entry point for next actions.
- [`docs/ROADMAP.md`](../../docs/ROADMAP.md) shows Bundle C as the home for
  workflow tooling follow-ups, signalling where to track this improvement.
- [`hybrid_workflow/AGENTS.md`](../AGENTS.md) instructs agents to groom the top
  `new` backlog item when the ready queue is empty—our current situation.
- [`hybrid_workflow/ROADMAP.md`](../ROADMAP.md) lists tooling automation tasks
  and highlights that the reporter drives the ready queue guidance.

### Current State

- `python -m scripts.workflow.report_hybrid_status --next-actions` ignores the
  `--owner`, `--priority`, and `--relates-to` filters even though they are
  exposed by the CLI.
- Triaging next actions for a specific owner or roadmap bundle requires manual
  scanning of the full ready queue output or running separate filtered commands.

### Desired State

- `--next-actions` honours optional filters so the highest-priority relevant
  tasks surface immediately.
- Fallback to `new` tasks still respects the filters when no `ready` items
  match.
- Regression coverage locks in the behaviour.

---

## Design / Plan

### Constraints

- Maintain backwards-compatible defaults (no filters yields existing behaviour).
- Reuse existing case-insensitive matching for `relates_to` tags.
- Avoid altering JSON output or other command paths.

### Implementation Outline

1. Extend `select_next_actions` to accept optional owner/priority/relates_to
   filters and apply them before selecting ready/new tasks.
2. Pass CLI filter values through when `--next-actions` is set.
3. Add regression tests covering filtered selection and fallback behaviour.
4. Update roadmap/task documentation to register the new backlog item.

### Validation Strategy

- `pytest scripts/tests/test_report_hybrid_status.py`
- Manual spot-check of `python -m scripts.workflow.report_hybrid_status
  --next-actions` with filters (documented in Evidence section).

---

## Steps

1. [x] 2026-02-19 — Groomed backlog entry, captured context ladder notes, and
       marked task ready for implementation.
2. [x] 2026-02-19 — Implemented filtered `--next-actions` selection in the reporter.
3. [x] 2026-02-19 — Extended reporter tests for owner/priority/relates_to filters.
4. [x] 2026-02-19 — Ran validation commands, captured evidence, and refreshed roadmap links.
5. [x] 2026-02-20 — Marked the task done, archived the record, and aligned roadmap references.

---

## Evidence

### Commands

```bash
$ pytest scripts/tests/test_report_hybrid_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 16 items

scripts/tests/test_report_hybrid_status.py ................                                                              [100%]

====================================================== 16 passed in 0.07s ======================================================

$ python -m scripts.workflow.report_hybrid_status --next-actions --owner docs-devrel
No tasks matched the supplied filters.
Tip: When the ready queue is empty, groom the highest-priority new backlog item under hybrid_workflow/backlog/ and mark it ready once scoped.
```

### Summary

- Pytest confirms reporter regression coverage for the new filter-aware next-actions selection.
- Manual CLI invocation demonstrates the command path honours the owner filter and retains workflow guidance when no tasks match.

---

## Completion Checklist (Definition of Done)

- [x] `--next-actions` respects owner/priority/relates_to filters, including
      fallback to `new` tasks when no ready items qualify.
- [x] Reporter tests cover the new behaviour.
- [x] Roadmap reflects the task and its status.
- [x] Evidence section captures validation commands with passing results.
- [x] Task status updated and archived when complete.

---
