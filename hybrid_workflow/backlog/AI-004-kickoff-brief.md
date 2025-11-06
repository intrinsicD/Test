---
id: AI-004
title: Kickoff brief readiness
status: in_progress
priority: P0
area: program
size: L
owner: agent-orchestrator
gates: [docs]
relates_to: [bundle:D]
blocked_on: []
links:
  - "docs/ROADMAP.md#bundle-d--kickoff-coordination"
  - "docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md"
  - "docs/archive/backlog/legacy/tasks/AI_004_KICKOFF_BRIEF.md"
  - "hybrid_workflow/backlog/archive/SPRINT-11-alignment.md"
---

# Task AI-004 — Kickoff Brief Readiness

## Intent

Produce the AI-004 kickoff packet so roadmap, backlog, sprint tracker, and risk mitigations align on a single source of truth before executive sign-off.

---

## Context

**Current State:**
- Kickoff packet material is dispersed across roadmap timelines, sprint notes, and archived readiness checklists.
- Demo evidence (harness, sandbox, datasets) exists but requires curation to prove readiness to stakeholders.
- Risk mitigation owners are tracked separately, making it easy for due dates to drift.

**Desired State:**
- A curated kickoff brief enumerates agenda, timeline, risks, and demo artefacts with accountable owners.
- Demo baselines (harness run, sandbox wiring, dataset validation) are published with hashes and telemetry pointers.
- Roadmap, sprint tracker, and backlog cross-link this task to keep future updates synchronized.

**References:**
- [`docs/ROADMAP.md#bundle-d--kickoff-coordination`](../docs/ROADMAP.md#bundle-d--kickoff-coordination)
- [`docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md`](../docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md)
- [`docs/archive/backlog/legacy/tasks/AI_004_KICKOFF_BRIEF.md`](../docs/archive/backlog/legacy/tasks/AI_004_KICKOFF_BRIEF.md)
- [`hybrid_workflow/backlog/SPRINT-11-alignment.md`](SPRINT-11-alignment.md)

---

## Design / Plan

### Constraints

- Follow `hybrid_workflow/CONTRIBUTING.md` documentation standards and metadata conventions.
- Keep roadmap milestones in `docs/ROADMAP.md` synchronized with kickoff packet deliverables.
- Preserve mitigation owners and due dates captured in `DC-041` during updates.
- Surface demo evidence that can be reproduced from harness and sandbox tooling.

### Agenda & Scope

- Schema contract recap (`DC-040`) and validator demo.
- Phase 1 milestone review covering harness (`RT-320`), sandbox wiring (`TL-210`), and dataset packaging (`AS-330`).
- Risk mitigation status and owner sign-off.
- Demo readiness checklist and next sync cadence.

### Milestone Sequence

| Sequence | Focus | Backlog | Owner | Dependencies | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Publish kickoff brief cross-links | [`DC-041`](../docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md) | Product Manager | [`DC-040`](../docs/backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md) | Kickoff brief, roadmap timeline, and sprint tracker cross-links published together. |
| 2 | Capture harness dry-run evidence | [`RT-320`](../docs/backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md) | Runtime Lead | `DC-041` | Harness dry-run executes schema sample + telemetry export; smoke test recorded. |
| 3 | Finalise sandbox integration notes | [`TL-210`](../docs/backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md) | Tools Lead | `RT-320` | Sandbox consumes harness presets and exposes dataset selection for review demo. |
| 4 | Validate dataset manifests | [`AS-330`](../docs/backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md) | Assets Lead | `RT-320`, `TL-210` | Package manifests validated against harness + sandbox integration test. |

### Risk Ownership

| Risk | Owner | Mitigation |
| --- | --- | --- |
| Dataset licensing slips and blocks demo dataset selection. | Assets Lead | Secure fallback synthetic dataset and document provenance in `AS-330`. |
| Benchmark automation exceeds CI time budget for kickoff smoke. | Performance Lead | Produce ≤5 min smoke preset leveraging `CC-310` orchestrator options. |
| Harness coverage gaps hide schema regressions. | Runtime Lead | Extend RT-320 smoke to assert schema version + telemetry channels before review. |

### Prep Checklist

- [ ] Circulate slide outline covering agenda, timeline, and risk mitigations (see [Slide Outline Draft](#slide-outline-draft)).
- [ ] Confirm demo scene and dataset availability across runtime, tools, and rendering teams (see [Demo Scene & Dataset Availability Targets](#demo-scene--dataset-availability-targets)).
- [ ] Capture benchmark baseline from harness + sandbox combo for kickoff packet (see [Benchmark Baseline Targets](#benchmark-baseline-targets)).

### Slide Outline Draft

1. **Kickoff Objectives** – revisit AI-004 readiness scorecard, milestone cadence, and success metrics.
2. **Schema Contract Recap** – highlight validator coverage, enforcement flags, and Python/C++ integration points.
3. **Harness Walkthrough** – dry-run demo of `python -m scripts.prototyping.run_prototype_harness --case-study geometry-baseline` showing configuration resolution, dataset validation, and telemetry targets.
4. **Sandbox Wiring** – ImGui layout for preset selection, telemetry overlays, and benchmark trigger entry points (paired with TL-210 follow-ups).
5. **Dataset Packaging** – manifest provenance summary, checksum table, and ingestion automation checkpoints for AS-330.
6. **Benchmark Baseline** – geometry remesh case-study snapshot plus comparative benchmark rollout plan.
7. **Risk Register & Mitigations** – review current owners, mitigation checkpoints, and escalation paths.
8. **Next Steps** – outline upcoming sprint focus, CI enablement, and review cadence through kickoff.

### Demo Scene & Dataset Availability Targets

- **Harness validation** – Plan harness execution of the `geometry-baseline` case study to resolve the `geometry-remesh-baseline` dataset, validate mesh artefacts, and confirm zero asset failures. Capture manifest hashes once the dry-run executes.
- **Rendering preset** – Lock the `research-baseline` preset at 1280×720 deferred shading (overlays disabled) to mirror sandbox defaults in TL-210 and document configuration deltas.
- **Runtime configuration** – Align orbit camera defaults (position `⟨0,0,4⟩`, target `⟨0,0,0⟩`) and simulation cadence (`Δt = 1/60 s`, two max substeps) between sandbox previews and harness runs; record deviations if adjustments are required.

### Benchmark Baseline Targets

Capture the following metrics once the harness + sandbox dry-run completes and include them in the kickoff appendix:

| Metric | Target Capture | Notes |
| --- | --- | --- |
| Scenario | Record selected case study (`geometry-baseline`) | Ensure configuration matches kickoff scope |
| Dataset | Document dataset identifier and manifest hash | Validate against AS-330 package inventory |
| Telemetry target | Confirm telemetry export path | Attach sample to appendix |
| Geometry metrics | Surface area, triangle quality, operations counts | Compare with historical baselines |
| Duration | Dry-run execution time | Flag if >5% variance from prior runs |

### Test Plan

- **Documentation validation:**
  - `python scripts/validate_docs.py`
- **Evidence curation:**
  - Attach harness dry-run log and sandbox screenshots in kickoff packet appendix.
  - Store telemetry exports under `telemetry/geometry-baseline/` for reviewers.
- **Regression guard:**
  - Re-run harness dry-run when schema validators (`DC-040`) change.

---

## Steps

1. [ ] Publish kickoff brief cross-links across roadmap, sprint tracker, and backlog.
2. [ ] Record harness dry-run evidence and attach telemetry artefacts.
3. [ ] Finalize sandbox wiring notes and dataset availability confirmations.
4. [ ] Validate dataset manifests and benchmark snapshots for inclusion.
5. [ ] Present kickoff packet and capture executive sign-off.

---

## Evidence

### Test Results

```bash
# Capture command outputs once validation runs
# python scripts/validate_docs.py
```

**Test Summary:**
- Documentation validation: pending (capture after updates)

### Additional Notes

- Native runtime shared library is not included in current workspace snapshots; once `libengine_runtime.so` ships, rerun harness with `--frames 120` to extend timing data ahead of the review.
- Weekly progress and demo artefacts should continue to sync alongside `PM-510` to ensure visibility.

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| docs | [ ] Pending | Docs/DevRel | Kickoff packet, roadmap updates |

### Updated Files

- `hybrid_workflow/ROADMAP.md`
- `docs/ROADMAP.md`
- `hybrid_workflow/backlog/SPRINT-11-alignment.md`
- Kickoff packet artefacts under `docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md`

---

## Completion Checklist (Definition of Done)

- [ ] Agenda, timeline, risks, and demo artefacts consolidated in kickoff packet.
- [ ] Harness + sandbox demo evidence attached with reproducible commands and telemetry hashes.
- [ ] Roadmap and backlog cross-links updated to reference this task.
- [ ] Risk mitigations confirmed with owners and due dates.
- [ ] Kickoff review sign-off recorded with follow-up tasks filed if needed.
- [ ] Documentation validation (`python scripts/validate_docs.py`) recorded in Evidence.
- [ ] Status updated to `review` → `done` after kickoff approval.

---

## Result

**PR:** (pending)

**SHA:** (pending)

**Completion Date:** (pending)

**Notes:**

- Maintain weekly sync alignment with `SPRINT-11` tracker and PM-510 demos.
- Escalate licensing or benchmark blockers immediately to avoid slipping the kickoff review.
- Spawn follow-up tasks for Sprint 12 planning once kickoff outcomes land.

**Follow-ups:**

- [ ] Prepare Sprint 12 backlog realignment → create task SPRINT-12.

---

## Role Coordination

| Role | Name/Agent | Responsibilities | Status |
|------|------------|------------------|--------|
| Agent Orchestrator | Agent Orchestrator (11) | Facilitate daily syncs, update brief, escalate blockers. | Active |
| Product Manager | Product Manager (10) | Own agenda, ensure backlog and roadmap alignment. | Active |
| Knowledge Librarian | Knowledge Librarian (12) | Aggregate context packs, archive meeting notes. | Active |
| Specialist Engineer(s) | Module Leads | Deliver stream milestones (runtime, tools, assets, rendering, performance). | Active |
| Docs/DevRel | Docs/DevRel (95) | Refresh kickoff packet docs, NAVIGATION pointers, slide materials. | Active |
| QA/Test Specialist | QA/Test Specialist (90) | Capture harness/sandbox smoke outputs and attach logs. | Active |
| Performance Engineer | Performance Engineer (80) | Provide comparative benchmark baselines. | Active |
| Safety Reviewer | Safety Reviewer (15) | Validate dataset licensing and schema security notes. | Active |
| Reviewer | Reviewer (99) | Audit final packet for completeness. | Active |
| Release Manager | Release Manager (98) | Coordinate post-review release cadence and comms. | Active |

**Escalation Path:** Agent Orchestrator → Product Manager → Executive Review (if milestones slip)

**Additional Artifacts Created:**

- Kickoff packet appendix snapshots under `docs/backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md`
- Slide outline asset referencing this backlog file

---

