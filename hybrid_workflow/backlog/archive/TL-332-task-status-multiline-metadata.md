---
id: TL-332
title: Task status CLI multiline metadata parsing
status: done
priority: P3
area: workflow
size: S
owner: docs-devrel
gates: [tests]
relates_to: [bundle:C]
blocked_on: []
links:
  - "hybrid_workflow/task_status.py"
  - "python/tests/test_hybrid_workflow_task_status.py"
---

# Task TL-332 — Task Status CLI Multiline Metadata Parsing

## Intent

Ensure the hybrid workflow task status CLI correctly parses YAML frontmatter lists that span multiple lines so downstream tooling retains accurate metadata.

---

## Context

- The previous parser stored empty strings when metadata such as `links` or `blocked_on` used multi-line YAML list syntax.
- Downstream filters (e.g., `--blocked`) silently missed dependencies, reducing the usefulness of the CLI for roadmap reviews.
- Backlog authors prefer multi-line lists for readability, so losing this metadata forced awkward formatting compromises.

---

## Design / Plan

### Approach

- Extend `parse_frontmatter` to recognise block lists and normalise quoted entries.
- Reuse the same stripping logic inside `parse_list_field` so inline and block lists share behaviour.
- Add regression coverage that exercises `load_task` on a synthetic backlog entry featuring multi-line lists.

### Test Strategy

- Execute the focused Python unit tests for the task status CLI module.

---

## Steps

1. [x] Update `parse_frontmatter` to capture block list values and remove inline comments.
2. [x] Generalise `parse_list_field` so it accepts both string and list inputs from the frontmatter parser.
3. [x] Add regression coverage in `python/tests/test_hybrid_workflow_task_status.py` to confirm multi-line metadata parsing.
4. [x] Sync the workflow roadmap to record TL-332 completion.

---

## Evidence

### Tests

```bash
pytest python/tests/test_hybrid_workflow_task_status.py
```

**Summary:** The targeted unit tests covering task status metadata parsing pass locally, demonstrating the regression is resolved.
