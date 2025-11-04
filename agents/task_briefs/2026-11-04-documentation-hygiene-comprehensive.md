# Task Brief: Comprehensive Documentation Hygiene

> Owner: Docs/DevRel (Role 8)  
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)  
> Audit Date: 2026-11-04  
> Backlog Reference: Documentation maintenance (ongoing)

## 1. Summary
- **Title:** Comprehensive Documentation Hygiene Across All Markdown Files
- **Roadmap / Backlog Reference:** Maintenance task supporting all active initiatives
- **Primary Goal:** Archive completed task artifacts, resolve orphaned documents, and maintain documentation health without breaking existing workflows
- **Linked Workflow Artefacts:** 
  - Task brief: `agents/task_briefs/2026-11-04-documentation-hygiene-comprehensive.md`
  - Audit report: `docs/archive/doc_audit_report.json`

## 2. Scope & Boundaries
### In scope:
- Archive 95 completed task artifacts from agents/task_briefs/ and agents/context_packages/
- Resolve 16 orphaned documents (link integration or archival)
- Update cross-references where archival breaks links
- Create archive index for discoverability
- Preserve all metadata (timestamps, authorship, content)
- Validate no broken links post-cleanup

### Out of scope:
- Modifying active documentation content (114 active files remain untouched)
- Changing documentation structure or taxonomy
- Rewriting or refactoring documentation content
- Adding new documentation guidelines

### Architectural considerations / ADRs:
- Follow AGENTS.md Section 0.7 Documentation Integration Checklist
- Maintain docs/NAVIGATION.md as authoritative index
- Preserve archive/ directory structure for provenance

## 3. Success Criteria
### Functional:
- All 95 completed task artifacts moved to appropriate archive locations
- All 16 orphaned documents either linked or archived
- Archive index created at docs/archive/ARCHIVE_INDEX.md
- Zero broken links after cleanup

### Documentation:
- Updated docs/NAVIGATION.md to reflect archival structure
- Archive index documents what was moved and why
- Completion summary with before/after metrics

### Validation:
- `python scripts/validate_docs.py` passes without errors
- All cross-references resolve correctly
- Archive structure browsable and discoverable

### Quality gates & benchmarks:
- No changes to active documentation files (114 files)
- All archived content preserves original timestamps
- Archive operations reversible (content not deleted)

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | README.md | Workspace overview - no changes needed | N/A |
| 2 | docs/NAVIGATION.md | Will update to reference archive index | Docs/DevRel |
| 3 | docs/ROADMAP.md | References completed items now archived | Knowledge Librarian |
| 4 | docs/backlog/active/ | Some archived tasks may reference agent artifacts | Knowledge Librarian |
| 5 | docs/modules/*/README.md | Module docs stable, no changes needed | N/A |
| 6 | docs/specs/ADR-*.md | ADRs unchanged | N/A |
| 7 | docs/archive/ | Target location for archival | Docs/DevRel |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | AI Agent | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | AI Agent | 2 & 5 | Context package, archive hand-off | Active |
| Specialist Engineer(s) | N/A | 3 | Not required for doc-only task | N/A |
| Docs/DevRel | AI Agent | 2, 4, 5 | Documentation updates, archival execution | Active |
| QA/Test Specialist | AI Agent | 4 | Run validate_docs.py | Active |
| Performance Engineer | N/A | 4 | Not applicable | N/A |
| Safety Reviewer | N/A | 4 | Not applicable | N/A |
| Reviewer | AI Agent | 4 | Review archival plan | Active |
| Release Manager | N/A | 5 | Not applicable | N/A |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Problem statement received | Task brief completed | This document |
| 2 – Context Assembly | Task brief approved | Detailed action plan created | Section 8 below |
| 3 – Execution & Collaboration | Action plan approved | All archival operations complete | Git commits |
| 4 – Quality Gates | Archival complete | validate_docs.py passes, no broken links | Test output |
| 5 – Release & Documentation Sync | Quality gates pass | Archive index published, metrics reported | Completion summary |

## 7. Timeline & Milestones
- Kickoff: 2026-11-04T10:01:00Z
- Implementation window: 2026-11-04 (same day)
- Quality gate window: 2026-11-04 (same day)
- Release target: 2026-11-04 (same day)
- Post-release monitoring: Ongoing link validation in CI

## 8. Detailed Action Plan

### 8.1 Completed Task Artifacts (95 files)
**Target:** Move to `docs/archive/agents/` with subdirectories for task_briefs and context_packages

**High-confidence archival targets (88 files):**
- agents/task_briefs/2026-*.md (32 files) - All dated task briefs with completion markers
- agents/context_packages/2026-*.md (56 files) - All dated context packages with completion markers

**Requires review (7 files):**
- agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md - Keep (referenced by AGENTS.md)
- agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md - Keep (active template)
- agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md - Keep (active template)
- agents/TEMPLATES/ADR_TEMPLATE.md - Keep (active template)
- docs/archive/backlog/legacy/tasks/*.md (already archived) - No action

**Actions:**
1. Create `docs/archive/agents/task_briefs/` directory
2. Create `docs/archive/agents/context_packages/` directory
3. Move 32 dated task briefs to archive
4. Move 56 dated context packages to archive
5. Update any cross-references in active backlog items

### 8.2 Orphaned Documents (16 files)
**Category A: Design docs to link from NAVIGATION.md (3 files):**
- docs/design/AN-230-benchmark-harness-design.md
- docs/modules/compute/DISPATCHER-EXTENSION-GUIDE.md
- docs/modules/rendering/QUICKSTART.md

**Category B: Module backlogs (consolidated into main README) (2 files):**
- docs/modules/geometry/BACKLOG.md
- docs/modules/tools/BACKLOG.md

**Category C: Archive completed reviews (3 files):**
- docs/reviews/TASK_COMPLETION_SUMMARY.md
- docs/reviews/GEOMETRY_VIEWER_BUILD_FIX_INDEX.md
- docs/reviews/ARCHITECTURE_EVALUATION_EXECUTIVE_SUMMARY.md

**Category D: Archive orphaned context packages (7 files):**
- All remaining orphaned context packages → docs/archive/agents/context_packages/

**Category E: Design doc to archive (1 file):**
- docs/modules/runtime/AI004_CONFIGURATION_LOADER_DESIGN.md (superseded by ADR-0007)

**Actions:**
1. Add references in docs/NAVIGATION.md for Category A
2. Merge Category B into module READMEs, archive originals
3. Move Category C to docs/archive/reviews/
4. Move Category D per section 8.1
5. Move Category E to docs/archive/design/

### 8.3 Cross-Reference Updates
**Files requiring link updates:**
- docs/NAVIGATION.md - Add newly referenced design docs
- Any active backlog items linking to archived task briefs (scan required)

### 8.4 Archive Structure
```
docs/archive/
  agents/
    task_briefs/
      2026-02-*.md
      2026-03-*.md
      2026-04-*.md
    context_packages/
      2026-02-*.md
      2026-03-*.md
      2026-04-*.md
  design/
    AI004_CONFIGURATION_LOADER_DESIGN.md
  reviews/
    TASK_COMPLETION_SUMMARY.md
    GEOMETRY_VIEWER_BUILD_FIX_INDEX.md
    ARCHITECTURE_EVALUATION_EXECUTIVE_SUMMARY.md
  ARCHIVE_INDEX.md (new - index of all archived content)
```

## 9. Known Risks & Dependencies
### Risks:
- **Risk 1:** Breaking cross-references during archival
  - Mitigation: Scan all active docs for links to archived files before moving
  - Mitigation: Update links or document in ARCHIVE_INDEX.md
  
- **Risk 2:** Loss of historical context
  - Mitigation: Preserve all metadata (timestamps, content)
  - Mitigation: Create comprehensive archive index
  
- **Risk 3:** Confusion about archived vs active content
  - Mitigation: Clear ARCHIVE_INDEX.md with rationale for each archival
  - Mitigation: Update docs/NAVIGATION.md with archive browsing guidance

### Dependencies:
- Git for version control and metadata preservation
- scripts/validate_docs.py for link validation
- docs/NAVIGATION.md as authoritative documentation index

### Contingency:
- All operations are git-tracked and reversible
- Archive is browsable and searchable
- Original timestamps preserved via git metadata

## 10. Communication Plan
- Async updates cadence: After each major section (8.1, 8.2, 8.3, 8.4)
- Live sync triggers: Quality gate failures or unexpected link breaks
- Escalation path: Agent Orchestrator for scope questions

## 11. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-11-04T10:01 | AI Agent | Initial audit completed | 225 files scanned, 95 archival candidates identified |
| 2026-11-04T10:15 | AI Agent | Task brief created | Action plan approved, proceeding to execution |

## 12. Quality Gates Checklist
- [ ] All 88 dated task briefs/context packages archived
- [ ] All 16 orphaned documents resolved (linked or archived)
- [ ] Archive index created with full inventory
- [ ] docs/NAVIGATION.md updated
- [ ] `python scripts/validate_docs.py` passes
- [ ] No broken links in active documentation
- [ ] Git history preserves all timestamps
- [ ] Completion summary generated

> **Reminder:** Update this brief as archival progresses. Track each file moved and link updated.
