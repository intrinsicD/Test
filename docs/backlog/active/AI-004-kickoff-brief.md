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
- [ ] Circulate slide outline covering agenda, timeline, and risk mitigations by
      **2026-02-09**.
- [ ] Confirm demo scene and dataset availability across runtime, tools, and
      rendering teams.
- [ ] Capture benchmark baseline from harness + sandbox combo for inclusion in
      kickoff packet.

## Links
- Roadmap Phase 1 timeline: [`docs/ROADMAP.md#phase-1-milestone-timeline-kickoff-review-2026-02-20`](../../ROADMAP.md#phase-1-milestone-timeline-kickoff-review-2026-02-20)
- Sprint tracker alignment: [`2026-02-03-sprint-11.md`](2026-02-03-sprint-11.md)
- AI-004 initiative card: [`AI-004-application-prototyping-enablement.md`](../../archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md)
