# AI-004 Kickoff Brief (2026-02-20 Review)

AI-004 enters the kickoff review on **2026-02-20** to prove the prototyping
workflow is ready for research teams. This brief synchronises the roadmap,
backlog, and sprint tracker so every module lead aligns on deliverables,
dependencies, and risk mitigations.

## Agenda
- Schema contract recap (`DC-040`) and validator demo.
- Phase 1 milestone review: harness (`RT-320`), sandbox wiring (`TL-210`),
  dataset packaging (`AS-330`).
- Risk mitigation status and owner sign-off.
- Demo readiness checklist and next sync cadence.

## Success Metrics
- Kickoff packet accepted by runtime, tools, assets, and performance leads.
- Harness + sandbox operate on at least one packaged dataset with telemetry
  captured for comparative benchmarks.
- Risk mitigations closed or re-scoped with documented follow-ups by
  **2026-02-13**.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Facilitate daily syncs, update task brief, escalate blockers across streams. | Agent Orchestrator (11) |
| Product Manager | Own agenda/success metrics, ensure backlog items map to kickoff outcomes. | Product Manager (10) |
| Knowledge Librarian | Aggregate context packs, archive meeting notes, maintain cross-links to roadmap/backlog. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Runtime, tools, assets, rendering, and performance leads deliver stream milestones. | Module Leads |
| Docs/DevRel | Refresh kickoff packet docs, NAVIGATION pointers, and share slide materials. | Docs/DevRel (95) |
| QA/Test Specialist | Capture harness/sandbox smoke outputs and attach logs to quality reports. | QA/Test Specialist (90) |
| Performance Engineer | Provide comparative benchmark baselines for demo readiness. | Performance Engineer (80) |
| Safety Reviewer | Validate dataset licensing and schema security notes before the review. | Safety Reviewer (15) |
| Reviewer | Audit final packet for completeness ahead of executive review. | Reviewer (99) |
| Release Manager | Coordinate post-review release cadence and communications. | Release Manager (98) |

## Milestone Timeline

| Sequence | Target Date | Backlog | Owner | Dependencies | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | 2026-02-07 | [`DC-041`](DC-041-ai-004-kickoff-readiness.md) | Product Manager | [`DC-040`](DC-040-ai-004-configuration-schema.md) | Kickoff brief, roadmap timeline, and sprint tracker cross-links published. |
| 2 | 2026-02-12 | [`RT-320`](RT-320-runtime-prototyping-harness.md) | Runtime Lead | `DC-041` | Harness dry-run executes schema sample + telemetry export; smoke test recorded. |
| 3 | 2026-02-14 | [`TL-210`](TL-210-experiment-sandbox-ui.md) | Tools Lead | `RT-320` | Sandbox consumes harness presets and exposes dataset selection for review demo. |
| 4 | 2026-02-16 | [`AS-330`](AS-330-reference-dataset-packages.md) | Assets Lead | `RT-320`, `TL-210` | Package manifests validated against harness + sandbox integration test. |

## Risk Ownership

| Risk | Owner | Mitigation Due | Notes |
| --- | --- | --- | --- |
| Dataset licensing slips and blocks demo dataset selection. | Assets Lead | 2026-02-13 | Secure fallback synthetic dataset and document provenance in `AS-330`. |
| Benchmark automation exceeds CI time budget for kickoff smoke. | Performance Lead | 2026-02-13 | Produce ≤5 min smoke preset leveraging `CC-310` orchestrator options. |
| Harness coverage gaps hide schema regressions. | Runtime Lead | 2026-02-13 | Extend RT-320 smoke to assert schema version + telemetry channels before review. |

## Prep Checklist
- [x] Circulate slide outline covering agenda, timeline, and risk mitigations by
      **2026-02-09** (see [Slide Outline](#slide-outline-published-2026-02-09)).
- [x] Confirm demo scene and dataset availability across runtime, tools, and
      rendering teams (see [Demo Scene & Dataset Availability](#demo-scene--dataset-availability)).
- [x] Capture benchmark baseline from harness + sandbox combo for inclusion in
      kickoff packet (see [Benchmark Baseline Snapshot](#benchmark-baseline-snapshot)).

### Slide Outline (Published 2026-02-09)
1. **Kickoff Objectives** – revisit AI-004 readiness scorecard, milestone
   cadence, and success metrics.
2. **Schema Contract Recap** – highlight validator coverage, enforcement flags,
   and Python/C++ integration points.
3. **Harness Walkthrough** – dry-run demo of
   `python -m scripts.prototyping.run_prototype_harness --case-study geometry-baseline`
   showing configuration resolution, dataset validation, and telemetry targets.
4. **Sandbox Wiring** – ImGui layout for preset selection, telemetry overlays,
   and benchmark trigger entry points (paired with TL-210 follow-ups).
5. **Dataset Packaging** – manifest provenance summary, checksum table, and
   ingestion automation checkpoints for AS-330.
6. **Benchmark Baseline** – geometry remesh case-study snapshot (see below) plus
   comparative benchmark rollout plan.
7. **Risk Register & Mitigations** – review current owners, mitigation deadlines,
   and escalation paths ahead of 2026-02-13 gate.
8. **Next Steps** – sprint 12 focus, CI enablement, and review cadence through
   the 2026-02-20 kickoff.

The slide outline lives alongside the kickoff deck storyboard so module leads
can attach updated screenshots and telemetry exports without reworking the
agenda.

### Demo Scene & Dataset Availability
- **Harness validation** – The dry-run harness execution confirmed that the
  `geometry-baseline` case study resolves the `geometry-remesh-baseline`
  dataset, validates both mesh artefacts, and reports zero asset failures.
  Output from the `--describe-json` capture records:
  - `assets/datasets/remesh_sample/source_mesh.obj` (`cd19dd57…6a6225d0`, 196 B)
  - `assets/datasets/remesh_sample/output_mesh.obj` (`651a849d…34c44e65`, 277 B)
  Both hashes match the manifest expectations and are now linked in the kickoff
  packet dataset appendix.
- **Rendering preset** – The resolved rendering schema locks the
  `research-baseline` preset at 1280×720 deferred shading with overlays
  disabled, matching the sandbox defaults documented in TL-210.
- **Runtime configuration** – Orbit camera defaults (position `⟨0,0,4⟩`, target
  `⟨0,0,0⟩`) and simulation cadence (`Δt = 1/60 s`, two max substeps) mirror the
  sandbox preview so switching between interactive and headless runs no longer
  reinitialises scene state.

### Benchmark Baseline Snapshot
Harness dry-run summaries and sandbox metadata now provide the initial geometry
remeshing baseline shipped with the kickoff packet:

| Metric | Value | Source |
| --- | --- | --- |
| Scenario | `geometry-baseline` | Harness dry-run summary |
| Dataset | `geometry-remesh-baseline` | Harness dry-run summary |
| Telemetry target | `telemetry/geometry-baseline/{scenario}.json` | Harness dry-run summary |
| Input mesh | 4 vertices, 2 faces, edge length range 1.0–1.4142 | Dataset manifest |
| Output mesh | 5 vertices, 4 faces, edge length range 0.5–1.118 | Dataset manifest |
| Surface area | Input 1.0, output 1.0 | Dataset manifest |
| Remeshing mode | Uniform, target edge length 0.5 | Dataset manifest |
| UV reuse | Single chart, stretch ≤1.05, fill ratio 0.95 | Dataset manifest |
| Remesh operations | 2 splits, 1 collapse | Dataset manifest |
| Remesh duration | 3.2 ms | Dataset manifest |
| Triangle quality | 4 triangles, quality 0.64/0.82/0.99 | Dataset manifest |

The native runtime shared library is not packaged in this workspace snapshot, so
`average_tick_ms` and dispatch timings remain `N/A`. Once the CI build publishes
`libengine_runtime.so`, rerun the case study with `--frames 120` to extend the
table with timing data before the 2026-02-20 review.

## Links
- Roadmap Phase 1 timeline: [`docs/ROADMAP.md#phase-1-milestone-timeline-kickoff-review-2026-02-20`](../../ROADMAP.md#phase-1-milestone-timeline-kickoff-review-2026-02-20)
- Sprint tracker alignment: [`2026-02-03-sprint-11.md`](2026-02-03-sprint-11.md)
- AI-004 initiative card: [`AI-004-application-prototyping-enablement.md`](../../archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md)
