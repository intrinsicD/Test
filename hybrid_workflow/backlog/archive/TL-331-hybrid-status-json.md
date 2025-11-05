---
id: TL-331
title: Hybrid status reporter JSON export
status: done
priority: P3
area: tools
size: S
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - "scripts/workflow/report_hybrid_status.py"
  - "scripts/tests/test_report_hybrid_status.py"
---

# Task TL-331 — Hybrid status reporter JSON export

## Intent

Extend the hybrid workflow status reporter so automation can consume task metadata without scraping table output.

---

## Context

- Roadmap bundle C tracks documentation and automation improvements for the workflow system.
- Status tooling only produced text tables, forcing downstream scripts to re-parse formatted strings.
- The dashboard generator already consumes structured data; exposing JSON directly from the CLI aligns both entry points.

---

## Design / Plan

- Add a `--format` flag to `report_hybrid_status.py` supporting `table` (default) and `json`.
- Reuse the existing `TaskMetadata` dataclass for serialisation while keeping dashboard imports stable.
- Document the new flag in workflow guidance so contributors know how to integrate with automation.
- Back the feature with targeted unit tests covering both empty and populated payloads.

---

## Steps

1. [x] Implement JSON payload generation with deterministic ordering and counts.
2. [x] Add CLI flag wiring and keep table rendering unchanged by default.
3. [x] Document new usage in `hybrid_workflow/README.md` and `hybrid_workflow/MIGRATION.md`.
4. [x] Cover success and edge cases via `scripts/tests/test_report_hybrid_status.py`.

---

## Evidence

### Test Results

```bash
$ pytest scripts/tests/test_report_hybrid_status.py
===================================================== test session starts ======================================================
platform linux -- Python 3.12.10, pytest-8.4.1, pluggy-1.6.0
rootdir: /workspace/Test
collected 4 items

scripts/tests/test_report_hybrid_status.py ....                                                                          [100%]

====================================================== 4 passed in 0.05s =======================================================
```

```bash
$ python scripts/validate_docs.py
All documentation links resolved successfully.
```

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [x] Complete | QA/Test Specialist | `pytest scripts/tests/test_report_hybrid_status.py` |
| docs | [x] Complete | Docs/DevRel | `python scripts/validate_docs.py`; `hybrid_workflow/README.md`, `hybrid_workflow/MIGRATION.md` updates |

### Updated Files

- `scripts/workflow/report_hybrid_status.py`
- `scripts/tests/test_report_hybrid_status.py`
- `hybrid_workflow/README.md`
- `hybrid_workflow/MIGRATION.md`

---

## Completion Checklist (Definition of Done)

- [x] JSON output available for automation use.
- [x] Documentation references the new flag.
- [x] Unit tests protect the new behaviour and edge cases.
- [x] Quality report updated with passing test results.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)
