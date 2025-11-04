---
id: DC-050
title: Migrate workflow to hybrid model
status: in_progress
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
8. [ ] Validate all cross-links work correctly
9. [ ] Update `docs/NAVIGATION.md` to reference hybrid workflow
10. [ ] Document migration guide for existing tasks
11. [ ] Create automation examples (status queries, dashboard)

---

## Evidence

### Test Results

```bash
# File creation successful
$ ls -la hybrid_workflow/
total 40
drwxr-xr-x  3 alex alex  4096 Nov  4 10:30 .
drwxr-xr-x 18 alex alex  4096 Nov  4 10:25 ..
-rw-r--r--  1 alex alex  8234 Nov  4 10:28 AGENTS.md
-rw-r--r--  1 alex alex  4512 Nov  4 10:29 CONTRIBUTING.md
-rw-r--r--  1 alex alex  3876 Nov  4 10:30 README.md
-rw-r--r--  1 alex alex  2145 Nov  4 10:30 ROADMAP.md
drwxr-xr-x  3 alex alex  4096 Nov  4 10:31 backlog

$ ls -la hybrid_workflow/backlog/
total 24
drwxr-xr-x 3 alex alex  4096 Nov  4 10:31 .
drwxr-xr-x 3 alex alex  4096 Nov  4 10:30 ..
-rw-r--r-- 1 alex alex  6234 Nov  4 10:31 000-template.md
-rw-r--r-- 1 alex alex  8142 Nov  4 10:32 T-0120-gpu-resource-provider.md
-rw-r--r-- 1 alex alex  2456 Nov  4 10:33 DC-050-workflow-migration.md
drwxr-xr-x 2 alex alex  4096 Nov  4 10:31 archive

# Metadata query test
$ grep "^status:" hybrid_workflow/backlog/*.md
hybrid_workflow/backlog/000-template.md:status: new
hybrid_workflow/backlog/DC-050-workflow-migration.md:status: in_progress
hybrid_workflow/backlog/T-0120-gpu-resource-provider.md:status: in_progress

$ grep "^priority:" hybrid_workflow/backlog/*.md
hybrid_workflow/backlog/000-template.md:priority: P1
hybrid_workflow/backlog/DC-050-workflow-migration.md:priority: P3
hybrid_workflow/backlog/T-0120-gpu-resource-provider.md:priority: P1
```

**Test Summary:**
- File structure created successfully
- Metadata queries working (grep-based)
- Example task migration demonstrates template usage
- Documentation validation: pending (step 8)

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| docs | [~] In Progress | Docs/DevRel | Files created, validation pending |

### Updated Files

**Created:**
- `hybrid_workflow/AGENTS.md`
- `hybrid_workflow/CONTRIBUTING.md`
- `hybrid_workflow/ROADMAP.md`
- `hybrid_workflow/README.md`
- `hybrid_workflow/backlog/000-template.md`
- `hybrid_workflow/backlog/T-0120-gpu-resource-provider.md` (example)
- `hybrid_workflow/backlog/DC-050-workflow-migration.md` (this file)

**Pending Updates:**
- `docs/NAVIGATION.md` — Add hybrid workflow section
- Migration guide document

---

## Completion Checklist (Definition of Done)

- [x] All core workflow files created
- [x] Template created with metadata frontmatter
- [x] Example task migration demonstrates usage
- [ ] Cross-links validated with `python scripts/validate_docs.py`
- [ ] `docs/NAVIGATION.md` updated with hybrid workflow references
- [ ] Migration guide documented
- [ ] Automation examples provided (grep queries, status dashboard)
- [ ] Task metadata updated to `status: done`

---

## Result

**PR:** (pending completion)  
**SHA:** (pending merge)  
**Completion Date:** (in progress, started 2025-11-04)

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
- Complete validation and navigation updates
- Create automation tooling (dashboard, queries)
- Document migration process for remaining active tasks

**Follow-ups:**
- [ ] Build web dashboard for task status → Create task TL-320
- [ ] Script to auto-update ROADMAP from task metadata → Create task TL-321

---

_Created as part of hybrid workflow implementation on 2025-11-04_

