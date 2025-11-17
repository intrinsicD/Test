---
id: TL-349
title: Task-status search covers blocked_on metadata
status: done
priority: P3
area: workflow
size: XS
owner: docs-devrel
gates: [tests, docs]
relates_to: [bundle:C]
blocked_on: []
links:
  - hybrid_workflow/task_status.py
  - scripts/workflow/report_hybrid_status.py
---

# Task TL-349 — Task-status search covers blocked_on metadata

## Intent

Extend the hybrid workflow task-status utilities so `--search` filters can
locate tasks by blocker identifiers or tracking links without juggling separate
filters.

## Context

* Contributors frequently search for blocked tasks by dependency identifier
  (e.g. `TL-310`) when triaging the backlog. The CLI previously ignored
  `blocked_on` and `links` metadata, so `--search TL-310` returned nothing even
  when several tasks were blocked on TL-310.
* Both the lightweight `hybrid_workflow/task_status.py` entry point and the
  richer `scripts.workflow.report_hybrid_status` CLI share the same filtering
  helpers, so the gap affected both workflows and their downstream automation.

## Design / Plan

1. Update the search haystack to include `blocked_on` and `links` strings so
   keyword filtering surfaces blockers and reference links alongside owner/area
   metadata.
2. Keep the implementation centralised inside `filter_tasks` to benefit both
   CLIs automatically.
3. Propagate `links` through `TaskMetadata` so `scripts.workflow.report_hybrid_status`
   preserves the new metadata and exposes it via JSON rendering.
4. Document the expanded search scope directly in the task-status usage block
   so future operators know about the capability.

## Steps

1. [x] Expand `Task` search haystack with `blocked_on` and `links` metadata and
       update the usage comment.
2. [x] Extend `TaskMetadata` (plus helpers and JSON serialization) with `links`
       so the `scripts.workflow.report_hybrid_status` CLI retains parity.
3. [x] Add regression tests for both CLIs ensuring search matches blocker/link
       metadata.
4. [x] Archive the task with test evidence and roadmap alignment.

## Evidence

### Test Results

```bash
pytest python/tests/test_hybrid_workflow_task_status.py -q
pytest scripts/tests/test_report_hybrid_status.py -q
```

### Documentation

* Updated the `hybrid_workflow/task_status.py` usage block to call out that
  `--search` now includes blocked_on and link metadata.

### Roadmap / Metadata

* Added TL-349 to the Bundle C checklist in `hybrid_workflow/ROADMAP.md`.
* Task archived with `status: done`.
