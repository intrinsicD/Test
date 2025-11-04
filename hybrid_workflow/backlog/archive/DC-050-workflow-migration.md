---
id: DC-050
title: Migrate workflow to hybrid model
status: done
priority: P3
area: docs
size: M
owner: agent-orchestrator
gates: [docs]
relates_to: [bundle:C]
blocked_on: []
links: ["hybrid_workflow/README.md", "docs/NAVIGATION.md"]
---

# Task DC-050 — Migrate Workflow to Hybrid Model

## Intent

Implement the hybrid workflow system in `hybrid_workflow/` directory so the team can benefit from streamlined task tracking with metadata-driven automation while preserving quality coordination for complex work.

---

## Context

**Current State:**
- Root-level workflow is comprehensive but heavyweight (3-4 documents per task)
- Proposed `/workflow/` redesign is simpler but lacks role coordination
- No structured metadata for automation

**Desired State:**
- Hybrid workflow combines best of both approaches
- Metadata-driven task files enable automation
- Optional artifacts for complex coordination
- Documentation updated to reference new system

**References:**
- Analysis: workflow comparison documents
- Current system: `AGENTS.md`, `CONTRIBUTION.md`, `agents/ROLES.md`
- Proposed system: `workflow/AGENTS.md`, `workflow/CONTRIBUTING.md`

---

## Design / Plan

### Constraints

- Preserve valuable concepts: Context Ladder, role coordination, quality gates
- Simplify task tracking with frontmatter metadata
- Enable automation through structured data
- Maintain integration with existing docs infrastructure
- Provide migration path for existing tasks

### Implementation Approach

Create new `hybrid_workflow/` directory with:
1. **AGENTS.md** — 7-step workflow + Context Ladder + optional role coordination
2. **CONTRIBUTING.md** — Coding standards and conventions
3. **ROADMAP.md** — Bundle-based priorities with metadata integration
4. **README.md** — Overview and comparison
5. **backlog/000-template.md** — Metadata-driven task template
6. **backlog/archive/** — Completed task storage

### Test Plan

- **Documentation validation:** Run `python scripts/validate_docs.py` after creation
- **Usability test:** Create example task migration (T-0120) to validate template
- **Automation test:** Verify metadata can be queried with grep/scripts

---

## Steps

1. [x] Create `hybrid_workflow/AGENTS.md` with 7-step workflow and Context Ladder
2. [x] Create `hybrid_workflow/CONTRIBUTING.md` with coding standards
3. [x] Create `hybrid_workflow/ROADMAP.md` with bundle structure
4. [x] Create `hybrid_workflow/README.md` with overview and comparison
5. [x] Create `hybrid_workflow/backlog/000-template.md` with metadata template
6. [x] Create example migration: `backlog/T-0120-gpu-resource-provider.md`
7. [x] Create this meta-task as simple example: `backlog/DC-050-workflow-migration.md`
8. [x] Validate all cross-links work correctly — ran `python scripts/validate_docs.py`
9. [x] Update `docs/NAVIGATION.md` to reference hybrid workflow and directory additions
10. [x] Document migration guide for existing tasks (`hybrid_workflow/MIGRATION.md`)
11. [x] Create automation examples (status report script + README references)

---

## Evidence

### Test Results

```bash
$ python scripts/validate_docs.py
All documentation links resolved successfully.

$ python -m scripts.workflow.report_hybrid_status --include-archived
Status       Priority  ID      Owner               Title                             File
===========  ========  ======  ==================  ================================  ===========================================
in_progress  P1        T-0120  rendering-lead      GPU resource provider completion  hybrid_workflow/backlog/T-0120-gpu-resource-provider.md
done         P3        DC-050  agent-orchestrator  Migrate workflow to hybrid model  hybrid_workflow/backlog/archive/DC-050-workflow-migration.md

Status counts:
  in_progress: 1
  done: 1
```

**Test Summary:**
- Hybrid workflow references validated with `python scripts/validate_docs.py`
- Status reporting script surfaces active tasks and confirms migrated entry metadata

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| docs | [x] Complete | Docs/DevRel | `python scripts/validate_docs.py`, automation script output |

### Updated Files

**Created (initial implementation):**
- `hybrid_workflow/AGENTS.md`
- `hybrid_workflow/CONTRIBUTING.md`
- `hybrid_workflow/ROADMAP.md`
- `hybrid_workflow/README.md`
- `hybrid_workflow/backlog/000-template.md`
- `hybrid_workflow/backlog/T-0120-gpu-resource-provider.md` (example)
- `hybrid_workflow/backlog/DC-050-workflow-migration.md` (this file)

**Finalisation assets:**
- `hybrid_workflow/MIGRATION.md`
- `scripts/workflow/__init__.py`
- `scripts/workflow/report_hybrid_status.py`

**Updated:**
- `docs/NAVIGATION.md` — references hybrid workflow quick start and migration guide
- `hybrid_workflow/README.md` — links migration guide and automation script
- `hybrid_workflow/backlog/DC-050-workflow-migration.md` — completed status, evidence, and checklists

**Pending Updates:**
- None (all scope complete)

---

## Completion Checklist (Definition of Done)

- [x] All core workflow files created
- [x] Template created with metadata frontmatter
- [x] Example task migration demonstrates usage
- [x] Cross-links validated with `python scripts/validate_docs.py`
- [x] `docs/NAVIGATION.md` updated with hybrid workflow references
- [x] Migration guide documented
- [x] Automation examples provided (status report script + grep examples)
- [x] Task metadata updated to `status: done`

---

## Result

**PR:** (pending completion)  
**SHA:** (pending merge)  
**Completion Date:** 2025-11-05

**Notes:**

This task serves as both:
1. The implementation of the hybrid workflow system
2. A simple example of what lightweight task tracking looks like

Key design decisions:
- Combined clarity of 7-step workflow with depth of Context Ladder
- Made role coordination optional (scales with task complexity)
- Used YAML frontmatter for automation-friendly metadata
- Preserved integration with existing docs infrastructure
- Created both simple (this task) and complex (T-0120) examples

**Next Steps:**
- Circulate migration guide to module leads and capture feedback in follow-up tasks
- Prototype dashboard/roadmap automation (spawn TL-320/TL-321 follow-ups)
- Monitor adoption and schedule retrospective after first migrated sprint

**Follow-ups:**
- [ ] Build web dashboard for task status → Create task TL-320
- [ ] Script to auto-update ROADMAP from task metadata → Create task TL-321

---

_Created as part of hybrid workflow implementation on 2025-11-04_

