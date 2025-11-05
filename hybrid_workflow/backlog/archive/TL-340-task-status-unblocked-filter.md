---
id: TL-340
title: Add unblocked filter to task status CLI
status: done
priority: P3
area: tools
size: S
owner: docs-automation
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links: []
---

# Task TL-340 — Add unblocked filter to task status CLI

## Intent

Extend `hybrid_workflow/task_status.py` with an explicit flag for surfacing
unblocked tasks so teams can rapidly assemble ready-to-execute work lists during
planning sessions.

---

## Context

**Current State:**
- CLI exposes `--blocked` filter (delivered in TL-330) but lacks parity for the
  complementary "show only unblocked" query.
- Contributors must visually parse the table or write custom grep commands to
  isolate unblocked items, slowing status reviews.
- Quick reference documentation only teaches the blocked workflow.

**Desired State:**
- Introduce a dedicated `--unblocked` flag that filters out tasks with blockers
  while composing cleanly with status/priority filters.
- Guard the new option with tests that exercise the parser and filtering logic.
- Update workflow quick reference so the capability is discoverable alongside
  the blocker commands.

**References:**
- `hybrid_workflow/task_status.py` — CLI implementation
- `python/tests/test_hybrid_workflow_task_status.py` — regression coverage
- `hybrid_workflow/QUICK_REFERENCE.md` — canonical command reference

---

## Design / Plan

### Constraints
- Preserve backwards-compatible defaults: no flags still lists all tasks.
- Keep blocker filtering mutually exclusive to prevent ambiguous combinations.
- Avoid duplicating parser wiring in tests by factoring out a helper.

### Implementation Outline
- Factor parser creation into `build_parser()` to share between CLI and tests.
- Add mutually exclusive `--blocked` / `--unblocked` flags mapped to the
  existing `filter_tasks` tri-state.
- Extend quick reference docs with unblocked command examples.
- Augment tests to assert parser behavior and filtering semantics.

### Test Plan
- **Unit:** verify `filter_tasks(..., blocked_only=False)` returns only
  unblocked tasks and that `build_parser()` recognises `--unblocked`.
- **Regression:** reuse existing coverage for blocked filtering to ensure no
  regressions.
- **Docs:** run `python scripts/validate_docs.py` after updating references.

---

## Steps

1. [x] Capture gap analysis and implementation outline in this task file.
2. [x] Implement `--unblocked` flag plus parser helper in `task_status.py`.
3. [x] Update tests to cover the parser and unblocked filtering behaviour.
4. [x] Document the new CLI usage in `QUICK_REFERENCE.md`.
5. [x] Run targeted `pytest` module and documentation validator.
6. [x] Archive task with evidence.

---

## Evidence

### Test Results

```bash
$ pytest python/tests/test_hybrid_workflow_task_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 4 items

python/tests/test_hybrid_workflow_task_status.py ....                                                                    [100%]

====================================================== 4 passed in 0.06s =======================================================

$ python scripts/validate_docs.py
All documentation links resolved successfully.
```

### Quality Gate Sign-offs
- `tests`: ✅ Covered by automated unit tests.
- `docs`: ✅ Quick reference updated and validator executed.

---

## Notes
- Future follow-up: surface the tri-state capability in `COMPLETE.md` command
  recipes if teams request it there as well.
