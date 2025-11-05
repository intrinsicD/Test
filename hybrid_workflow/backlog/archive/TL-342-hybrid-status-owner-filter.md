---
id: TL-342
title: Hybrid status reporter owner filter
status: done
priority: P3
area: tools
size: S
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links: []
---

# Task TL-342 — Hybrid Status Reporter Owner Filter

## Intent

Expose an owner filter in the hybrid status reporter so agents can focus on their assigned backlog entries when triaging work.

---

## Context

**Current State:**
- `scripts.workflow.report_hybrid_status` filters by `status` and `priority` only.
- Agents must manually scan large tables to find entries they own.
- Weekly coordination requires quick visibility into owner-specific queues.

**Desired State:**
- CLI flag narrows results to tasks owned by a specific person or role.
- Tests cover filtering semantics so regressions surface immediately.
- Documentation and roadmap reflect the new tooling capability.

**References:**
- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/AGENTS.md` (workflow usage guidance)

---

## Design / Plan

### Constraints

- Follow `hybrid_workflow/CONTRIBUTING.md` Python guidelines (type hints, docstrings, pathlib).
- Keep CLI behaviour backward compatible; `--owner` should compose with existing filters.
- Update tests and roadmap metadata alongside code.

### Approach

1. Extend `filter_tasks` to accept an optional owner parameter.
2. Wire a `--owner` CLI argument through argument parsing and filtering.
3. Add targeted unit coverage verifying owner filtering and mixed filters.
4. Document the new capability in this task record and roadmap bundle C.

### Edge Cases & Failure Modes

- **Unknown owner:** Filtering should return an empty result without errors.
- **Case sensitivity:** Owner matching remains exact to align with metadata conventions.
- **Next actions path:** `--next-actions` bypasses owner filtering; maintain current behaviour.

### Test Plan

- `pytest scripts/tests/test_report_hybrid_status.py`
- Ensure `--owner` support documented via test coverage.

---

## Steps

1. [x] Analyse existing filtering logic and CLI arguments.
2. [x] Implement owner filtering support in reporter script.
3. [x] Expand unit tests covering owner filters and composability.
4. [x] Update roadmap bundle C to record the completed tooling improvement.
5. [x] Capture pytest evidence in this task file after running validation.
6. [x] Archive task with `status: done` and cross-links updated.

---

## Evidence

### Test Results

```bash
$ pytest scripts/tests/test_report_hybrid_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 10 items

scripts/tests/test_report_hybrid_status.py ..........                                                                    [100%]

====================================================== 10 passed in 0.07s ======================================================
```

**Test Summary:**
- ✅ `pytest scripts/tests/test_report_hybrid_status.py`

### Updated Files

- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/backlog/archive/TL-342-hybrid-status-owner-filter.md`
- `hybrid_workflow/ROADMAP.md`

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)

**Notes:**
- Encourage agents to pass `--owner <role>` during standups to surface action items quickly.

**Follow-ups:**
- [ ] Evaluate fuzzy matching across owner aliases if future backlog entries require synonyms.

---
