# Central Roadmap

This roadmap defines how the engine reaches **application readiness**—a state where research teams can prototype, benchmark, and ship interactive experiences with confidence. It complements module READMEs and the backlog entries under [`docs/backlog/`](backlog/). Update all three whenever priorities shift.

## Vision

> Deliver a production-capable runtime and tooling stack that loads curated datasets, renders with high fidelity, and measures performance against reference implementations.

Key outcomes:
This plan tracks the `AI-004` Application Prototyping Enablement initiative and breaks it into phased milestones.
- Researchers launch experiments from a shared configuration schema without ad-hoc wiring.
- Interactive and headless workflows share the same datasets, presets, and telemetry.
- Comparative benchmarks run automatically in CI and surface regressions within two percent.

## Priority Bands

Priorities use a numeric scale (**1 = highest urgency**, **5 = lowest**). Each milestone below cites backlog items and their current priority so agents can jump directly to the owning file.

## Path to Application Readiness

### Phase 0 — Foundations *(Complete)*
- ✅ [`RE-610`](backlog/archive/RE-610-research-rendering-baseline.md) *(Priority 2)* — Research rendering baseline with telemetry overlays.
- ✅ AI-001 to AI-003 infrastructure (handles, streaming, frame-graph metadata).

### Phase 1 — Kickoff Ready *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`DC-040`](backlog/active/DC-040-ai-004-configuration-schema.md) | Ratify shared configuration schema and validators. | Agent Orchestrator + Module Leads | Complete |
| 1 | [`DC-041`](backlog/active/DC-041-ai-004-kickoff-readiness.md) | Publish kickoff plan, risk ownership, and milestone sequencing. | Product Manager | Complete |
| 2 | [`RT-320`](backlog/active/RT-320-runtime-prototyping-harness.md) | Ship schema-driven harness with interactive + headless flows. | Runtime Lead | Complete |

**Exit Criteria:** Schema approved, kickoff brief published, harness prototype booting sample datasets.

<a id="ai004-phase1-timeline"></a>
#### Phase 1 Milestone Timeline (Kickoff Review 2026-02-20)

| Sequence | Target Date | Backlog | Owner | Dependencies | Notes | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 2026-02-07 | [`DC-041`](backlog/active/DC-041-ai-004-kickoff-readiness.md) | Product Manager | [`DC-040`](backlog/active/DC-040-ai-004-configuration-schema.md) | Kickoff packet: roadmap timeline, sprint tracker, and risk register cross-linked. | ✅ Complete |
| 2 | 2026-02-12 | [`RT-320`](backlog/active/RT-320-runtime-prototyping-harness.md) | Runtime Lead | `DC-041` | Harness smoke demo exercising schema sample + telemetry export recorded; kickoff brief now carries dataset hash appendix. | ✅ Complete |
| 3 | 2026-02-14 | [`TL-210`](backlog/active/TL-210-experiment-sandbox-ui.md) | Tools Lead | `RT-320` | Sandbox integrates harness presets and dataset selectors ready for review walk-through; overlay telemetry tuning continues. | ✅ Complete |
| 4 | 2026-02-16 | [`AS-330`](backlog/active/AS-330-reference-dataset-packages.md) | Assets Lead | `RT-320`, `TL-210` | Dataset package validated against harness + sandbox handoff for demo content. | ✅ Complete |

> Kickoff coordination artefacts: [`AI-004-kickoff-brief.md`](backlog/active/AI-004-kickoff-brief.md), [`2026-02-03-sprint-11.md`](backlog/active/2026-02-03-sprint-11.md).

### Phase 2 — Harness & Dataset Integration *(Priorities 2–3)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 3 | [`AS-330`](backlog/active/AS-330-reference-dataset-packages.md) | Curate licensed datasets with manifests and ingestion tooling. | Assets Lead | Complete |
| 3 | [`TL-210`](backlog/active/TL-210-experiment-sandbox-ui.md) | Finalise sandbox UI wiring to harness + benchmarks. | Tools Lead | Complete |
| 3 | [`RT-321`](backlog/active/RT-321-prototyping-case-studies.md) | Validate two case studies end to end with telemetry baselines. | Runtime Lead | Complete |

**Exit Criteria:** Harness, sandbox, and datasets operate together; case studies produce reproducible artefacts.

### Phase 3 — Benchmark Confidence *(Priorities 4–5)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 4 | [`CC-310`](backlog/active/CC-310-comparative-benchmark-automation.md) | Automate comparative benchmarks and CI smoke suite. | Performance Lead | Complete |
| 4 | [`CC-311`](backlog/active/CC-311-benchmark-visualisation.md) | Render comparative reports and expose them in tooling + CI. | Performance Lead | Complete |
| 5 | Follow-up polish items (to be spawned once CC-311 closes). | — | — |

**Exit Criteria:** Automated benchmarks and visual reports block regressions; kickoff demo is reproducible on CI hardware.

### Phase 4 — GPU Execution & Tooling Readiness *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`T-0120`](backlog/active/T-0120-gpu-resource-provider.md) | Implement GPU resource provider to unlock backend allocations and shader pipelines. | Rendering Lead | Planned |
| 1 | [`T-0119`](backlog/active/T-0119-command-encoder-integration.md) | Translate frame-graph work into backend command buffers and submissions. | Rendering Lead | Planned |
| 1 | [`RT-410`](backlog/active/RT-410-runtime-stage-planner.md) | Deliver stage planner and presentation loop from ADR-0008. | Runtime Lead | Planned |
| 2 | [`TL-310`](backlog/active/TL-310-editor-foundations.md) | Re-enable editor builds and integrate tooling registry with runtime. | Tools Lead | Planned |

**Exit Criteria:** OpenGL/Vulkan execute real workloads with shader pipelines, runtime presentation loop synchronises with tooling, and the editor/tooling stack is buildable with baseline smoke coverage.

## Active Backlog Snapshot

| Backlog | Priority | Status | Notes |
| --- | --- | --- | --- |
| [`T-0120`](backlog/active/T-0120-gpu-resource-provider.md) | 1 | Planned | GPU allocations, textures, and shader pipelines remain unimplemented; reinstated to unblock rendering backends. |
| [`T-0119`](backlog/active/T-0119-command-encoder-integration.md) | 1 | Planned | Command encoder missing; frame-graph execution still stubbed. |
| [`RT-410`](backlog/active/RT-410-runtime-stage-planner.md) | 1 | Planned | Runtime main loop lacks stage planner and presentation backends. |
| [`DC-040`](backlog/active/DC-040-ai-004-configuration-schema.md) | 1 | Complete | Schema ADR, cross-language validators, and prototyping playbook published. |
| [`DC-041`](backlog/active/DC-041-ai-004-kickoff-readiness.md) | 1 | Complete | Kickoff brief + roadmap timeline published. |
| [`RT-320`](backlog/active/RT-320-runtime-prototyping-harness.md) | 2 | Complete | Harness + telemetry shipped; maintenance tasks captured in module TODOs. |
| [`TL-310`](backlog/active/TL-310-editor-foundations.md) | 2 | Planned | Editor builds disabled; registry and harness integration outstanding. |
| [`AS-330`](backlog/active/AS-330-reference-dataset-packages.md) | 3 | Complete | Dataset packages published with ingestion automation and provenance docs. |
| [`TL-210`](backlog/active/TL-210-experiment-sandbox-ui.md) | 3 | Complete | Sandbox wiring finished; follow-up automation tracked in tools TODOs. |
| [`RT-321`](backlog/active/RT-321-prototyping-case-studies.md) | 3 | Complete | Geometry + rendering case studies documented with CTest coverage. |
| [`CC-310`](backlog/active/CC-310-comparative-benchmark-automation.md) | 4 | Complete | Orchestrator, SVG plots, and smoke helper published for AI-004 fixtures. |
| [`CC-311`](backlog/active/CC-311-benchmark-visualisation.md) | 4 | Complete | Telemetry viewer comparative mode + AI-004 reports published. |

## Risks & Mitigations

| Priority | Risk | Owner | Mitigation | Mitigation Due |
| --- | --- | --- | --- | --- |
| 1 | GPU resource provider/command encoder slip keeps backends non-functional. | Rendering Lead | Pair T-0120/T-0119 with shared milestones; demo backend smoke tests weekly. | 2026-03-01 |
| 1 | Stage planner delivery lags GPU work, blocking presentation. | Runtime Lead | Prototype planner API in parallel; stage joint integration review before backend merge. | 2026-03-05 |
| 2 | Editor/tooling reinstatement blocked by runtime hooks. | Tools Lead | Align TL-310 milestones with RT-410 deliverables; introduce feature flags for incremental validation. | 2026-03-08 |
| 3 | Legacy documentation remains out of sync with reopened tasks. | Knowledge Librarian | Audit module READMEs when each backlog item starts; run docs validator. | 2026-02-25 |

## Maintenance Checklist

- Review this roadmap weekly; update status/progress before each sync.
- When a backlog item completes, move its file to [`backlog/archive/`](backlog/archive/) and update tables above.
- Keep module READMEs and the root [`README.md`](../README.md) aligned with the priority bands listed here.
- Run `python scripts/validate_docs.py` after editing roadmap or backlog files to catch broken links.

**Last updated:** 2026-02-17.
