# Documentation Restructure Proposal

**Created:** 2025-10-22  
**Status:** DRAFT - Awaiting Review

## Executive Summary

The current documentation has evolved organically and now suffers from:
1. **Redundancy** - Multiple overlapping "start here" documents
2. **Clutter** - Historical artifacts (prints/, reviews/) mixed with active docs
3. **Confusion** - Unclear precedence and navigation paths for AI agents
4. **Maintenance burden** - Duplicated information across root + docs/modules

This proposal recommends a streamlined structure optimized for agentic workflows while maintaining human readability.

---

## Problems with Current Structure

### Critical Issues

| Problem | Impact | Evidence |
|---------|--------|----------|
| Dual entry points (README.md vs docs/README.md) | Agents don't know where to start | Both claim to be "first stop" |
| AGENTS.md vs docs/agents.md overlap | Contradictory guidance possible | ~80% content duplication |
| Completed tasks dominate roadmap | Hard to see what's next | 9/10 initiatives marked ✅ Done |
| prints/ directory (19 files) | Clutters navigation | Implementation prompts from past sprints |
| reviews/ directory (10 files) | Adds noise | Historical conversations, not specs |
| Module roadmaps duplicate central roadmap | Update burden, drift risk | 13 modules × ROADMAP.md |

### Moderate Issues

- **docs/architecture.md** vs **CODING_STYLE.md** have overlapping "invariants" sections
- **docs/conventions.md** vs **CODING_STYLE.md** duplicate C++ guidelines
- **docs/glossary.md** is incomplete (only 7 terms for a large project)
- **Module README template** exists but inconsistently applied

---

## Recommended Structure

### Phase 1: Immediate Cleanup (No Code Changes)

#### 1.1 Archive Historical Artifacts

```
docs/
  archive/                          # NEW
    prints/                         # MOVED from docs/prints/
      2025-02-*/
      2025-03-*/
      README.md                     # Explains these are historical
    reviews/                        # MOVED from docs/reviews/
      2025-*/
      README.md
    tasks/                          # MOVED - completed tasks
      done/
        T-0104-*.md
        T-0113-*.md
        ...
```

**Rationale:** Preserves history without cluttering active navigation.

#### 1.2 Consolidate Entry Points

**KEEP:**
- `README.md` (root) - Quick start, build instructions, module health table
- `AGENTS.md` (root) - AI agent working agreement, workflow steps

**MERGE & SIMPLIFY:**
- `docs/README.md` → Becomes `docs/NAVIGATION.md` (clear role: documentation index)
- `docs/agents.md` → DELETE (merge unique content into root AGENTS.md)

**UPDATE:**
- Root `README.md` points to `docs/NAVIGATION.md` for "detailed documentation"
- Root `AGENTS.md` Section 1 says: "Read README.md, then docs/NAVIGATION.md"

#### 1.3 Consolidate Style & Convention Docs

**MERGE:**
- `docs/conventions.md` + `docs/architecture.md` (invariants section) → `CODING_STYLE.md`

**RESULT:**
- `CODING_STYLE.md` - Comprehensive: C++, Python, testing, architectural invariants
- `docs/architecture.md` - Focus ONLY on: module map, data flow, key design decisions

#### 1.4 Streamline Roadmap

**Current problem:** Roadmap shows 9/10 initiatives complete, obscuring active work.

**Solution:**

```markdown
# docs/ROADMAP.md

## Active Work (Q4 2025)

| ID | Intent | Owner | Next Milestone |
|----|--------|-------|----------------|
| AI-002 | Async asset streaming | Assets, Runtime | Cancellation hardening (AI-002.2) |
| RT-006 | IO fuzzing | IO | CI integration (blocked on infra) |

## Recently Completed (Archive after 30 days)

| ID | Intent | Completed | Notes |
|----|--------|-----------|-------|
| DC-004 | Error handling std | 2025-03-17 | See design/error_handling_migration.md |
| AI-001 | Handle validation | 2025-04-30 | Telemetry in debug builds |
| ... | ... | ... | ... |

## Backlog (Prioritized)

### Immediate Next (Sprint Planning)
- [ ] **AN-230** - GPU sampling benchmarks (blocked on CO-170)
- [ ] **AS-320** - Material persistence planning
- [ ] **GE-221+** - Remeshing execution (depends on GE-212 RFP)

### Mid-term (3-6 months)
- [ ] **PY-001** - Core bindings and .pyi stubs
- [ ] **DC-003** - SDL backend implementation (see platform/sdl_backend_checklist.md)

### Long-term / Research
- [ ] Advanced state machines (AN-240)
- [ ] Plugin hot-reload architecture
```

**Key changes:**
- Active work visible immediately
- Completed work auto-archives after sprint
- Backlog clearly prioritized by timeline

#### 1.5 Module Documentation Standard

**Enforce consistent structure:**

```
docs/modules/<name>/
  README.md              # Overview, APIs, examples
  BACKLOG.md             # Replaces ROADMAP.md - clearer name
  <specific-topics>.md   # E.g., backend_checklist.md
```

**Each module README must have:**
1. **Purpose** (2-3 sentences)
2. **Key APIs** (code examples)
3. **How to build/test**
4. **Current status** (link to BACKLOG.md)
5. **Related specs** (links to docs/specs/)

**Module BACKLOG.md structure:**
```markdown
## In Progress
- [ ] Task ID: Description (owner, target date)

## Blocked
- [ ] Task ID: Description (blocker reference)

## Next Up (Prioritized)
- [ ] ...

## Ideas / Future Work
- ...
```

---

### Phase 2: Structural Improvements (Requires Review)

#### 2.1 Create Clear Agent Entry Flow

**docs/NAVIGATION.md** (replaces docs/README.md):

```markdown
# Documentation Navigation for AI Agents

## 🎯 Start Here Workflow

**First-time or general questions?**
1. Read [`../README.md`](../README.md) - workspace overview, build steps, module health
2. Read [`../AGENTS.md`](../AGENTS.md) - your working agreement and workflow rules
3. Return here for specialized documentation

**Working on a specific task?**
1. Check [`ROADMAP.md`](ROADMAP.md) - is your work active, backlog, or blocked?
2. Find your module in [`modules/<name>/README.md`](modules/) - understand the subsystem
3. Read related specs in [`specs/`](specs/) - understand constraints
4. Follow [`prompts/implementation-playbook.md`](prompts/implementation-playbook.md)

**Need architectural context?**
1. [`architecture.md`](architecture.md) - module boundaries, data flow, invariants
2. [`specs/ADR-*.md`](specs/) - binding decisions

## 📁 Directory Guide

| Directory | Purpose | When to Use |
|-----------|---------|-------------|
| `specs/` | ADRs, RFPs - binding architectural decisions | Before designing new features |
| `design/` | Deep dives, guides, strategies | Understanding complex subsystems |
| `modules/` | Per-module README + backlog | Working in specific subsystem |
| `tasks/` | Active sprint work, acceptance criteria | Implementing assigned work |
| `prompts/` | Reusable agent instruction templates | Standarizing AI workflows |
| `archive/` | Historical artifacts (prints, reviews, completed tasks) | Historical research only |

## 🔧 Common Tasks

### Implementing a Feature
→ [`prompts/implementation-playbook.md`](prompts/implementation-playbook.md)

### Reviewing Code
→ [`prompts/review-checklist.md`](prompts/review-checklist.md)

### Refactoring
→ [`prompts/refactor-playbook.md`](prompts/refactor-playbook.md)

### Architecture Audit
→ [`prompts/architecture-audit.md`](prompts/architecture-audit.md)

## 📊 Key References

- **Coding Standards:** [`../CODING_STYLE.md`](../CODING_STYLE.md)
- **Telemetry:** [`design/telemetry_schema.md`](design/telemetry_schema.md), [`design/telemetry_instrumentation_guide.md`](design/telemetry_instrumentation_guide.md)
- **Error Handling:** [`design/error_handling_migration.md`](design/error_handling_migration.md)
- **Resource Management:** [`design/resource_management.md`](design/resource_management.md)

---

**Documentation Health Check:**
- Run `python scripts/validate_docs.py` after editing
- Update this file when adding new directories or major documents
```

#### 2.2 Expand Glossary

Current glossary has only 7 terms. Should be comprehensive reference.

**Proposed expansion:**
- All initiative IDs (DC-*, AI-*, RT-*, CC-*, etc.) with one-line descriptions
- Key technical terms (Frame Graph, RuntimeHost, Spatial Index, etc.)
- Module names and responsibilities
- Acronyms (ADR, RFP, MVP, etc.)

#### 2.3 Module Backlog Integration

**Problem:** Module ROADMAPs duplicate central ROADMAP, causing sync issues.

**Solution:** 
- Central ROADMAP tracks **cross-cutting initiatives only**
- Module BACKLOGs track **module-specific work**
- Clear ownership: If it affects 2+ modules → central ROADMAP; if 1 module → module BACKLOG

---

## Implementation Plan

### Week 1: Archive & Consolidate
- [ ] Create `docs/archive/` and move `prints/`, `reviews/`, completed tasks
- [ ] Merge `docs/agents.md` unique content into `AGENTS.md`, delete file
- [ ] Rename `docs/README.md` → `docs/NAVIGATION.md`, rewrite as directory index
- [ ] Update root `README.md` and `AGENTS.md` to reference new structure

### Week 2: Roadmap & Module Cleanup
- [ ] Restructure `docs/ROADMAP.md` with Active/Completed/Backlog sections
- [ ] Move completed initiatives to "Recently Completed" with archive date
- [ ] Rename `modules/*/ROADMAP.md` → `modules/*/BACKLOG.md`
- [ ] Audit all 13 modules for README template compliance

### Week 3: Style Guide Consolidation
- [ ] Merge conventions + architecture invariants into `CODING_STYLE.md`
- [ ] Trim `docs/architecture.md` to focus on module map + data flow
- [ ] Expand `docs/glossary.md` to 50+ terms

### Week 4: Validation & Documentation
- [ ] Update all cross-references for moved/renamed files
- [ ] Run `scripts/validate_docs.py` and fix broken links
- [ ] Create `docs/CHANGELOG.md` documenting restructure
- [ ] Update agent prompts to reference new structure

---

## Success Criteria

### Quantitative
- [ ] Reduce top-level `docs/` file count from 20+ to ~12
- [ ] All modules have consistent README structure (13/13)
- [ ] Zero broken links (`validate_docs.py` passes)
- [ ] Glossary has 50+ terms

### Qualitative
- [ ] AI agent can determine "what to read first" in <30 seconds
- [ ] Active roadmap items visible without scrolling past completed work
- [ ] Single source of truth for each concern (no duplicate guidance)
- [ ] Historical artifacts clearly separated from active docs

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Breaking existing AI agent prompts | Update prompts in same commit as restructure |
| Lost historical context | Archive, don't delete; add archive/README.md |
| Merge conflicts during transition | Communicate freeze period, batch updates |
| Incomplete module compliance | Phase in over 2 sprints, not all at once |

---

## Open Questions

1. **Archive retention:** Keep indefinitely or delete after 1 year?
2. **Module roadmaps:** Rename to BACKLOG.md or different name?
3. **Glossary maintenance:** Auto-generate from code comments or manual?
4. **Sprint numbering:** Replace "Sprint 1, 2, 3" with calendar dates?

---

## Approval & Next Steps

**Reviewers:** @alex (human), AI agents via architecture-audit prompt  
**Approval needed before:** Phase 1.1 (archiving)  
**Timeline:** 4 weeks for full implementation  
**Rollback plan:** Git revert; archive is non-destructive

---

**After approval, create tracking tasks:**
- [ ] `T-NNNN-docs-archive-historical-artifacts.md`
- [ ] `T-NNNN-docs-consolidate-entry-points.md`
- [ ] `T-NNNN-docs-streamline-roadmap.md`
- [ ] `T-NNNN-docs-module-compliance-audit.md`

