---
id: TL-350
title: Add link filter to hybrid workflow task status CLIs
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
  - hybrid_workflow/QUICK_REFERENCE.md
  - hybrid_workflow/IMPLEMENTATION_SUMMARY.md
  - hybrid_workflow/COMPLETE.md
---

# Task TL-350 — Add Link Filter to Hybrid Workflow Task Status CLIs

## Intent

Allow operators to filter backlog tasks by explicit `links:` metadata so PRs, ADRs, or
roadmap references can be surfaced deterministically without relying on free-form
keyword search.

---

## Context

**Current State:**
- `hybrid_workflow/task_status.py` and `scripts/workflow/report_hybrid_status.py` support
  filters for status, priority, area, gates, owners, and roadmap bundles plus keyword
  search that scans titles, owners, and blocker metadata.
- Contributors often encode references (e.g., `docs/ROADMAP.md`, ADR IDs, PR links) in
  the `links:` frontmatter list, but the CLIs cannot require specific link entries.
- Automation jobs that need to find tasks citing a document currently scrape JSON output
  and perform secondary filtering.

**Desired State:**
- Both CLIs expose a `--link` flag (repeatable and comma-separated) that matches the
  exact `links:` entries case-insensitively.
- The shared `filter_tasks()` helper enforces that all requested links exist on a task,
  preserving deterministic automation.
- Documentation surfaces the new filter alongside the other command examples so agents
  discover it quickly.

---

## Design / Plan

1. Extend `filter_tasks()` and `select_next_actions()` in `hybrid_workflow/task_status.py`
   to accept a list of required link identifiers and filter tasks accordingly.
2. Wire a new `--link` CLI argument for both `task_status.py` and
   `scripts/workflow/report_hybrid_status.py`, normalising comma-separated values.
3. Update the corresponding unit tests to cover parser plumbing and link filtering.
4. Refresh the workflow quick reference, implementation summary, and completion docs to
   document the new filter.
5. Capture roadmap/backlog updates referencing TL-350 under Bundle C.

---

## Steps

1. [x] Add `links` filtering support to `filter_tasks()`/`select_next_actions()` in
       `hybrid_workflow/task_status.py` plus CLI wiring and usage text.
2. [x] Update `scripts/workflow/report_hybrid_status.py` to accept `--link` and pass it
       through to the shared helpers.
3. [x] Extend the Python unit tests covering both CLIs with parser and filtering
       regression tests.
4. [x] Document the new command in `hybrid_workflow/QUICK_REFERENCE.md`,
       `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`, and `hybrid_workflow/COMPLETE.md` and
       record the task under Bundle C in both roadmaps.
5. [x] Archive TL-350 with test evidence below.

---

## Evidence

### Test Results

```bash
pytest python/tests/test_hybrid_workflow_task_status.py -q
pytest scripts/tests/test_report_hybrid_status.py -q
```

### Documentation

- Updated `hybrid_workflow/QUICK_REFERENCE.md`, `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`,
  and `hybrid_workflow/COMPLETE.md` with link-filter examples.
- Added TL-350 to Bundle C checklists in both `hybrid_workflow/ROADMAP.md` and
  `docs/ROADMAP.md`.

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`
- `hybrid_workflow/COMPLETE.md`
- `hybrid_workflow/ROADMAP.md`
- `docs/ROADMAP.md`
---
