# Archive Index

This index catalogs all archived documentation with rationale and provenance. Archived content is preserved for historical reference but is no longer actively maintained.

---

## Archive Structure

```
docs/archive/
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

### 1. Module Backlogs

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

### 3. Design Documents

**Location:** `docs/archive/design/`

**File:** `AI004_CONFIGURATION_LOADER_DESIGN.md`

**Rationale:** Superseded by ADR-0007 (AI-004 configuration schema). The ADR represents the binding architectural decision, while this design doc was an earlier exploration. Preserved for historical context.

**Active Reference:** `docs/specs/ADR_0007_AI_004_CONFIGURATION_SCHEMA.md`

---

### 4. Review Summaries

**Location:** `docs/archive/reviews/legacy/`

**Files:**
- `TASK_COMPLETION_SUMMARY.md` - Historical task completion summary
- `GEOMETRY_VIEWER_BUILD_FIX_INDEX.md` - Build fix session index
- `ARCHITECTURE_EVALUATION_EXECUTIVE_SUMMARY.md` - Architecture evaluation summary

**Rationale:** These review summaries documented completed work sessions but had no active cross-references. Archived to declutter active reviews/ directory. Current active reviews remain in `docs/reviews/`.

**Active Reviews (not archived):**
- Architecture audits and roadmap reviews
- Application framework completion summaries
- Session summaries for recent work
- Component analysis documents
- Geometry viewer architecture analysis

---

### 5. Pre-existing Archives (Unchanged)

The following archive categories existed prior to cleanup and remain unchanged:

**`docs/archive/backlog/legacy/tasks/`:**
- Sprint planning
- AI-004 initiative tasks
- Module-specific completed tasks (assets, benchmarks, compute, geometry, IO, math, physics, rendering, runtime, testing, tooling)
- Research baseline tasks

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
