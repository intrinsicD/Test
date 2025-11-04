---
id: TL-320
title: Task status dashboard automation
status: in_progress
priority: P3
area: tools
size: M
owner: tools-lead
gates: [docs]
relates_to: [bundle:C]
blocked_on: []
links: ["scripts/workflow/report_hybrid_status.py", "scripts/workflow/dashboard.py", "hybrid_workflow/README.md", "hybrid_workflow/ROADMAP.md"]
---

# Task TL-320 — Task Status Dashboard Automation

## Intent

Build a lightweight dashboard that visualises hybrid workflow task metadata so stakeholders can monitor progress without manually running scripts.

---

## Context

**Current State:**
- Hybrid workflow exposes metadata-rich task files and a CLI status script.
- No visual dashboard exists; status reviews require manual script execution and interpretation.
- PM-510 demos and roadmap syncs need faster visibility into task state transitions.

**Desired State:**
- Web-based dashboard reads task metadata and renders lifecycle states, priorities, and blockers.
- Automation refreshes on demand (local command) with optional static export for docs.
- Links from roadmap and README point to the dashboard for quick overviews.

**References:**
- [`scripts/workflow/report_hybrid_status.py`](../scripts/workflow/report_hybrid_status.py)
- [`hybrid_workflow/README.md`](../hybrid_workflow/README.md)
- [`hybrid_workflow/ROADMAP.md`](../hybrid_workflow/ROADMAP.md)
- [`hybrid_workflow/backlog/DC-050-workflow-migration.md`](archive/DC-050-workflow-migration.md)

---

## Design / Plan

### Constraints

- Parse YAML frontmatter without introducing heavyweight dependencies.
- Provide static HTML/JSON output suitable for CI artifact publication.
- Keep deployment optional; dashboard should run locally with `python` tooling only.
- Ensure accessibility (keyboard navigation, contrast) and responsive layout.

### Implementation Sketch

```python
from pathlib import Path
import frontmatter

def collect_tasks(root: Path):
    for path in root.glob("*.md"):
        post = frontmatter.load(path)
        yield {
            "id": post["id"],
            "title": post["title"],
            "status": post["status"],
            "priority": post["priority"],
            "blocked_on": post.get("blocked_on", []),
            "links": post.get("links", []),
            "file": str(path)
        }
```

Render collected metadata into a single-page dashboard using a lightweight template (Jinja2) and ship CSS from `/assets/`.

### Edge Cases & Failure Modes

- **Malformed frontmatter:** Validate schema and highlight issues in dashboard/error logs.
- **Missing metadata fields:** Fallback to defaults and surface warnings in output.
- **Large task set:** Paginate or allow filtering to keep dashboard responsive.
- **Stale data:** Provide timestamp + command to regenerate output.

### Test Plan

- **Unit Tests:**
  - Parser handles representative task files (active + archived).
  - Filtering/grouping logic for statuses and priorities.
- **Integration Tests:**
  - CLI generates dashboard HTML + JSON snapshots.
  - Validate generated HTML contains references to all tasks.
- **Documentation:**
  - Update README/ROADMAP with dashboard usage instructions.

---

## Steps

1. [x] Define schema + validation helpers for task metadata extraction.
2. [x] Implement dashboard generator under `scripts/workflow/` with CLI entry point.
3. [x] Add unit tests covering parser and rendering pipeline.
4. [x] Provide sample output under `assets/hybrid_workflow_dashboard/` (optional).
5. [x] Document workflow in `hybrid_workflow/README.md` and update roadmap bundle C checkbox.
6. [x] Run `python scripts/validate_docs.py` after documentation updates.
7. [ ] Mark task `done` and archive with evidence once dashboard shipped.

---

## Evidence

### Test Results

```bash
pytest scripts/tests/test_dashboard.py
python scripts/validate_docs.py
```

**Summary:**
- Unit tests: `scripts/tests/test_dashboard.py`
- Integration tests: covered by CLI invocation in tests
- Documentation validation: `python scripts/validate_docs.py`
- Static snapshot: `assets/hybrid_workflow_dashboard/` generated via `python -m scripts.workflow.dashboard --output-dir $(pwd)/assets/hybrid_workflow_dashboard`

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| docs | [x] Pass | Docs/DevRel | README/ROADMAP/NAVIGATION updates + `python scripts/validate_docs.py` |
| tests | [x] Pass | QA/Test | `pytest scripts/tests/test_dashboard.py` |
| perf | [ ] N/A | — | — |
| safety | [ ] N/A | — | — |
| release | [ ] N/A | — | — |

### Updated Files

- `scripts/workflow/dashboard.py`
- `scripts/tests/test_dashboard.py`
- `hybrid_workflow/README.md`
- `hybrid_workflow/ROADMAP.md`
- `docs/NAVIGATION.md` (if adding dashboard reference)
- `assets/hybrid_workflow_dashboard/index.html`
- `assets/hybrid_workflow_dashboard/tasks.json`

---

## Completion Checklist (Definition of Done)

- [x] Dashboard CLI generates HTML/JSON summary from hybrid workflow tasks.
- [x] Tests added for parser + rendering logic.
- [x] Documentation updated with usage instructions and links.
- [x] Optional static snapshot committed or instructions provided for generation.
- [ ] Task status advanced to `done` and moved to archive with evidence.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)

**Notes:**
- Coordinate with PM-510 to reuse dashboard outputs during weekly demos.
- Consider follow-up to publish dashboard in CI pipelines once MVP validated.

**Follow-ups:**
- [ ] Explore live dashboard hosting in docs site → spawn TL-321.

---

## Role Coordination (Optional)

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | Agent Orchestrator | Prioritise automation relative to documentation backlog | Planned |
| Tools Lead | Tools Lead | Implement dashboard tooling | Planned |
| Knowledge Librarian | Knowledge Librarian | Document usage + maintain links | Planned |
| Docs/DevRel | Docs Team | Review dashboard instructions and integrate with docs | Planned |
