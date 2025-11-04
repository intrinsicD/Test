# AI-004 Kickoff Brief (2025-12-05 Review)

## Purpose
Coordinate the Phase 1 deliverables for the AI-004 prototyping initiative ahead of the
2025-12-05 kickoff review. This brief captures the prioritized tasks, owners, and
inter-task dependencies so engineering leads can sequence work and mitigate
risks without ad-hoc status syncs.

## Milestone Timeline (Phase 1)

| Sequence | Task ID | Deliverable | Owner | Target Date | Dependencies |
| --- | --- | --- | --- | --- | --- |
| 1 | `DC-041` | Kickoff readiness packet published (roadmap updates, risk register, cross-linking) | @pm-agent | 2025-12-05 | Schema alignment (`DC-040`), latest module status notes |
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
| Dataset licensing approvals delay case study manifests | @assets-lead | Finalise permissive asset shortlist; confirm legal review slots | 2025-11-27 |
| Comparative plotting stack exceeds CI time budget | @perf-lead | Start with reduced case study set, cache artefacts between stages | 2025-11-27 |
| Sandbox automation flakes on headless runners | @tools-lead | Add deterministic seed + retry harness; run nightly smoke before promo | 2025-11-27 |

## Agenda & Success Metrics

1. **Schema + risk review (Owner: @pm-agent)** — confirm mitigation deck and roadmap updates complete; success = consensus on schema adoption blockers cleared.
2. **Case study readiness (Owner: @runtime-lead)** — demonstrate harness + sandbox executing geometry/rendering scenarios; success = telemetry baselines captured for both studies.
3. **Benchmark visualisation plan (Owner: @perf-lead)** — walk through comparative report flow and CI budget; success = agreed gating criteria and hardware allocation approval path.
4. **Dataset licensing checkpoint (Owner: @assets-lead)** — validate approval status and fallback assets; success = signed-off asset list with provenance attachments.
5. **Sandbox automation stability (Owner: @tools-lead)** — review headless automation checklist; success = nightly smoke coverage committed with retry strategy.

## Coordination Checklist

- [ ] Confirm schema validators (`DC-040`) enabled with `--require-schema` in harness CI runs.
- [ ] Publish kickoff slide deck outline referencing this brief, roadmap milestone table, and AI-004 task card.
- [ ] Schedule weekly syncs between runtime/tools/performance leads until kickoff review completes.
- [ ] Ensure telemetry viewer comparative outputs stored under `assets/benchmarks/ai-004/` for reuse in CI & demo.

## References
- [`AI_004_APPLICATION_PROTOTYPING_ENABLEMENT.md`](AI_004_APPLICATION_PROTOTYPING_ENABLEMENT.md)
- [`../../../../ROADMAP.md)
- [`RT_321_PROTOTYPING_CASE_STUDY_VALIDATION.md`](RT_321_PROTOTYPING_CASE_STUDY_VALIDATION.md)
- [`CC_311_BENCHMARK_VISUALIZATION_INTEGRATION.md`](CC_311_BENCHMARK_VISUALIZATION_INTEGRATION.md)
- [`TL_210_EXPERIMENT_SANDBOX_UI.md`](TL_210_EXPERIMENT_SANDBOX_UI.md)
- [`AS_330_REFERENCE_DATASET_PACKAGES.md`](AS_330_REFERENCE_DATASET_PACKAGES.md)
