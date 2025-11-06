---
id: TL-345
title: Hybrid status reporter relates_to filter
status: done
priority: P2
area: tools
size: S
owner: tools-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - scripts/workflow/report_hybrid_status.py
  - scripts/tests/test_report_hybrid_status.py
  - hybrid_workflow/README.md
  - hybrid_workflow/QUICK_REFERENCE.md
  - hybrid_workflow/ROADMAP.md
---

# Task TL-345 — Hybrid Status Reporter Relates_To Filter

## Intent

Extend the hybrid status reporter CLI so agents can filter tasks by ROADMAP bundle
metadata, matching the semantics already available in the lightweight
`hybrid_workflow/task_status.py` utility. This keeps automation and manual tooling
in sync and lets teams slice reports by initiative.

---

## Context

**Current State:**
- `scripts/workflow/report_hybrid_status.py` can filter by status, priority, and owner but ignores
  `relates_to` metadata recorded in hybrid workflow backlog files.
- The CLI flattens results into table/JSON outputs that downstream tooling consumes,
  yet bundle-based triage still requires ad-hoc filtering or the separate
  `hybrid_workflow/task_status.py` helper.
- Documentation in `hybrid_workflow/README.md` and `QUICK_REFERENCE.md` only advertises the
  existing filters, leaving bundle-based slices undocumented.

**Desired State:**
- The reporter accepts `--relates-to` arguments (case-insensitive, multi-tag) and
  filters results accordingly.
- JSON output exposes the associated bundle tags so downstream scripts can reuse
  the metadata without reparsing markdown.
- Quick reference documentation highlights the new capability and roadmap bundle
  guidance remains aligned.

**References:**
- [`hybrid_workflow/AGENTS.md`](../AGENTS.md)
- [`docs/ROADMAP.md`](../../docs/ROADMAP.md)
- [`hybrid_workflow/ROADMAP.md`](../ROADMAP.md)
- [`hybrid_workflow/task_status.py`](../task_status.py)
- [`scripts/tests/test_report_hybrid_status.py`](../../scripts/tests/test_report_hybrid_status.py)

---

## Design / Plan

### Constraints

- Maintain CLI compatibility with existing options and defaults.
- Preserve deterministic ordering in reports and JSON payloads.
- Avoid introducing third-party YAML parsers—rely on existing helpers.
- Document the new behaviour wherever the reporter is referenced.
- Keep tests authoritative for both filtering logic and CLI argument handling.

### API / Data Sketch

```
@dataclass
class TaskMetadata:
    path: Path
    identifier: str
    title: str
    status: str
    priority: str
    owner: str
    relates_to: tuple[str, ...]
```

- Extend `filter_tasks` to accept an optional `relates_to` iterable.
- Flatten parsed CLI groups into a single list before filtering.
- Include `relates_to` in JSON serialisation for automation parity.

### Edge Cases & Failure Modes

- **Case-insensitive matches:** Normalise both CLI inputs and task metadata to
  lowercase before comparison.
- **Multiple tags:** Treat `--relates-to A B` as logical OR (any overlap).
- **Missing metadata:** Tasks without `relates_to` should never surface when the
  filter is supplied.

### Test Plan

- **Unit Tests:**
  - Extend `scripts/tests/test_report_hybrid_status.py` to cover relates-to filtering,
    case-insensitive matching, and CLI parsing of grouped arguments.
  - Update JSON expectations to validate the new field.
- **Regression:**
  - Re-run the reporter test suite to ensure existing filters remain unaffected.
- **Docs:**
  - Update `hybrid_workflow/README.md` and `QUICK_REFERENCE.md` snippets to advertise the
    new option (docs gate).

---

## Steps

1. [x] Load roadmap/workflow guidance and confirm parity requirements for automation tools.
2. [x] Extend `TaskMetadata` and CLI parsing to capture bundle metadata without breaking existing flags.
3. [x] Implement case-insensitive relates-to filtering and expose tags via JSON output.
4. [x] Update reporter unit tests plus documentation snippets covering the new option.
5. [x] Run targeted tests and record evidence.

---

## Evidence

### Test Results

```bash
pytest scripts/tests/test_report_hybrid_status.py
```

**Test Summary:**
- ✅ `pytest scripts/tests/test_report_hybrid_status.py`
