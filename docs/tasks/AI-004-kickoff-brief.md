# AI-004 Kickoff Brief (2025-12-05 Review)

## Purpose
Coordinate the Phase 1 deliverables for the AI-004 prototyping initiative ahead of the
2025-12-05 kickoff review. This brief captures the prioritized tasks, owners, and
inter-task dependencies so engineering leads can sequence work and mitigate
risks without ad-hoc status syncs.

## Milestone Timeline (Phase 1)

| Sequence | Task ID | Deliverable | Owner | Target Date | Dependencies |
| --- | --- | --- | --- | --- | --- |
| 1 | `DC-041` | Kickoff readiness packet published (roadmap updates, risk register, cross-linking) | @pm-agent | 2025-12-29 | Schema alignment (`DC-040`), latest module status notes |
| 2 | `RT-321` | Geometry + rendering case studies executable through harness & sandbox with telemetry baselines | @runtime-lead (with @assets-lead, @tools-lead) | 2026-01-16 | Dataset packaging (`AS-330`), harness schema enforcement (`DC-040`) |
| 3 | `CC-311` | Comparative benchmark visualisation & CI gating wired to AI-004 case studies | @perf-lead (with @tools-lead) | 2026-01-19 | Case study artefacts (`RT-321`), comparative orchestrator (`CC-310`) |

## Priority Stack

| Rank | Task | RICE Notes |
| --- | --- | --- |
| P0 | `DC-041` Kickoff readiness | **Reach**: entire AI-004 surface (5 modules); **Impact**: unlocks review gate; **Confidence**: 70% given schema already aligned; **Effort**: 4 person-days. RICE ≈ 87.5 |
| P0 | `RT-321` Case study validation | Reach: runtime + assets + rendering integrators; Impact: mandatory demo coverage; Confidence: 55% (datasets still finalising); Effort: 10 person-days. RICE ≈ 44.0 |
| P0 | `CC-311` Benchmark visualisation | Reach: performance + tooling teams; Impact: required CI/regression signal; Confidence: 60%; Effort: 7 person-days. RICE ≈ 51.4 |
| P1 | `TL-210` Sandbox UI integration demo | Reach: researchers; Impact: supports kickoff demo but can trail benchmark wiring by one sprint; Confidence: 65%; Effort: 8 person-days. RICE ≈ 39.1 |
| P1 | `AS-330` Dataset package expansion | Reach: runtime + research teams; Impact: enables additional case studies beyond the two mandatory ones; Confidence: 60%; Effort: 9 person-days. RICE ≈ 33.3 |

## Risk Register Updates

| Risk | Owner | Mitigation | Deadline |
| --- | --- | --- | --- |
| Dataset licensing approvals delay case study manifests | @assets-lead | Finalise permissive asset shortlist; confirm legal review slots | 2025-12-27 |
| Comparative plotting stack exceeds CI time budget | @perf-lead | Start with reduced case study set, cache artefacts between stages | 2026-01-09 |
| Sandbox automation flakes on headless runners | @tools-lead | Add deterministic seed + retry harness; run nightly smoke before promo | 2026-01-05 |

## Coordination Checklist

- [ ] Confirm schema validators (`DC-040`) enabled with `--require-schema` in harness CI runs.
- [ ] Publish kickoff slide deck outline referencing this brief, roadmap milestone table, and AI-004 task card.
- [ ] Schedule weekly syncs between runtime/tools/performance leads until kickoff review completes.
- [ ] Ensure telemetry viewer comparative outputs stored under `assets/benchmarks/ai-004/` for reuse in CI & demo.

## References
- [`AI-004-application-prototyping-enablement.md`](AI-004-application-prototyping-enablement.md)
- [`../ROADMAP.md#ai-004-—-application-prototyping-enablement-🚀-active`](../ROADMAP.md#ai-004-—-application-prototyping-enablement-🚀-active)
- [`RT-321-prototyping-case-study-validation.md`](RT-321-prototyping-case-study-validation.md)
- [`CC-311-benchmark-visualization-integration.md`](CC-311-benchmark-visualization-integration.md)
- [`TL-210-experiment-sandbox-ui.md`](TL-210-experiment-sandbox-ui.md)
- [`AS-330-reference-dataset-packages.md`](AS-330-reference-dataset-packages.md)
