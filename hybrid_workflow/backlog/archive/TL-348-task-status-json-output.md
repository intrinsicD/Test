---
id: TL-348
title: Task status CLI JSON output
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
  - "hybrid_workflow/README.md"
  - "hybrid_workflow/QUICK_REFERENCE.md"
  - "hybrid_workflow/IMPLEMENTATION_SUMMARY.md"
  - "hybrid_workflow/COMPLETE.md"
---

# Task TL-348 — Task status CLI JSON output

## Intent

Enable automation workflows to consume hybrid workflow backlog data directly from
`hybrid_workflow/task_status.py` by adding JSON-formatted output that mirrors the
documentation-first tooling experience.

---

## Context

**Current State:**
- The CLI emits human-readable tables and summaries only.
- JSON exports exist for the higher-level `report_hybrid_status.py` tool but not
  the lightweight CLI used for terminal-first workflows.
- Automation must currently shell out to the reporting script or post-process
  tabular output, which is brittle and diverges from documentation guidance.

**Desired State:**
- `task_status.py` accepts `--format json` for task listings, summaries, and
  detail views.
- JSON payloads include status/priority counts plus the task metadata required
  for dashboards and scripting.
- Documentation and quick-reference material highlight the new automation path.

**References:**
- [`hybrid_workflow/AGENTS.md`](../../AGENTS.md) — automation guidance.
- [`scripts/workflow/report_hybrid_status.py`](../../scripts/workflow/report_hybrid_status.py) —
  existing JSON exporter for summary reporting.
- [`hybrid_workflow/QUICK_REFERENCE.md`](../QUICK_REFERENCE.md) — command cheat sheet.

---

## Design / Plan

### Behaviour

- Add `--format {table,json}` to the CLI parser (default: `table`).
- Serialize task lists with deterministic ordering and summary counts.
- Support JSON responses for `--summary` and `--detail` pathways.
- Return structured error payloads when a requested task is missing.

### Data Format

```json
{
  "tasks": [
    {
      "id": "TL-310",
      "title": "Editor foundations",
      "status": "ready",
      "priority": "P1",
      "owner": "tools-lead",
      "gates": ["tests", "docs"],
      "relates_to": ["bundle:B"],
      "blocked_on": [],
      "file": "hybrid_workflow/backlog/archive/TL-310-editor-foundations.md"
    }
  ],
  "counts": {
    "by_status": {"ready": 1},
    "by_priority": {"P1": 1},
    "blocked": 0,
    "total": 1,
    "available": 6
  }
}
```

### Testing

- Extend `python/tests/test_hybrid_workflow_task_status.py` with JSON-focused
  unit tests covering parser wiring, list output, summary aggregation, and detail
  serialization.
- Ensure existing regression coverage still passes.

### Documentation

- Update quick reference, implementation summary, completion summary, and README
  automation examples with JSON usage snippets.
- Archive the task under `backlog/archive/` with evidence below.

---

## Steps

1. [x] Implement JSON serialization helpers and CLI flag handling in
   `hybrid_workflow/task_status.py`.
2. [x] Add regression coverage for JSON rendering and parser support.
3. [x] Refresh workflow documentation to highlight the automation commands.
4. [x] Archive TL-348 with evidence.

---

## Evidence

### Test Results

```bash
$ pytest python/tests/test_hybrid_workflow_task_status.py -q
```

### Updated Files

- `hybrid_workflow/task_status.py`
- `python/tests/test_hybrid_workflow_task_status.py`
- `hybrid_workflow/README.md`
- `hybrid_workflow/QUICK_REFERENCE.md`
- `hybrid_workflow/IMPLEMENTATION_SUMMARY.md`
- `hybrid_workflow/COMPLETE.md`

---

## Completion Checklist (Definition of Done)

- [x] CLI exposes `--format json` with table as the default.
- [x] JSON output covers listings, summaries, and detail views with deterministic
      ordering.
- [x] Automation-focused documentation references the new commands.
- [x] Tests exercise serialization helpers and parser wiring.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** 2026-05-06
