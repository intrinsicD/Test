---
id: TL-343
title: Task status CLI owner filter
status: done
priority: P3
area: tools
size: S
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - "hybrid_workflow/task_status.py"
  - "python/tests/test_hybrid_workflow_task_status.py"
---

# Task TL-343 — Task Status CLI Owner Filter

## Intent

Add an owner filter to `hybrid_workflow/task_status.py` so agents can quickly audit
backlog items assigned to a specific person or role.

---

## Context

**Current State:**
- The task status CLI supports status, priority, area, relates_to, and blocker filters.
- Agents rely on external tooling or manual scans to focus on their own queue.
- Documentation does not describe an owner-based query.

**Desired State:**
- CLI accepts `--owner` and returns only tasks owned by the supplied value.
- Unit tests cover the new filter and parser wiring.
- Workflow documentation highlights the owner query option alongside other filters.

**References:**
- [`hybrid_workflow/task_status.py`](../../task_status.py)
- [`python/tests/test_hybrid_workflow_task_status.py`](../../../python/tests/test_hybrid_workflow_task_status.py)
- [`hybrid_workflow/AGENTS.md`](../../AGENTS.md)

---

## Design / Plan

### Constraints

- Preserve backwards compatibility for existing filter combinations.
- Match owner comparisons to exact metadata strings (case-sensitive).
- Update documentation snapshots that advertise CLI usage examples.

### Approach

1. Extend `filter_tasks` and argument parsing to accept an optional owner filter.
2. Add unit tests covering owner filtering and parser behaviour.
3. Refresh workflow documentation to include owner query examples.
4. Archive this task with evidence once tests pass.

---

## Steps

1. [x] Implement owner filtering in `hybrid_workflow/task_status.py`.
2. [x] Add pytest coverage for owner filtering semantics.
3. [x] Update workflow documentation (Implementation Summary, Quick Reference, Complete guide).
4. [x] Record validation output and archive the task.

---

## Evidence

### Test Results

```bash
$ pytest python/tests/test_hybrid_workflow_task_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 10 items

python/tests/test_hybrid_workflow_task_status.py ..........                                                              [100%]

====================================================== 10 passed in 0.06s ======================================================
```

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/COMPLETE.md`
- `hybrid_workflow/backlog/archive/TL-343-task-status-owner-filter.md`
- `hybrid_workflow/ROADMAP.md`

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)

**Notes:**
- The owner filter mirrors the behaviour in `scripts.workflow.report_hybrid_status` for consistent tooling semantics.

**Follow-ups:**
- [ ] Evaluate fuzzy matching or alias support if future metadata introduces synonyms.

---
