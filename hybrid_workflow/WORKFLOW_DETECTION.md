# How the Hybrid Workflow Detects What to Work On

**Date:** 2025-11-06  
**Summary:** Explanation of task selection, prioritization, and workflow automation

---

## Quick Answer

The hybrid workflow detects what to work on through:

1. **Metadata-driven task files** — Each task has YAML frontmatter with `status`, `priority`, `owner`, etc.
2. **Automated status script** — `python -m scripts.workflow.report_hybrid_status` parses task metadata
3. **Smart task selection** — `--next-actions` flag selects highest-priority `ready` tasks (or `new` if none ready)
4. **Manual roadmap review** — Agents follow `docs/ROADMAP.md` for bundle-based priorities

---

## The Detection System

### 1. Task Metadata (YAML Frontmatter)

Every task file in `hybrid_workflow/backlog/*.md` starts with structured metadata:

```yaml
---
id: RT-410
title: Runtime stage planner & presentation loop
status: in_progress       # ← Lifecycle state
priority: P1              # ← Urgency (P0 = highest)
area: runtime
size: L
owner: runtime-lead
gates: [tests, perf, docs]
relates_to: [bundle:B]    # ← Roadmap bundle
blocked_on: []            # ← Dependencies
links: []
---
```

**Key Fields for Detection:**
- `status` — Current lifecycle state (`new`, `ready`, `in_progress`, `review`, `done`, `archived`)
- `priority` — Urgency level (`P0` to `P5`, where `P0` is highest)
- `owner` — Who is responsible
- `blocked_on` — What dependencies must be resolved first
- `relates_to` — Which roadmap bundle(s) this belongs to

---

### 2. Automated Status Script

**Script:** `scripts/workflow/report_hybrid_status.py`

**How it works:**

1. **Scans** `hybrid_workflow/backlog/` for `*.md` files
2. **Parses** YAML frontmatter from each file
3. **Filters** tasks by status, priority, owner, or bundle
4. **Sorts** by status order → priority order → ID
5. **Outputs** table or JSON

**Example commands:**

```bash
# Show all tasks
python -m scripts.workflow.report_hybrid_status

# Show next 3 tasks to work on
python -m scripts.workflow.report_hybrid_status --next-actions --limit 3

# Show tasks in a specific status
python -m scripts.workflow.report_hybrid_status --status ready

# Show high-priority tasks
python -m scripts.workflow.report_hybrid_status --priority P1

# Show tasks in a specific bundle
python -m scripts.workflow.report_hybrid_status --relates-to bundle:A

# Output as JSON for automation
python -m scripts.workflow.report_hybrid_status --format json
```

---

### 3. Task Selection Algorithm

**Function:** `select_next_actions()` in `report_hybrid_status.py`

**Algorithm:**

```python
def select_next_actions(tasks, limit):
    # 1. Try to find ready tasks
    ready = [task for task in tasks if task.status == "ready"]
    
    # 2. If no ready tasks, fall back to new tasks
    pool = ready if ready else [task for task in tasks if task.status == "new"]
    
    # 3. Sort by priority, then ID
    ordered = sorted(pool, key=lambda t: (priority_rank(t.priority), t.identifier))
    
    # 4. Return top N
    return ordered[:limit]
```

**Priority Ranking:**
- `P0` = 0 (highest)
- `P1` = 1
- `P2` = 2
- `P3` = 3
- Unknown priorities = lowest

**Status Order (for sorting):**
1. `new` — Not yet started
2. `ready` — Ready to implement (groomed, scoped, dependencies clear)
3. `in_progress` — Currently being worked on
4. `review` — In code review
5. `done` — Complete, not yet archived
6. `archived` — Moved to `backlog/archive/`

---

### 4. Workflow Integration (AGENTS.md)

**Step 1: Select Task** from AGENTS.md:

> - Start with [`docs/ROADMAP.md`](../docs/ROADMAP.md) and pick the highest-priority item marked `status: ready`.
> - If nothing is ready, groom the top `new` backlog item under `hybrid_workflow/backlog/` and mark it `ready` once scoped.
> - Check the task's `blocked_on` field to ensure dependencies are clear.

**Translation:**
1. Run `python -m scripts.workflow.report_hybrid_status --next-actions`
2. Pick the first `ready` task (or first `new` if none ready)
3. Verify `blocked_on` is empty before starting
4. Update task `status: ready` → `status: in_progress`

---

## Example Detection Flow

### Scenario 1: Agent Starting Work

```bash
# Agent asks: "What should I work on?"
$ python -m scripts.workflow.report_hybrid_status --next-actions --limit 3
```

**Output:**
```
Status  Priority  ID      Owner         Title                        File
======  ========  ======  ============  ===========================  ===================
ready   P1        RG-451  rendering     Shader pipeline compiler     backlog/RG-451...md
ready   P2        TL-311  tools-lead    Scene hierarchy panel        backlog/TL-311...md
new     P1        RT-415  runtime-lead  Physics integration hooks    backlog/RT-415...md
```

**Agent decision:**
- Choose `RG-451` (highest priority, status ready)
- Open `hybrid_workflow/backlog/RG-451-shader-pipeline-compiler.md`
- Check `blocked_on: []` — no blockers
- Update `status: ready` → `status: in_progress`
- Begin work following AGENTS.md workflow

---

### Scenario 2: Filtering by Bundle

```bash
# Product Manager asks: "What's left in Bundle A?"
$ python -m scripts.workflow.report_hybrid_status --relates-to bundle:A
```

**Output:**
```
Status       Priority  ID      Title                         
===========  ========  ======  ============================
in_progress  P1        RG-450  Modular render pipeline      
review       P1        RG-450-A Node descriptor API
```

**PM decision:**
- 2 tasks active in Bundle A
- 1 in progress, 1 in review
- Bundle A nearly complete

---

### Scenario 3: Finding Blockers

```bash
# Agent asks: "Why is TL-310 not ready?"
$ grep -A 5 "blocked_on:" hybrid_workflow/backlog/TL-310-editor-foundations.md
```

**Output:**
```yaml
blocked_on: ["RT-410"]
```

**Agent decision:**
- TL-310 blocked by RT-410
- Check RT-410 status: `in_progress`
- Work on something else until RT-410 completes
- RT-410 owner should update when unblocking

---

## Detection at Different Levels

### Level 1: Manual (Human/Agent)

**Read ROADMAP.md:**
- See bundle priorities (A, B, C, D)
- See which tasks are unchecked `[ ]`
- Pick highest-priority bundle with open tasks

**Example:**
```markdown
## Bundle A — GPU Execution (Priority 1)
- [x] T-0120 — GPU resource provider ✓
- [x] T-0119 — Command encoder ✓
- [ ] RG-450 — Modular render pipeline ← Work on this
```

---

### Level 2: Semi-Automated (CLI)

**Run status script:**
```bash
python -m scripts.workflow.report_hybrid_status --next-actions
```

**Agent sees:**
- Top 5 ready tasks (or new if no ready)
- Sorted by priority
- Can immediately start work

---

### Level 3: Fully Automated (JSON)

**Script integration:**
```bash
python -m scripts.workflow.report_hybrid_status --next-actions --format json > tasks.json
```

**Automation reads JSON:**
```json
{
  "tasks": [
    {
      "id": "RG-450",
      "title": "Modular render pipeline planner",
      "status": "ready",
      "priority": "P1",
      "owner": "rendering-lead",
      "file": "hybrid_workflow/backlog/RG-450-modular-render-pipeline.md",
      "relates_to": ["bundle:A"]
    }
  ],
  "counts": {
    "by_status": {"ready": 1},
    "total": 1
  }
}
```

**Automation can:**
- Assign tasks to agents
- Generate work queues
- Trigger CI pipelines
- Update dashboards

---

## Smart Fallbacks

### When No Ready Tasks Exist

**Command:**
```bash
$ python -m scripts.workflow.report_hybrid_status --next-actions
```

**Algorithm:**
1. Look for `status: ready` tasks
2. **If none found:** Fall back to `status: new` tasks
3. Sort by priority
4. Return top N

**Output when no ready:**
```
Status  Priority  ID      Owner         Title
======  ========  ======  ============  ===================
new     P1        RT-415  runtime-lead  Physics integration
new     P2        TL-312  tools-lead    Performance panel
```

**Agent action:**
- Pick highest priority `new` task
- **Groom it:** Fill in Design/Plan section
- **Mark ready:** Update `status: new` → `status: ready`
- Then start work

---

### When No Tasks at All

**Output:**
```
No tasks matched the supplied filters.
Tip: When the ready queue is empty, groom the highest-priority new 
backlog item under hybrid_workflow/backlog/ and mark it ready once scoped.
```

**Agent action:**
1. Check `docs/ROADMAP.md` for strategic priorities
2. Create new task file using `000-template.md`
3. Add to ROADMAP bundle
4. Mark `status: new`
5. Groom and promote to `ready`

---

## Filter Capabilities

### By Status
```bash
python -m scripts.workflow.report_hybrid_status --status in_progress
```
Shows: All actively worked tasks

---

### By Priority
```bash
python -m scripts.workflow.report_hybrid_status --priority P1
```
Shows: All high-priority tasks

---

### By Owner
```bash
python -m scripts.workflow.report_hybrid_status --owner tools-lead
```
Shows: All tasks assigned to tools-lead

---

### By Bundle
```bash
python -m scripts.workflow.report_hybrid_status --relates-to bundle:A bundle:B
```
Shows: Tasks in Bundle A OR Bundle B

---

### Multiple Filters
```bash
python -m scripts.workflow.report_hybrid_status \
    --status ready \
    --priority P1 \
    --relates-to bundle:A
```
Shows: P1 tasks in Bundle A that are ready to start

---

## Dashboard Visualization

**Generate HTML dashboard:**
```bash
python -m scripts.workflow.dashboard --output-dir build/dashboard
```

**Creates:**
- `build/dashboard/index.html` — Interactive task board
- `build/dashboard/tasks.json` — Machine-readable data

**Dashboard shows:**
- Tasks grouped by status (columns)
- Color-coded by priority
- Filterable by bundle, owner, area
- Click to view task files

---

## Integration Points

### With ROADMAP.md
- Tasks have `relates_to: [bundle:A]` metadata
- ROADMAP lists tasks with checkboxes `[ ]` / `[x]`
- When task `status: done`, check ROADMAP box
- Agents reference ROADMAP for strategic context

### With AGENTS.md Workflow
- **Step 1 (Select Task):** Use `--next-actions`
- **Step 6 (Complete):** Update `status: done`, move to archive
- **Step 7 (Docs Sync):** Update ROADMAP checkboxes

### With Git Branches
- Branch names match task IDs: `feat/RG-450-render-pipeline`
- PR references task file path
- Easy to correlate commits → tasks → roadmap

---

## Detection Best Practices

### For Agents
1. **Always run `--next-actions`** before starting work
2. **Check `blocked_on`** field before claiming a task
3. **Update status immediately** when starting/finishing
4. **Keep metadata current** (owner, priority, status)

### For Product Managers
1. **Use bundle filters** to track strategic work
2. **Monitor priority distribution** (too many P1s?)
3. **Review blocked tasks weekly** to unblock teams
4. **Groom new tasks** to keep ready queue full

### For Automation
1. **Parse JSON output** for programmatic access
2. **Filter by multiple criteria** for targeted queries
3. **Monitor status counts** for health metrics
4. **Generate alerts** when ready queue empty

---

## Summary

**The hybrid workflow detects what to work on through:**

✅ **Structured metadata** in task file frontmatter  
✅ **Automated parsing** via Python script  
✅ **Smart prioritization** (status → priority → ID)  
✅ **Flexible filtering** (status, priority, owner, bundle)  
✅ **Intelligent fallbacks** (ready → new if empty)  
✅ **Multiple interfaces** (CLI, JSON, dashboard)  
✅ **Manual guidance** (ROADMAP.md for context)

**Key command to remember:**
```bash
python -m scripts.workflow.report_hybrid_status --next-actions
```

This gives you the top tasks to work on, sorted by priority, with automatic fallback to grooming when the ready queue is empty.

---

**See Also:**
- `hybrid_workflow/AGENTS.md` — Full workflow guide
- `hybrid_workflow/ROADMAP.md` — Strategic bundles and priorities  
- `scripts/workflow/report_hybrid_status.py` — Detection script source
- `scripts/workflow/dashboard.py` — Dashboard generator

