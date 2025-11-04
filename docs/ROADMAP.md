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
- ✅ [`RE-610`](backlog/archive/RE_610_RESEARCH_RENDERING_BASELINE.md) *(Priority 2)* — Research rendering baseline with telemetry overlays.
- ✅ AI-001 to AI-003 infrastructure (handles, streaming, frame-graph metadata).

### Phase 1 — Kickoff Ready *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md) | Ratify shared configuration schema and validators. | Agent Orchestrator + Module Leads | Complete |
| 1 | [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md) | Publish kickoff plan, risk ownership, and milestone sequencing. | Product Manager | Complete |
| 2 | [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md) | Ship schema-driven harness with interactive + headless flows. | Runtime Lead | Complete |

**Exit Criteria:** Schema approved, kickoff brief published, harness prototype booting sample datasets.

<a id="ai004-phase1-timeline"></a>
#### Phase 1 Milestone Timeline (Kickoff Review 2026-02-20)

| Sequence | Target Date | Backlog | Owner | Dependencies | Notes | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 2026-02-07 | [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md) | Product Manager | [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md) | Kickoff packet: roadmap timeline, sprint tracker, and risk register cross-linked. | ✅ Complete |
| 2 | 2026-02-12 | [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md) | Runtime Lead | `DC-041` | Harness smoke demo exercising schema sample + telemetry export recorded; kickoff brief now carries dataset hash appendix. | ✅ Complete |
| 3 | 2026-02-14 | [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md) | Tools Lead | `RT-320` | Sandbox integrates harness presets and dataset selectors ready for review walk-through; overlay telemetry tuning continues. | ✅ Complete |
| 4 | 2026-02-16 | [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md) | Assets Lead | `RT-320`, `TL-210` | Dataset package validated against harness + sandbox handoff for demo content. | ✅ Complete |

> Kickoff coordination artefacts: [`AI_004_KICKOFF_BRIEF.md`](backlog/active/AI_004_KICKOFF_BRIEF.md), [`2026-02-03-SPRINT_11.md`](backlog/active/2026-02-03-SPRINT_11.md).

### Phase 2 — Harness & Dataset Integration *(Priorities 2–3)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 3 | [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md) | Curate licensed datasets with manifests and ingestion tooling. | Assets Lead | Complete |
| 3 | [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md) | Finalise sandbox UI wiring to harness + benchmarks. | Tools Lead | Complete |
| 3 | [`RT-321`](backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md) | Validate two case studies end to end with telemetry baselines. | Runtime Lead | Complete |

**Exit Criteria:** Harness, sandbox, and datasets operate together; case studies produce reproducible artefacts.

### Phase 3 — Benchmark Confidence *(Priorities 4–5)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 4 | [`CC-310`](backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md) | Automate comparative benchmarks and CI smoke suite. | Performance Lead | Complete |
| 4 | [`CC-311`](backlog/archive/CC_311_BENCHMARK_VISUALISATION.md) | Render comparative reports and expose them in tooling + CI. | Performance Lead | Complete |
| 5 | Follow-up polish items (to be spawned once CC-311 closes). | — | — |

**Exit Criteria:** Automated benchmarks and visual reports block regressions; kickoff demo is reproducible on CI hardware.

### Phase 4 — GPU Execution & Tooling Readiness *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`T-0120`](backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md) | Implement GPU resource provider to unlock backend allocations and shader pipelines. | Rendering Lead | In Progress |
| 1 | [`T-0119`](backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md) | Translate frame-graph work into backend command buffers and submissions. | Rendering Lead | In Progress |
| 1 | [`RT-410`](backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | Deliver stage planner and presentation loop from ADR-0008. | Runtime Lead | In Progress |
| 2 | [`TL-310`](backlog/active/TL_310_EDITOR_FOUNDATIONS.md) | Re-enable editor builds and integrate tooling registry with runtime. | Tools Lead | Sequenced (awaits RT-410 hooks) |
| 2 | [`PM-510`](backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md) | Maintain weekly cross-module GPU/runtime/tooling integration demos and update documentation/risks. | Agent Orchestrator | Active |

**Exit Criteria:** OpenGL/Vulkan execute real workloads with shader pipelines, runtime presentation loop synchronises with tooling, and the editor/tooling stack is buildable with baseline smoke coverage.

#### Phase 4 Milestone Timeline (GPU Enablement Review 2026-03-22)

| Sequence | Target Date | Backlog | Owner | Dependencies | Notes | Status |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 2026-02-28 | [`T-0120`](backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md) + [`T-0119`](backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md) | Rendering Lead + Runtime Lead | [`ADR-0003`](specs/ADR_0003_RUNTIME_FRAME_GRAPH.md) | Joint design review to ratify resource ownership, encoder submission flow, and telemetry checkpoints. | 🚧 In Progress |
| 2 | 2026-03-05 | [`RT-410`](backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | Runtime Lead | Sequence 1 | Stage planner adapters wired into runtime host; presentation mocks exercised in harness regression suite. | 🟡 Scheduled |
| 3 | 2026-03-08 | [`PM-510`](backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md) | Agent Orchestrator | Sequence 1 | First weekly integration demo covering GPU submission → runtime presentation with telemetry capture. | 🟢 Active |
| 4 | 2026-03-12 | [`TL-310`](backlog/active/TL_310_EDITOR_FOUNDATIONS.md) | Tools Lead | Sequence 2 | Editor module re-enabled behind feature flag; panel registry consumes shared presentation adapters for demo readiness. | 🟡 Scheduled |

## Active Backlog Snapshot

| Backlog | Priority | Status | Notes |
| --- | --- | --- | --- |
| [`T-0120`](backlog/active/T_0120_GPU_RESOURCE_PROVIDER.md) | 1 | In Progress | GPU allocations, textures, and shader pipelines being implemented with shared reviews alongside T-0119. |
| [`T-0119`](backlog/active/T_0119_COMMAND_ENCODER_INTEGRATION.md) | 1 | In Progress | Command encoder integration underway; smoke demos paired with T-0120 milestone. |
| [`RT-410`](backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | 1 | In Progress | Stage planner adapters under active development to unblock presentation loop integration. |
| [`TL-310`](backlog/active/TL_310_EDITOR_FOUNDATIONS.md) | 2 | Sequenced | Editor builds remain disabled; work scheduled to start once RT-410 adapters merge. |
| [`PM-510`](backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md) | 2 | Active | Weekly integration demos and documentation syncs keep GPU/runtime/tooling deliverables aligned. |
| [`PM-520`](backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) | 2 | Planned | Executes backlog/roadmap archival cleanup identified by the March 2026 audit. |
> Archived backlog entries: [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md), [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md), [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md), [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md), [`RT-321`](backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md), [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md), [`CC-310`](backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md), [`CC-311`](backlog/archive/CC_311_BENCHMARK_VISUALISATION.md), and [`PL-240`](backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md) now reside under `docs/backlog/archive/` following PM-520 remediation.
> Archived backlog entries: [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md), [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md), [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md), [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md), [`RT-321`](backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md), [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md), [`CC-310`](backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md), [`CC-311`](backlog/archive/CC_311_BENCHMARK_VISUALISATION.md), [`PL-240`](backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md), and [`PM-520`](backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) now reside under `docs/backlog/archive/` following PM-520 remediation (completed 2026-11-04).
## Risks & Mitigations

| Priority | Risk | Owner | Mitigation | Mitigation Due |
| --- | --- | --- | --- | --- |
| 1 | GPU resource provider/command encoder slip keeps backends non-functional. | Rendering Lead | Run the joint T-0120/T-0119 milestone with shared design reviews and publish outputs through [`PM-510`](backlog/active/PM_510_WEEKLY_INTEGRATION_DEMOS.md). | 2026-03-01 |
| 1 | Stage planner delivery lags GPU work, blocking presentation. | Runtime Lead | Start RT-410 alongside the GPU milestone; present weekly planner/presentation progress in PM-510 demos. | 2026-03-05 |
| 2 | Editor/tooling reinstatement blocked by runtime hooks. | Tools Lead | Sequence TL-310 immediately after RT-410 adapter merge and preview editor state during PM-510 demos. | 2026-03-08 |
| 3 | Legacy documentation remains out of sync with reopened tasks. | Knowledge Librarian | Capture updates from weekly demos and rerun docs validator after each milestone increment. | 2026-02-25 |

## Maintenance Checklist

- Review this roadmap weekly; update status/progress before each sync.
- When a backlog item completes, move its file to [`backlog/archive/`](backlog/archive/) and update tables above.
- Keep module READMEs and the root [`README.md`](../README.md) aligned with the priority bands listed here.
- Run `python scripts/validate_docs.py` after editing roadmap or backlog files to catch broken links.

**Last updated:** 2026-02-24.
