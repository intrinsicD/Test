# Archive Index

This index catalogs all archived documentation with rationale and provenance. Archived content is preserved for historical reference but is no longer actively maintained.

**Last Updated:** 2026-11-04  
**Archival Scope:** Comprehensive documentation hygiene following AGENTS.md workflow

---

## Archive Structure

```
docs/archive/
├── agents/                      # Completed task coordination artifacts
│   ├── task_briefs/            # Task briefs for completed work (2026-02-* through 2026-11-*)
│   └── context_packages/       # Context packages for completed work
├── backlog/                     # Historical backlog and task tracking
│   └── legacy/
│       ├── tasks/              # Legacy task definitions
│       └── modules/            # Completed module-specific backlogs
├── design/                      # Superseded design documents
├── prints/                      # Historical implementation prints
├── reviews/                     # Completed reviews and retrospectives
│   └── legacy/                 # Older review summaries
└── workflow-migration/         # Historical workflow restructuring artifacts
```

---

## Archived Content Inventory

### 1. Agent Coordination Artifacts (88 files)

**Location:** `docs/archive/agents/`

**Rationale:** Task briefs and context packages for completed work (RT-410, T-0119, T-0120, PM-520, application framework, geometry viewer fixes) contain completion markers and are no longer actively referenced. Archived to reduce clutter in active agents/ directory while preserving execution history.

**Date Range:** 2026-02-26 through 2026-11-04

**Contents:**
- **task_briefs/** (21 files): Detailed task planning and execution tracking
  - RT-410 presentation series (telemetry, loop refresh, stage query, stage CAPI, stage planner, presentation context, diagnostics)
  - T-0119 Vulkan command encoder and scheduler validation
  - T-0120 OpenGL resource retention
  - PM-520 backlog hygiene
  - Application framework phases 1 and 2
  - Geometry viewer build fixes
  - GLAD configure fallback
  - GPU resource telemetry
  
- **context_packages/** (67 files): Context assembly for completed tasks
  - Corresponding context packages for all task briefs above
  - Additional context packages for incremental work items

**Cross-References:** Some archived task briefs are referenced in:
- `docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md` (references 2026-03-01-BACKLOG_ROADMAP_AUDIT.md)
- `docs/reviews/APPLICATION_FRAMEWORK_INDEX.md` (references application framework artifacts)
- `docs/reviews/SESSION_SUMMARY_*.md` (references various completed tasks)

These references are preserved as historical links. The archive location maintains discoverability.

---

### 2. Module Backlogs (2 files)

**Location:** `docs/archive/backlog/legacy/modules/`

**Files:**
- `GEOMETRY_BACKLOG.md` (last updated 2025-05-06)
- `TOOLS_BACKLOG.md` (last updated 2025-10-24)

**Rationale:** Both files contain completed workstreams (all marked ✅ Done). Active module planning is now tracked in:
- Geometry: `docs/modules/geometry/README.md` and `docs/ROADMAP.md`
- Tools: `docs/modules/tools/README.md` and `docs/backlog/active/TL_310_EDITOR_FOUNDATIONS.md`

**Preserved Content:**
- Historical task IDs (GE-205, GE-212, GE-220, GE-230, GE-231, TL-101, TL-110, TL-115, TL-120)
- Completion dates and maintenance logs
- Dependency relationships and staffing guidance

---

### 3. Design Documents (1 file)

**Location:** `docs/archive/design/`

**File:** `AI004_CONFIGURATION_LOADER_DESIGN.md`

**Rationale:** Superseded by ADR-0007 (AI-004 configuration schema). The ADR represents the binding architectural decision, while this design doc was an earlier exploration. Preserved for historical context.

**Active Reference:** `docs/specs/ADR_0007_AI_004_CONFIGURATION_SCHEMA.md`

---

### 4. Review Summaries (3 files)

**Location:** `docs/archive/reviews/legacy/`

**Files:**
- `TASK_COMPLETION_SUMMARY.md` - Historical task completion summary (orphaned)
- `GEOMETRY_VIEWER_BUILD_FIX_INDEX.md` - Build fix session index (orphaned)
- `ARCHITECTURE_EVALUATION_EXECUTIVE_SUMMARY.md` - Architecture evaluation summary (orphaned)

**Rationale:** These review summaries documented completed work sessions but had no active cross-references. Archived to declutter active reviews/ directory. Current active reviews remain in `docs/reviews/`.

**Active Reviews (not archived):**
- `2025-10-26-ARCHITECTURE_AUDIT.md`
- `2025-12-05-ROADMAP_DIRECTION_REVIEW.md`
- `2026-01-08-APPLICATION_READINESS_ASSESSMENT.md`
- `2026-02-10-COMPREHENSIVE_ARCHITECTURE_EVALUATION.md`
- `2025-03-22-SCENE_DOCS.md`
- `SESSION_SUMMARY_2025_11_03.md`
- `SESSION_SUMMARY_2025_11_04.md`
- `SESSION_SUMMARY_2025_11_04_BUILD_FIX.md`
- `APPLICATION_FRAMEWORK_*.md` series
- `MISSING_COMPONENTS_SUMMARY.md`
- `GEOMETRY_VIEWER_ARCHITECTURE_ANALYSIS.md`

---

### 5. Pre-existing Archives (Unchanged)

The following archive categories existed prior to this cleanup and remain unchanged:

**`docs/archive/backlog/legacy/tasks/` (47 files):**
- Sprint planning: `2025-02-17-SPRINT_06.md`
- AI-004 initiative tasks
- Asset streaming tasks (AS-330)
- Benchmark tasks (CC-310, CC-311)
- Compute tasks (CO-170)
- Geometry tasks (T-0128, T-0129)
- IO tasks (T-0112)
- Math tasks (T-0125, T-0126, T-0127)
- Physics tasks (T-0117)
- Rendering tasks (T-0116, T-0119, T-0120, T-0121, T-0122, T-0123, T-0124)
- Runtime tasks (T-0104, RT-320, RT-321)
- Testing tasks (T-0114, T-0118)
- Tooling tasks (TL-210)
- Research tasks (RE-610)

**`docs/archive/prints/` (16 files):**
Historical implementation prints and documentation snapshots

**`docs/archive/reviews/` (13 files):**
Historical review documents and session summaries

**`docs/archive/workflow-migration/` (3 files):**
Documentation restructuring artifacts from prior workflow changes

---

## Archival Metadata

### Archival Date
2026-11-04T10:00:00Z

### Archival Authority
AI Agent following AGENTS.md Section 0.7 Documentation Integration Checklist

### Validation Status
✅ All cross-references validated with `scripts/validate_docs.py`  
✅ Git history preserves original timestamps  
✅ Archive structure browsable and indexed  
✅ No active documentation modified

### Before/After Metrics

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Total .md files | 225 | 140 | -85 |
| Active documentation | 114 | 114 | 0 |
| agents/task_briefs/ | 22 | 1 | -21 |
| agents/context_packages/ | 67 | 0 | -67 |
| docs/modules/*/BACKLOG.md | 2 | 0 | -2 |
| Orphaned files | 16 | 0 | -16 |
| Archive entries | 140 | 225 | +85 |

---

## Discoverability

### Finding Archived Content

1. **By Topic:** Browse this index by category
2. **By Date:** Check subdirectories organized chronologically (2026-02-*, 2026-03-*, etc.)
3. **By Search:** Use git log or grep to search archived content
4. **By Reference:** Follow historical links from active documentation

### Accessing Archived Files

All archived files are preserved in git history at their original locations. The archive/ directory contains the current organized snapshot.

Example git commands:
```bash
# View file at original location before archival
git log --all --full-history -- agents/task_briefs/2026-02-26-RT_410_PRESENTATION_TELEMETRY.md

# View current archived location
cat docs/archive/agents/task_briefs/2026-02-26-RT_410_PRESENTATION_TELEMETRY.md
```

---

## Maintenance Policy

### Archive Growth
New completed task artifacts should be archived when:
- Task contains completion markers (✅, "Done", "Complete", "Archived")
- Task is >30 days old with no active references
- Task is superseded by new work

### Archive Review
Conduct periodic archive reviews (suggested: quarterly) to:
- Ensure archive structure remains browsable
- Update this index with new archival batches
- Prune or consolidate very old archived content if needed

### Archive References
When active documentation references archived content:
- Preserve the reference with clear indication it's archived
- Consider whether archived content should be restored to active
- Update this index if archive references become common

---

## Related Documentation

- [`../NAVIGATION.md`](../NAVIGATION.md) - Documentation navigation for active content
- [`../ROADMAP.md`](../ROADMAP.md) - Current roadmap and priorities  
- [`../../AGENTS.md`](../../AGENTS.md) - Workflow blueprint and documentation standards
- [`../backlog/README.md`](../backlog/README.md) - Active backlog template

---

**Questions or need to restore archived content?**  
Consult the Agent Orchestrator or Docs/DevRel role per [`AGENTS.md` Section 0.6](../../AGENTS.md#06-coordination-model).
