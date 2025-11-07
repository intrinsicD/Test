# Hybrid Workflow — Quick Reference

## 🚀 Quick Start (30 seconds)

1. Read: [`AGENTS.md`](./AGENTS.md) — The 7-step workflow
2. Use: [`backlog/000-template.md`](./backlog/000-template.md) — Task template
3. Check: [`ROADMAP.md`](./ROADMAP.md) — Pick a bundle
4. Run: `python -m scripts.workflow.report_hybrid_status --next-actions` — Surface highest-priority ready work (falls back to `new`).

---

## 📋 Task Lifecycle

```
new → ready → in_progress → review → done → archived
```

Update status in task frontmatter as you progress.

---

## 7 Steps

| # | Step | What | Where |
|---|------|------|-------|
| 1 | **Select** | Pick highest-priority `ready` task | `ROADMAP.md` |
| 2 | **Plan** | Load context, design solution | Task file Design section |
| 3 | **Implement** | Code following standards | `CONTRIBUTING.md` |
| 4 | **Test** | Run canonical test stack | Command reference below |
| 5 | **Review** | Open PR with task reference | GitHub/GitLab |
| 6 | **Complete** | Archive task, update ROADMAP | Move to `backlog/archive/` |
| 7 | **Sync** | Validate docs, update navigation | `validate_docs.py` |

---

## 📝 Create a Task

```bash
# Copy template
cp hybrid_workflow/backlog/000-template.md hybrid_workflow/backlog/NNN-my-task.md

# Edit frontmatter
# ---
# id: NNN
# title: What you're doing
# status: new
# priority: P1
# area: rendering
# size: M
# owner: your-name
# gates: [tests]
# relates_to: [bundle:A]
# blocked_on: []
# links: []
# ---

# Fill in sections:
# - Intent (one sentence)
# - Context (why now?)
# - Design/Plan (how?)
# - Steps (what order?)
```

---

## 🧪 Test Stack (Step 4)

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

Paste output in task's **Evidence** section.

---

## 🔍 Query Tasks

```bash
# Quick next actions
python -m scripts.workflow.report_hybrid_status --next-actions
python -m scripts.workflow.report_hybrid_status --next-actions --limit 10
python hybrid_workflow/task_status.py --next-actions
python hybrid_workflow/task_status.py --next-actions --limit 3 --owner tools-lead

# Filter by roadmap bundle metadata (automation)
python -m scripts.workflow.report_hybrid_status --relates-to bundle:C
python -m scripts.workflow.report_hybrid_status --relates-to bundle:A bundle:D

# List all tasks
python hybrid_workflow/task_status.py

# Filter by status
python hybrid_workflow/task_status.py --status ready
python hybrid_workflow/task_status.py --status in_progress

# Filter by priority
python hybrid_workflow/task_status.py --priority P1

# Filter by area
python hybrid_workflow/task_status.py --area rendering

# Filter by owner
python hybrid_workflow/task_status.py --owner docs-devrel

# Filter by roadmap bundle metadata
python hybrid_workflow/task_status.py --relates-to bundle:D
python hybrid_workflow/task_status.py --relates-to bundle:A bundle:C

# Focus on blockers / unblocked work
python hybrid_workflow/task_status.py --blocked
python hybrid_workflow/task_status.py --blocked --status in_progress
python hybrid_workflow/task_status.py --unblocked
python hybrid_workflow/task_status.py --unblocked --priority P1

# Show summary (respects filters)
python hybrid_workflow/task_status.py --summary
python hybrid_workflow/task_status.py --status in_progress --summary

# Include archived records for retrospectives
python hybrid_workflow/task_status.py --include-archived
python hybrid_workflow/task_status.py --include-archived --summary

# Show details
python hybrid_workflow/task_status.py --detail T-0120

# Or use grep
grep -l "^status: ready" hybrid_workflow/backlog/*.md
grep -l "^priority: P1" hybrid_workflow/backlog/*.md
```

---

## ✅ Quality Gates

Declare in frontmatter: `gates: [tests, perf, docs, safety, release]`

| Gate | When | Evidence |
|------|------|----------|
| `tests` | Always | Test output, coverage |
| `perf` | Performance-sensitive code | Benchmarks, telemetry |
| `docs` | User-facing changes | Updated docs list |
| `safety` | Security/memory concerns | Sanitizer logs |
| `release` | Shipping features | Changelog, packaging |

---

## 📚 Context Ladder (Step 2)

Load these in order before coding:

1. `README.md` — Workspace overview
2. `docs/NAVIGATION.md` — Doc index
3. `docs/ROADMAP.md` — Priorities
4. Task file — Your acceptance criteria
5. `docs/modules/<module>/README.md` — Module invariants
6. `docs/specs/ADR-*.md` — Architecture decisions
7. `docs/archive/` — Historical context

---

## 🎯 When to Use What

| Task Complexity | What You Need |
|-----------------|---------------|
| **Simple** (<2 days, solo) | Task file only |
| **Medium** (>2 days, multi-person) | Task file + optional Task Brief |
| **Complex** (>1 week, high risk) | Task file + Task Brief + Context Package + Quality Report |

Templates for complex work: `agents/TEMPLATES/`

---

## 🏷️ Frontmatter Fields

```yaml
id: NNN                  # Unique task ID
title: Short title       # Imperative, concise
status: new              # new|ready|in_progress|review|done|archived
priority: P1             # P0|P1|P2|P3 (lower = higher priority)
area: rendering          # Module/domain tag
size: M                  # XS|S|M|L|XL
owner: agent             # Who's responsible
gates: [tests]           # Required quality gates
relates_to: [bundle:A]   # ROADMAP bundle tags
blocked_on: []           # Dependencies (empty if none)
links: []                # Related PRs, ADRs, docs
```

---

## 📊 ROADMAP Bundles

- **Bundle A** — GPU Execution (P1)
- **Bundle B** — Presentation & Tooling (P2)
- **Bundle C** — Documentation & Infrastructure (P3)

Link tasks with `relates_to: [bundle:A]`

---

## 🔀 Branch Naming

```bash
feat/NNN-short-title     # New feature
fix/NNN-short-title      # Bug fix
refactor/NNN-short-title # Code cleanup
```

Match your task ID (NNN).

---

## ✍️ Commit Messages

```
Imperative mood, mention task ID

Examples:
- "Implement GPU resource provider [T-0120]"
- "Fix buffer leak in hot reload [T-0120]"
- "Add retention window config [T-0120]"
```

---

## 🚦 Status Meanings

| Status | Meaning |
|--------|---------|
| `new` | Identified but not scoped |
| `ready` | Scoped, ready to start |
| `in_progress` | Active work happening |
| `review` | PR open, awaiting review |
| `done` | Merged, not yet archived |
| `archived` | Completed and moved to archive/ |

---

## 🤝 Role Coordination

For complex tasks, use roles from `agents/ROLES.md`:

- Agent Orchestrator
- Product Manager
- Knowledge Librarian
- Specialist Engineer(s)
- Docs/DevRel
- QA/Test Specialist
- Performance Engineer
- Safety Reviewer
- Reviewer
- Release Manager

Simple tasks don't need formal roles.

---

## 🛠️ File Locations

```
hybrid_workflow/
├── AGENTS.md              ← Workflow guide (READ THIS FIRST)
├── CONTRIBUTING.md        ← Code standards
├── ROADMAP.md             ← Bundles and priorities
├── README.md              ← Overview
├── task_status.py         ← Query tool
└── backlog/
    ├── 000-template.md    ← Template (copy this)
    ├── NNN-task.md        ← Your tasks
    └── archive/           ← Completed tasks

../agents/
├── ROLES.md               ← Role definitions
└── TEMPLATES/
    ├── TASK_BRIEF_TEMPLATE.md
    ├── CONTEXT_PACKAGE_TEMPLATE.md
    └── QUALITY_REPORT_TEMPLATE.md
```

---

## 💡 Examples

- **Complex task:** `backlog/T-0120-gpu-resource-provider.md`
- **Simple task:** `backlog/DC-050-workflow-migration.md`
- **Template:** `backlog/000-template.md`

---

## ❓ Common Questions

**Q: Do I need to fill every template section?**  
A: No. Skip sections that don't apply. Simple tasks need less detail.

**Q: When do I create separate artifacts?**  
A: Only for complex tasks requiring multi-role coordination. Most tasks use the task file only.

**Q: How do I mark a task blocked?**  
A: Add to `blocked_on: ["reason or dependency"]` in frontmatter.

**Q: Can I change frontmatter after starting?**  
A: Yes! Update status, owner, etc. as work progresses.

**Q: What if I'm working solo?**  
A: Still use the task file for structure and evidence, but skip role coordination.

---

## 📞 Help

- Workflow questions → `AGENTS.md`
- Code standards → `CONTRIBUTING.md`
- Task format → `backlog/000-template.md`
- Roles → `../agents/ROLES.md`

---

**Print this and keep it handy!** 📎

_Quick reference v1.0 (2025-11-04)_

