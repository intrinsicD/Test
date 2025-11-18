---
id: TL-351
title: Add blocked_on dependency filter to hybrid workflow task CLIs
status: done
priority: P3
area: workflow
size: XS
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - hybrid_workflow/task_status.py
  - python/tests/test_hybrid_workflow_task_status.py
  - scripts/workflow/report_hybrid_status.py
  - scripts/tests/test_report_hybrid_status.py
  - hybrid_workflow/README.md
  - hybrid_workflow/QUICK_REFERENCE.md
  - hybrid_workflow/IMPLEMENTATION_SUMMARY.md
  - hybrid_workflow/COMPLETE.md
  - hybrid_workflow/ROADMAP.md
  - docs/ROADMAP.md
---

# Task TL-351 — Add blocked_on dependency filter to hybrid workflow task CLIs

## Intent

Allow contributors and automation to query tasks that cite specific blockers by
surfacing a `--blocked-on` filter in both `hybrid_workflow/task_status.py` and
`scripts/workflow/report_hybrid_status.py`.

---

## Context

**Current State:**
- The CLIs can filter by `--blocked` / `--unblocked` to distinguish whether a
task has blockers, but they cannot require *which* dependency appears in the
`blocked_on` metadata.
- Product managers often need to answer "Which tasks are blocked by TL-310?"
without manually grepping backlog files or building ad-hoc search commands.
- Automation that triages dependencies currently has to parse JSON output and
post-process results, duplicating logic that could live in the shared filters.

**Desired State:**
- Both CLIs expose a repeatable `--blocked-on` flag (also supporting
comma-separated values) that requires each result to include all requested
dependencies, matched case-insensitively.
- `filter_tasks()` and `select_next_actions()` accept the new filter so the
feature works for table, summary, JSON, and next-action flows.
- Documentation and quick references show the new commands alongside the
existing blocker helpers.

---

## Design / Plan

1. Extend `filter_tasks()` to accept an optional `blocked_on` list and ensure it
   requires all requested dependencies (case-insensitive subset match).
2. Thread the new parameter through `select_next_actions()`, the CLI parser, and
   JSON rendering paths so both scripts behave consistently.
3. Update unit tests covering both CLIs with parser plumbing and filtering
   regression cases.
4. Refresh workflow documentation (README, quick reference, implementation
   summary, completion log) with usage examples, and log the task on both
   roadmaps under Bundle C.

---

## Steps

1. [x] Add `blocked_on` filtering support to `filter_tasks()` and
       `select_next_actions()` in `hybrid_workflow/task_status.py`, updating the
       CLI parser and usage block.
2. [x] Propagate the new argument through
       `scripts/workflow/report_hybrid_status.py` so JSON and table rendering
       honour the filter.
3. [x] Extend `python/tests/test_hybrid_workflow_task_status.py` and
       `scripts/tests/test_report_hybrid_status.py` with parser + filtering
       regression tests.
4. [x] Document the new commands in `README.md`, `QUICK_REFERENCE.md`,
       `IMPLEMENTATION_SUMMARY.md`, and `COMPLETE.md`, and record TL-351 under
       Bundle C in both roadmaps.
5. [x] Archive TL-351 with the evidence below.

---

## Evidence

### Test Results

```bash
pytest python/tests/test_hybrid_workflow_task_status.py -q
pytest scripts/tests/test_report_hybrid_status.py -q
```

### Documentation

- Added `--blocked-on` usage examples to the hybrid workflow README, quick
  reference, implementation summary, and completion log.
- Roadmaps (hybrid + docs) now list TL-351 under Bundle C.

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/README.md`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`
- `hybrid_workflow/COMPLETE.md`
- `hybrid_workflow/ROADMAP.md`
- `docs/ROADMAP.md`
- `hybrid_workflow/backlog/archive/TL-351-task-status-blocked-on-filter.md`
