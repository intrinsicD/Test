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

**Completion:** All Phase 1 milestones delivered. Coordination artefacts: [`AI-004-kickoff-brief.md`](../hybrid_workflow/backlog/AI-004-kickoff-brief.md), [`SPRINT-11-alignment.md`](../hybrid_workflow/backlog/SPRINT-11-alignment.md).

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
| 1 | [`T-0120`](../hybrid_workflow/backlog/T-0120-gpu-resource-provider.md) | Implement GPU resource provider to unlock backend allocations and shader pipelines. | Rendering Lead | In Progress |
| 1 | [`T-0119`](../hybrid_workflow/backlog/T-0119-command-encoder-integration.md) | Translate frame-graph work into backend command buffers and submissions. | Rendering Lead | In Progress |
| 1 | [`RT-410`](../hybrid_workflow/backlog/RT-410-runtime-stage-planner.md) | Deliver stage planner and presentation loop from ADR-0008. | Runtime Lead | In Progress |
| 2 | [`TL-310`](../hybrid_workflow/backlog/TL-310-editor-foundations.md) | Re-enable editor builds and integrate tooling registry with runtime. | Tools Lead | In Progress (planning; blocked on RT-410 hooks) |
| 2 | [`PM-510`](../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md) | Maintain weekly cross-module GPU/runtime/tooling integration demos and update documentation/risks. | Agent Orchestrator | Active |

**Exit Criteria:** OpenGL/Vulkan execute real workloads with shader pipelines, runtime presentation loop synchronises with tooling, and the editor/tooling stack is buildable with baseline smoke coverage.

**Sequencing:**
1. **T-0120 + T-0119** (Priority 1) - Joint GPU resource provider and command encoder work with shared design reviews
2. **RT-410** (Priority 1) - Stage planner adapters, depends on GPU milestone progress
3. **PM-510** (Priority 2) - Ongoing weekly integration demos covering GPU → runtime → tooling
4. **TL-310** (Priority 2) - Editor re-enablement; planning underway while RT-410 adapters land

## Active Backlog Snapshot

| Backlog | Priority | Status | Notes |
| --- | --- | --- | --- |
| [`T-0120`](../hybrid_workflow/backlog/T-0120-gpu-resource-provider.md) | 1 | In Progress | GPU allocations, textures, and shader pipelines being implemented with shared reviews alongside T-0119. |
| [`T-0119`](../hybrid_workflow/backlog/T-0119-command-encoder-integration.md) | 1 | In Progress | Command encoder integration underway; smoke demos paired with T-0120 milestone. |
| [`RT-410`](../hybrid_workflow/backlog/RT-410-runtime-stage-planner.md) | 1 | In Progress | Stage planner adapters under active development to unblock presentation loop integration. |
| [`TL-310`](../hybrid_workflow/backlog/TL-310-editor-foundations.md) | 2 | In Progress | Context assembly started; implementation will begin once RT-410 exposes presentation hooks. |
| [`PM-510`](../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md) | 2 | Active | Weekly integration demos and documentation syncs keep GPU/runtime/tooling deliverables aligned. |

> **Archived backlog entries:** [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md), [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md), [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md), [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md), [`RT-321`](backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md), [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md), [`CC-310`](backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md), [`CC-311`](backlog/archive/CC_311_BENCHMARK_VISUALISATION.md), [`PL-240`](backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md), and [`PM-520`](backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) are now in `docs/backlog/archive/` following completion of PM-520 backlog hygiene remediation.
## Risks & Mitigations

| Priority | Risk | Owner | Mitigation |
| --- | --- | --- | --- |
| 1 | GPU resource provider/command encoder slip keeps backends non-functional. | Rendering Lead | Run the joint T-0120/T-0119 milestone with shared design reviews and publish outputs through [`PM-510`](../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md). |
| 1 | Stage planner delivery lags GPU work, blocking presentation. | Runtime Lead | Start RT-410 alongside the GPU milestone; present weekly planner/presentation progress in PM-510 demos. |
| 2 | Editor/tooling reinstatement blocked by runtime hooks. | Tools Lead | Sequence TL-310 immediately after RT-410 adapter merge and preview editor state during PM-510 demos. |
| 3 | Legacy documentation remains out of sync with reopened tasks. | Knowledge Librarian | Capture updates from weekly demos and rerun docs validator after each milestone increment. |

## Maintenance Checklist

- Review this roadmap weekly; update status/progress before each sync.
- When a backlog item completes, move its file to [`backlog/archive/`](backlog/archive/) and update tables above.
- Keep module READMEs and the root [`README.md`](../README.md) aligned with the priority bands listed here.
- Run `python scripts/validate_docs.py` after editing roadmap or backlog files to catch broken links.

