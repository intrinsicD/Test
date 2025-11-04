# Hybrid Workflow Migration Guide

> **Purpose:** Provide a deterministic path for porting legacy backlog entries into the metadata-driven hybrid workflow while preserving historical context and quality gates.

---

## 1. Prerequisites

Before migrating, collect the following artefacts:

- Original backlog file(s) and supporting documents (task brief, context package, quality report).
- Latest status recorded in `docs/ROADMAP.md` and any module-specific READMEs.
- Evidence of completed gates (test output, telemetry snapshots, review notes).
- Owners for each gate to confirm sign-off during migration.

---

## 2. Migration Checklist

Use this sequence for every task you migrate. Record progress in the task's **Steps** section.

1. **Snapshot legacy state.** Archive the original backlog/documentation by moving it to `docs/archive/` (if not already captured) and note relevant links.
2. **Create hybrid task file.** Copy [`backlog/000-template.md`](./backlog/000-template.md) to `hybrid_workflow/backlog/<ID>-<kebab-title>.md` and fill in the YAML frontmatter.
3. **Port context.** Translate the legacy task narrative into the new sections (`Intent`, `Context`, `Design/Plan`, etc.), preserving references and decisions.
4. **Transcribe evidence.** Move test outputs, benchmarks, and validation logs into the **Evidence** section. Include command snippets or telemetry summaries as fenced code blocks.
5. **Update gates.** Ensure the `gates:` list matches the original definition of done and that each gate's owner is identified in the log or linked documents.
6. **Synchronise roadmap.** Update `docs/ROADMAP.md` (or relevant bundle file) so that the task's status, priority, and relationships align with the new metadata.
7. **Validate links.** Run `python scripts/validate_docs.py` to confirm cross-references remain intact.
8. **Log migration.** Add an entry under **Steps** describing when and why the migration occurred, including any deviations from the legacy process.
9. **Archive legacy duplicates.** Once the hybrid entry is authoritative, move superseded backlog files into `hybrid_workflow/backlog/archive/` or `docs/archive/` as appropriate.

---

## 3. Automation Support

The hybrid workflow includes utility scripts that summarise backlog metadata. Use them to verify migration completeness and to surface follow-up work.

### 3.1 Status Report Script

```bash
python -m scripts.workflow.report_hybrid_status
```

Default output groups active tasks by status and priority. Use command-line flags for targeted audits:

- `--status ready` to list tasks prepared for pickup.
- `--priority P1` to focus on highest urgency work.
- `--include-archived` to include files under `hybrid_workflow/backlog/archive/` for historical checks.

### 3.2 Ad-hoc Queries

The YAML frontmatter enables lightweight shell queries when scripting is unnecessary:

```bash
# List all tasks missing an owner
rg -l "^owner:\s*$" hybrid_workflow/backlog

# Inspect metadata headers quickly
rg "^id:|^status:|^priority:" hybrid_workflow/backlog/*.md
```

---

## 4. Quality Assurance

After migrating each task:

1. Re-run relevant tests or reference stored evidence to ensure reproducibility.
2. Update affected documentation (module READMEs, specs) to reflect the canonical source of truth.
3. Capture the migration in the task's **Completion Checklist** and mark the status as `done` when quality gates are satisfied.
4. Move the file to `hybrid_workflow/backlog/archive/` once all follow-ups are addressed.

---

## 5. Audit Log Template

Append the following snippet to the **Steps** section of migrated tasks to ensure uniform audit trails:

```markdown
- [x] YYYY-MM-DD — Migrated to hybrid workflow (agent-name)
- [ ] YYYY-MM-DD — Follow-up action (owner)
```

This structure keeps migrations discoverable and pairs with the status report script for accountability.

---

## 6. Future Enhancements

- Automate roadmap synchronisation from task metadata.
- Integrate hybrid workflow dashboards into CI artefacts.
- Expand validation tooling to check for missing evidence per declared gate.

Contributions toward these enhancements should reference Task `DC-050` and spawn dedicated backlog entries under `hybrid_workflow/backlog/`.
