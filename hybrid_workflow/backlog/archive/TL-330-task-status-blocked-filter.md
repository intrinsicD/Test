---
id: TL-330
title: Add blocked filter to task status CLI
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

# Task TL-330 — Add blocked filter to task status CLI

## Intent

Deliver a blocked-task filter for `hybrid_workflow/task_status.py` so contributors can quickly audit blockers during planning and status reviews.

---

## Context

**Current State:**
- `task_status.py` lists tasks and supports filters for status, priority, and area only.
- Blocked tasks are marked with a 🚫 suffix in the table but cannot be queried directly.
- Hybrid workflow guidance emphasises triaging blockers early, yet the CLI lacks parity with roadmap needs.

**Desired State:**
- Add a command-line flag to return only blocked tasks, working alongside existing filters.
- Extend quick reference docs so contributors know the flag exists.
- Back the change with unit coverage to prevent regressions.

**References:**
- `hybrid_workflow/AGENTS.md` — 7-step workflow and blocker escalation guidance.
- `hybrid_workflow/CONTRIBUTING.md` — coding standards for repository utilities.
- `hybrid_workflow/QUICK_REFERENCE.md` — canonical command reference to update.

---

## Design / Plan

### Constraints

- Maintain backward-compatible defaults: running the script without flags must continue to list all tasks.
- Reuse existing data-parsing helpers to avoid duplicating backlog metadata logic.
- Tests should not rely on repository task files to keep coverage hermetic.
- Documentation updates must keep roadmap/quick reference aligned with tooling behavior.

### API / Data Sketch

```python
# argparse wiring
parser.add_argument(
    "--blocked",
    action="store_true",
    help="Limit results to tasks with non-empty blocked_on metadata",
)

# filtering
filtered = filter_tasks(tasks, status=args.status, priority=args.priority,
                        area=args.area, blocked_only=args.blocked)
```

### Edge Cases & Failure Modes

- **No blocked tasks present:** Command should print "No tasks found." message from table helper.
- **Blocked filter combined with other filters:** Intersections should work (e.g., `--blocked --status in_progress`).
- **Malformed frontmatter:** Existing error handling prints a message and skips the file; no new behavior required.

### Test Plan

- **Unit Tests:**
  - Verify `filter_tasks(..., blocked_only=True)` returns only tasks with `blocked_on` entries.
  - Confirm `filter_tasks(..., blocked_only=False)` excludes blocked tasks while default preserves all.
  - Ensure CLI argument parser exposes the new flag.
- **Integration Tests:**
  - None required; unit coverage suffices for this script.
- **Documentation Validation:**
  - Run `pytest` for new tests.
  - Run `python scripts/validate_docs.py` after doc edits.

---

## Steps

1. [x] Draft backlog entry with intent, context, and plan.
2. [x] Implement `--blocked` filtering in `hybrid_workflow/task_status.py` with backwards compatibility.
3. [x] Add unit tests covering blocked filtering behavior.
4. [x] Update hybrid workflow quick reference to document the new flag.
5. [x] Run unit tests and documentation validator; record evidence below.
6. [x] Archive task file and reference in roadmap bundle.

---

## Evidence

### Test Results

```bash
$ pytest python/tests/test_hybrid_workflow_task_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 3 items

python/tests/test_hybrid_workflow_task_status.py ...                                                                     [100%]

====================================================== 3 passed in 0.02s =======================================================

$ python scripts/validate_docs.py
All documentation links resolved successfully.
```

**Test Summary:**
- Pytest: 3 passed
- Documentation validation: success

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [x] | QA/Test | Pytest command output |
| docs | [x] | Docs/DevRel | Documentation validator output |

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/ROADMAP.md`
- `hybrid_workflow/backlog/archive/TL-330-task-status-blocked-filter.md`

---

## Completion Checklist (Definition of Done)

- [x] Implementation complete with unit coverage
- [x] Tests passing locally
- [x] Documentation updated to reference new CLI option
- [x] Task archived with evidence and roadmap reference updated
- [x] Ready for PR with synchronized artefacts

---

## Result

**PR:** pending
**SHA:** pending
**Completion Date:** 2025-11-05

**Notes:**
- Flag can combine with existing filters for precise blocked-task queries.
- Future enhancement: add `--blocked` tri-state (only/unblocked/all) if teams request explicit unblocked view.

**Follow-ups:**

- [ ] None

---
