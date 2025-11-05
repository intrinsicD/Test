---
id: TL-341
title: Add next-action summary to hybrid status reporter
status: done
priority: P2
area: workflow
size: S
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - "scripts/workflow/report_hybrid_status.py"
  - "scripts/tests/test_report_hybrid_status.py"
  - "hybrid_workflow/QUICK_REFERENCE.md"
---

# Task TL-341 — Add next-action summary to hybrid status reporter

## Intent

Expose a quick "next action" summary from `report_hybrid_status.py` so agents can immediately list the highest-priority ready tasks—or the top new tasks when nothing is ready—without manually filtering the backlog.

---

## Context

**Current State:**
- `scripts/workflow/report_hybrid_status.py` can filter by status/priority but requires multiple invocations to decide what to work on next.
- The hybrid workflow prioritises ready tasks first, then grooming new ones, yet there is no one-shot CLI flag to surface that ordering.
- Documentation (Quick Reference) references manual filtering only.

**Desired State:**
- A CLI option outputs the highest-priority ready tasks, sorted by priority and identifier.
- When no ready tasks exist, the tool falls back to new tasks so agents know what to groom.
- Documentation and automated tests cover the new behaviour.

**References:**
- `hybrid_workflow/AGENTS.md` — workflow policy emphasising ready-then-new selection.
- `scripts/tests/test_report_hybrid_status.py` — existing coverage for the reporter.

---

## Design / Plan

### Behaviour

- Extend CLI parsing with `--next-actions` (boolean flag) and `--limit N` (optional, default 5).
- When `--next-actions` is used:
  - Load all tasks (respecting `--include-archived`).
  - Filter for `status == "ready"`.
  - If none found, filter for `status == "new"`.
  - Sort by `(priority order, identifier)` using the existing ordering helpers.
  - Return only the top `limit` tasks.
  - Render using existing format (`table` or `json`).
- When flag absent, behaviour unchanged.

### Data Structures

- Reuse `STATUS_ORDER` for ordering statuses; introduce `PRIORITY_ORDER` map to ensure `P0` precedes `P1`, etc., regardless of lexical ordering.
- Provide helper `select_next_actions(tasks, limit) -> list[TaskMetadata]` for testability.

### Testing

- Unit tests in `scripts/tests/test_report_hybrid_status.py` covering:
  - Selection of ready tasks when available.
  - Fallback to new tasks when no ready tasks exist.
  - Enforcement of limit value.
  - JSON output integration path when using `--next-actions`.

### Documentation

- Update `hybrid_workflow/QUICK_REFERENCE.md` with CLI example demonstrating the flag for Step 1.
- Note the helper in `hybrid_workflow/ROADMAP.md` maintenance tips if needed.

---

## Steps

1. [x] Implement helper and CLI flag in `report_hybrid_status.py` with priority ordering.
2. [x] Update unit tests covering new behaviour and CLI parsing.
3. [x] Refresh Quick Reference (and other docs if necessary) with usage guidance.
4. [x] Run targeted tests (`pytest scripts/tests/test_report_hybrid_status.py`).
5. [x] Update Evidence section and move task to archive once complete.

---

## Evidence

### Test Results

```bash
pytest scripts/tests/test_report_hybrid_status.py
```

**Summary:** `pytest` suite for the status reporter passed, covering new selection helpers and limit validation (see chunk `5a2bcf`).

### Updated Files

- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/ROADMAP.md`

---

## Completion Checklist (Definition of Done)

- [x] CLI exposes `--next-actions` flag and limit argument with correct ordering.
- [x] Tests validate selection logic and CLI output.
- [x] Documentation references the new workflow helper.
- [x] Task archived with Evidence recorded.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)

