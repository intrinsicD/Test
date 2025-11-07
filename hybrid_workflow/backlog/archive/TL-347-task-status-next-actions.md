---
id: TL-347
title: Add next-actions helper to task status CLI
status: done
priority: P3
area: workflow
size: S
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - "hybrid_workflow/task_status.py"
  - "python/tests/test_hybrid_workflow_task_status.py"
  - "hybrid_workflow/QUICK_REFERENCE.md"
  - "hybrid_workflow/IMPLEMENTATION_SUMMARY.md"
  - "hybrid_workflow/COMPLETE.md"
---

# Task TL-347 — Add next-actions helper to task status CLI

## Intent

Surface the highest-priority ready tasks directly from `hybrid_workflow/task_status.py`,
falling back to new items when the ready queue is empty, so contributors can follow the
workflow guidance without invoking the higher-level reporting scripts.

---

## Context

**Current State:**
- The CLI lists and filters tasks but lacks a one-shot "what should I do next?" view.
- Documentation recommends the hybrid status reporter for next-actions queries, adding
  friction for quick terminal workflows.
- Existing automation (TL-330, TL-340, TL-343) already enhanced filtering and metadata
  parsing for the CLI.

**Desired State:**
- `task_status.py --next-actions` prints the top ready tasks, sorted by priority and ID.
- The command gracefully falls back to new tasks when no ready items exist and accepts
  owner/area/priority/blocker filters for targeted grooming.
- Documentation references the new helper alongside the other CLI examples.

**References:**
- [`hybrid_workflow/AGENTS.md`](../../AGENTS.md) — emphasises ready→new progression.
- [`hybrid_workflow/QUICK_REFERENCE.md`](../../QUICK_REFERENCE.md) — command cheatsheet.
- [`scripts/workflow/report_hybrid_status.py`](../../../scripts/workflow/report_hybrid_status.py) —
  existing next-actions implementation for the reporting module.

---

## Design / Plan

### Behaviour

- Introduce `--next-actions` and optional `--limit` to the CLI parser.
- Reuse existing filters (priority, area, owner, relates_to, blocked/unblocked) while
  ignoring `--status` because the helper enforces ready→new selection.
- Sort candidate tasks by priority rank and identifier for deterministic output.
- Reject non-positive limits with a helpful error message.

### Testing

- Extend `python/tests/test_hybrid_workflow_task_status.py` with unit tests covering:
  - Ready-task preference and priority ordering.
  - Fallback to new tasks when ready queue is empty.
  - Filter/limit enforcement and invalid limit rejection.
  - Parser wiring for the new flags.

### Documentation

- Update `hybrid_workflow/QUICK_REFERENCE.md`, `IMPLEMENTATION_SUMMARY.md`, and
  `COMPLETE.md` with example commands using `--next-actions`.

---

## Steps

1. [x] Implement `select_next_actions()` helper plus CLI flag handling in
   `hybrid_workflow/task_status.py`.
2. [x] Add regression coverage exercising selection, filters, and parser wiring.
3. [x] Refresh hybrid workflow documentation to surface the new CLI usage.
4. [x] Archive the task with evidence below.

---

## Evidence

### Test Results

```bash
$ pytest python/tests/test_hybrid_workflow_task_status.py -q
```

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`
- `hybrid_workflow/COMPLETE.md`

---

## Completion Checklist (Definition of Done)

- [x] CLI exposes `--next-actions`/`--limit` with ready→new ordering and filter support.
- [x] Tests validate selection, filters, and parser behaviour.
- [x] Documentation references the helper alongside other CLI commands.
- [x] Task archived with evidence recorded.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** 2026-04-24
