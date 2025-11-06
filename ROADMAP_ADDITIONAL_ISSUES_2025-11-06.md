# Additional Roadmap Issues & Recommendations — 2025-11-06

## Issues Fixed in This Pass

### 1. ✅ Bundle D Priority Clarification
**File:** `hybrid_workflow/ROADMAP.md`

**Problem:** Bundle D labeled "Priority: 0" without explanation, causing confusion about whether it blocks technical work.

**Fix Applied:** Added clarifying note:
```markdown
**Priority:** P0 (Process/Coordination — runs parallel to technical bundles)

_Note: While Bundle D tasks have P0 priority for process coordination, they run 
in parallel with Bundles A-C technical work and don't block GPU/runtime/tooling execution._
```

**Impact:** Clarifies that P0 is for process priority, not technical blocking priority.

---

### 2. ⚠️ RG-450 → RT-410 Dependency Flagged for Review
**File:** `hybrid_workflow/backlog/RG-450-modular-render-pipeline.md`

**Problem:** RG-450 (Bundle A, rendering/frame-graph) depends on RT-410 (Bundle B, runtime/presentation) with no explanation in task context.

**Fix Applied:** Added inline comment in frontmatter:
```yaml
blocked_on: ["RT-410"]  # NOTE: Dependency needs architectural review - unclear why 
                         # render pipeline planner requires runtime stage planner
```

**Rationale:**
- RG-450 is about frame-graph node composition (render passes, resources)
- RT-410 is about runtime main loop and presentation adapters
- These seem like orthogonal concerns that could proceed in parallel
- No mention of RT-410 in RG-450's context, references, or design sections

**Recommendation:** Review with rendering-lead and runtime-lead to determine if:
1. They can work in parallel (remove blocker)
2. There's a real architectural dependency (document it in RG-450 context)
3. The dependency is on shared infrastructure that should be a separate task

---

## Remaining Issues (Not Fixed)

### 3. ⚠️ SPRINT-11 Incomplete but in Review

**Task:** SPRINT-11 — Sprint 11 alignment  
**Status:** `review`  
**Problem:** Multiple unchecked acceptance criteria and completion checklist items

**Incomplete Items:**
- [ ] `AI-004` kickoff brief updated with agenda, timeline, and risk owners
- [ ] Roadmap kickoff timeline reflects milestone sequencing and dependencies
- [ ] Harness smoke test recorded and linked from `RT-320` backlog entry
- [ ] Dataset manifest validated against harness dry-run and sandbox preview
- [ ] Sprint ledger captures scope, owners, and cross-links to kickoff packet
- [ ] Streams report outcomes with artefacts
- [ ] Roadmap and kickoff brief reference this sprint entry
- [ ] Documentation validation recorded in Evidence
- [ ] Status moved to `done` after kickoff review consumes sprint outputs

**Impact:** SPRINT-11 blocks AI-004 (P0), but isn't complete

**Recommendation:**
- Either complete the checklist items and mark `done`, OR
- Change status back to `in_progress` until items are complete, OR
- If items are no longer relevant, update the checklist and mark `done`

---

### 4. ⚠️ No Ready Tasks Pipeline

**Problem:** Zero tasks with `status: ready` or `status: new`

**Current State:**
```
in_progress: 5 tasks
review:      1 task
ready:       0 tasks  ← Problem
new:         0 tasks  ← Problem
```

**Impact:** 
- Agents have no clear next actions to pick up
- `--next-actions` filter returns empty
- Reduces workflow parallelism

**Recommendation:** Break down large tasks into subtasks:

**Example for RT-410 (size: L):**
Could be broken into:
- RT-410-A: Stage planner API design (ready)
- RT-410-B: Presentation backend abstraction (ready)
- RT-410-C: OpenGL presentation adapter (blocked on A, B)
- RT-410-D: Vulkan presentation adapter (blocked on A, B)
- RT-410-E: Integration tests (blocked on C, D)

**Example for RG-450 (size: L):**
Could be broken into:
- RG-450-A: Node descriptor API (ready)
- RG-450-B: Resource registry (ready)
- RG-450-C: Dependency resolver (blocked on A, B)
- RG-450-D: Transient allocator (blocked on B)
- RG-450-E: Barrier injection (blocked on C)

---

### 5. ⚠️ Bundle D Tasks Not in Central Roadmap

**Problem:** AI-004 and SPRINT-11 appear in `hybrid_workflow/ROADMAP.md` but not in `docs/ROADMAP.md`

**Impact:** Inconsistency between the two roadmap files

**Recommendation:** 
- Add Bundle D section to `docs/ROADMAP.md`, OR
- Add note to `docs/ROADMAP.md` explaining that process/coordination tasks live only in `hybrid_workflow/ROADMAP.md`

---

### 6. ⚠️ Outdated Task References in SPRINT-11

**Problem:** SPRINT-11 references tasks that don't exist in hybrid_workflow format:
- RT-320 (exists only in old docs/backlog/archive format)
- AS-330 (exists only in old docs/backlog/archive format)
- TL-210 (exists only in old docs/backlog/archive format)

**Impact:** Cross-references are inconsistent (some use old underscore format, some use new dash format)

**Recommendation:**
- Update SPRINT-11 to reference archived tasks correctly with full paths, OR
- Note that SPRINT-11 references historical Phase 1-3 work, now complete

---

### 7. ℹ️ Future Dates in Task Metadata

**Minor Issue:** DC-041 completion notes reference "2026-02-03" (future date)

**Impact:** Suggests test/fixture data or placeholder dates

**Recommendation:** Low priority - update to actual completion dates when known, or clarify these are target dates

---

## Summary of Changes Made

| File | Change | Type |
|------|--------|------|
| `hybrid_workflow/ROADMAP.md` | Clarified Bundle D priority as process/coordination | Fix |
| `hybrid_workflow/backlog/RG-450-modular-render-pipeline.md` | Added comment flagging RT-410 dependency for review | Flag |

---

## Recommended Next Actions

### High Priority (This Week)

1. **Complete or update SPRINT-11** 
   - Review checklist items
   - Mark complete or move back to in_progress
   - This will unblock AI-004 (P0)

2. **Review RG-450 ↔ RT-410 dependency**
   - Consult rendering-lead and runtime-lead
   - Either document rationale in RG-450 context, or remove blocker
   - Could unblock significant P1 work

3. **Create ready tasks**
   - Break down RT-410 into smaller subtasks
   - Break down RG-450 into smaller subtasks
   - Mark at least 2-3 tasks as `ready` for agents to pick up

### Medium Priority (Next Sprint)

4. **Sync central roadmap**
   - Add Bundle D to `docs/ROADMAP.md` or explain exclusion

5. **Update SPRINT-11 references**
   - Fix cross-references to use correct archived task paths

6. **Clean up dates**
   - Update future dates in task metadata to actuals or targets

---

## Validation

Run these commands to verify current state:

```bash
# Check all tasks
python hybrid_workflow/task_status.py

# Check blocked tasks (should be 3)
python hybrid_workflow/task_status.py --blocked

# Check unblocked tasks (should be 3: SPRINT-11, RT-410, PM-510)
python hybrid_workflow/task_status.py --unblocked

# Check for ready tasks (currently returns 0)
python -m scripts.workflow.report_hybrid_status --status ready

# Validate documentation links
python scripts/validate_docs.py
```

---

**Review completed:** 2025-11-06  
**Additional issues found:** 5  
**Issues fixed:** 2  
**Status:** 🟡 Improved but needs follow-up on SPRINT-11, RG-450 dependency, and ready task pipeline

