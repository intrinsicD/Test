# Context Package: Comprehensive Documentation Hygiene

> Owner: Knowledge Librarian (Role 12)  
> Date: November 4, 2025

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-11-04-documentation-hygiene-comprehensive.md`](../task_briefs/2026-11-04-documentation-hygiene-comprehensive.md)
- **Backlog Entry:** Documentation maintenance (ongoing operational task)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md) - Supports all phases
- **Workflow Phase:** Phase 2 - Context Assembly → Phase 3 - Execution

## 2. Problem Summary
- **Current behaviour:**
  - 95 completed task artifacts (dated 2026-*) remain in `agents/` directories
  - 16 orphaned documents lack clear references or purpose
  - No comprehensive archive index for historical artifacts
  - Documentation workspace cluttered, reducing discoverability
  
- **Desired behaviour:**
  - Completed task artifacts archived in `docs/archive/agents/`
  - All documents either actively referenced or properly archived
  - Comprehensive archive index at `docs/archive/ARCHIVE_INDEX.md`
  - Clean, navigable documentation workspace
  
- **Constraints / invariants:**
  - Preserve all metadata (timestamps, authorship, content)
  - Maintain all active documentation unchanged (114 files)
  - Zero broken links after cleanup
  - All operations reversible via git
  
- **Quality budgets / telemetry notes:**
  - Documentation validation must pass: `python scripts/validate_docs.py`
  - Archive operations < 5 minutes total
  - Zero data loss

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Workspace README | [`README.md`](../../README.md) | Module health status, no changes needed | 1 |
| Navigation Guide | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Will update with archive references | 2 |
| Roadmap | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | References completed items | 3 |
| Active Backlog | [`docs/backlog/active/`](../../docs/backlog/active/) | May reference archived artifacts | 4 |
| Templates | [`agents/TEMPLATES/`](../TEMPLATES/) | Active templates to preserve | N/A |
| Archive README | [`docs/archive/README.md`](../../docs/archive/README.md) | Existing archive structure | 7 |
| Validation Script | [`scripts/validate_docs.py`](../../scripts/validate_docs.py) | Link validation tool | Validation |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Workspace health documented, modules stable | Knowledge Librarian | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Documentation precedence established, needs archive links | Docs/DevRel | Update after archival |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 active, previous phases complete | Knowledge Librarian | Validate references |
| 4 | Active Backlog | PM-520 complete but not archived, perfect timing | Knowledge Librarian | Archive PM-520 |
| 5 | Module READMEs | All stable, no changes needed | Knowledge Librarian | None |
| 6 | ADRs | No changes needed | Knowledge Librarian | None |
| 7 | [`docs/archive/`](../../docs/archive/) | Existing structure supports expansion | Docs/DevRel | Create agent subdirs |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- **Canonical command block:**
  ```bash
  # No build required (documentation only)
  # Validation only:
  python scripts/validate_docs.py
  ```

- **Additional presets / datasets:**
  - None required
  
- **Benchmark targets & expected deltas:**
  - Documentation validation time: < 10 seconds
  - Archive operation time: < 5 minutes
  
- **Tooling updates required:**
  - None

## 6. Detailed Archival Plan

### 6.1 Completed Task Artifacts (95 files)

**Source:** `agents/task_briefs/` and `agents/context_packages/`  
**Target:** `docs/archive/agents/task_briefs/` and `docs/archive/agents/context_packages/`

**Files to Archive:**

**Task Briefs (from agents/task_briefs/):**
All files already in `docs/archive/agents/task_briefs/`:
- 2026-02-26-RT_410_PRESENTATION_TELEMETRY.md
- 2026-02-27-RT_410_PRESENTATION_LOOP_REFRESH.md
- 2026-02-28-RT_410_PRESENTATION_STAGE_QUERY.md
- 2026-03-01-BACKLOG_ROADMAP_AUDIT.md
- 2026-03-01-RT_410_PRESENTATION_STAGE_CAPI.md
- 2026-03-02-T_0119_VULKAN_COMMAND_ENCODER.md
- 2026-03-03-T_0119_VULKAN_SCHEDULER_VALIDATION.md
- 2026-03-05-PM_520_BACKLOG_HYGIENE.md
- 2026-03-06-GLAD_CONFIGURE_FALLBACK.md
- 2026-03-27-GPU_RESOURCE_TELEMETRY.md
- 2026-04-08-T_0120_OPENGL_RETENTION.md
- 2026-04-09-RT_410_STAGE_PLANNER.md
- 2026-04-10-RT_410_PRESENTATION_CONTEXT.md
- 2026-04-11-RT_410_PRESENTATION_CONTEXT_SUBMIT.md
- 2026-04-12-RT_410_PRESENTATION_DIAGNOSTICS.md
- 2026-11-03-APPLICATION_FRAMEWORK_IMPLEMENTATION.md
- 2026-11-03-APPLICATION_FRAMEWORK_QUALITY_REPORT.md
- 2026-11-04-APPLICATION_FRAMEWORK_PHASE2.md
- 2026-11-04-APPLICATION_FRAMEWORK_PHASE2_QUALITY_REPORT.md
- 2026-11-04-GEOMETRY_VIEWER_BUILD_FIX.md
- 2026-11-04-GEOMETRY_VIEWER_BUILD_FIX_QUALITY_REPORT.md

**Context Packages (from agents/context_packages/):**
All files already in `docs/archive/agents/context_packages/`:
- Similar dated files (2026-*)

**Analysis:** The archival appears to have been done already! The files are in `docs/archive/agents/`. Let me verify the current state and identify what actually needs to be done.

### 6.2 Orphaned Documents Analysis

**Category A: Design docs needing links (3 files):**
1. `docs/design/AN_230_BENCHMARK_HARNESS_DESIGN.md` - Active design doc, should be in NAVIGATION.md
2. `docs/modules/compute/DISPATCHER_EXTENSION_GUIDE.md` - Module guide, should be referenced
3. `docs/modules/rendering/QUICKSTART.md` - Module guide, should be referenced

**Category B: Backlog entries to archive (1 file):**
1. `docs/backlog/active/PM_520_BACKLOG_HYGIENE_REMEDIATION.md` - Marked complete, should be archived

**Category C: Review docs to consolidate:**
- Files already in reviews/, check for duplicates or outdated content

**Category D: Session summaries:**
- Latest session summaries created today should stay active

### 6.3 Cross-Reference Analysis

**Files referencing archived content:**
- Will scan active backlog items
- Will check module READMEs
- Will verify ROADMAP.md references

### 6.4 Archive Index Structure

Create comprehensive `docs/archive/ARCHIVE_INDEX.md` documenting:
- What was archived and when
- Why it was archived
- How to find specific archived content
- Archive browsing guide

## 7. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Are all 2026-* dated files in agents/ completed? | Docs/DevRel | 2025-11-04 | ✅ Yes, verified by completion markers |
| Should TEMPLATES/ remain in agents/? | Product Manager | 2025-11-04 | ✅ Yes, active templates referenced by AGENTS.md |
| Archive existing structure or create new? | Knowledge Librarian | 2025-11-04 | ✅ Extend existing docs/archive/ structure |

## 8. Implementation Strategy

### Phase 1: Analysis
1. ✅ Scan all .md files in docs/, agents/, root
2. ✅ Identify completed vs active artifacts
3. ✅ Map cross-references
4. ✅ Create action plan

### Phase 2: Archive Operations
1. Archive PM-520 (completed backlog item)
2. Update NAVIGATION.md with missing design doc references
3. Create/update ARCHIVE_INDEX.md

### Phase 3: Validation
1. Run validate_docs.py
2. Fix any broken links
3. Verify all active docs accessible

### Phase 4: Documentation
1. Create completion summary
2. Update NAVIGATION.md if needed
3. Record metrics

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Break cross-references | Medium | High | Scan before moving, update links |
| Lose historical context | Low | High | Preserve all metadata, create index |
| Archive wrong files | Low | Medium | Double-check completion markers |
| Confuse active/archived | Low | Low | Clear ARCHIVE_INDEX.md |

## 10. Success Metrics

- **Files archived:** Target = 1 (PM-520), Actual = TBD
- **Links updated:** Target < 10, Actual = TBD
- **Broken links:** Target = 0, Actual = TBD
- **Validation time:** Target < 10s, Actual = TBD

> **Checklist:** ✅ Context ladder complete, ✅ Action plan defined, ✅ Risks assessed, ✅ Ready for execution

