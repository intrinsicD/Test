# Central Roadmap

This roadmap defines how the engine reaches **application readiness**—a state where research teams can prototype, benchmark, and ship interactive experiences with confidence. It complements module READMEs and active task tracking in [`../hybrid_workflow/backlog/`](../hybrid_workflow/backlog/). Update all three whenever priorities shift.

> **Note:** For workflow-specific bundle tracking, see [`../hybrid_workflow/ROADMAP.md`](../hybrid_workflow/ROADMAP.md).

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

**Completion:** All Phase 1 milestones delivered. Coordination artefacts: [`AI-004-kickoff-brief.md`](../hybrid_workflow/backlog/AI-004-kickoff-brief.md), [`SPRINT-11-alignment.md`](../hybrid_workflow/backlog/archive/SPRINT-11-alignment.md).

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

### Bundle C — Documentation & Infrastructure *(Priority 3 — Active)*
- ✅ [`DC-050`](../hybrid_workflow/backlog/archive/DC-050-workflow-migration.md) — Workflow migration to hybrid model.
- ✅ [`TL-320`](../hybrid_workflow/backlog/archive/TL-320-task-dashboard.md) — Task status dashboard automation.
- ✅ [`TL-330`](../hybrid_workflow/backlog/archive/TL-330-task-status-blocked-filter.md) — Task status CLI blocked filter.
- ✅ [`TL-331`](../hybrid_workflow/backlog/archive/TL-331-hybrid-status-json.md) — Hybrid status reporter JSON export.
- ✅ [`TL-332`](../hybrid_workflow/backlog/archive/TL-332-task-status-multiline-metadata.md) — Task status CLI multiline metadata parsing.
- ✅ [`TL-341`](../hybrid_workflow/backlog/archive/TL-341-next-action-summary.md) — Next-action summary for hybrid status reporter.
- ✅ [`TL-342`](../hybrid_workflow/backlog/archive/TL-342-hybrid-status-owner-filter.md) — Hybrid status reporter owner filter.
- ✅ [`TL-343`](../hybrid_workflow/backlog/archive/TL-343-task-status-owner-filter.md) — Task status CLI owner filter.
- ✅ [`TL-344`](../hybrid_workflow/backlog/archive/TL-344-next-actions-guidance.md) — Next-actions guidance for empty ready queue.
- ✅ [`TL-345`](../hybrid_workflow/backlog/archive/TL-345-hybrid-status-relates-to-filter.md) — Hybrid status reporter relates_to filter.
- ✅ [`TL-346`](../hybrid_workflow/backlog/archive/TL-346-next-actions-filter-support.md) — Next-actions filter support.

### Phase 4 — GPU Execution & Tooling Readiness *(Priorities 1–2)*
| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 1 | [`T-0120`](../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) | Implement GPU resource provider to unlock backend allocations and shader pipelines. | Rendering Lead | Done |
| 1 | [`T-0119`](../hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md) | Translate frame-graph work into backend command buffers and submissions. | Rendering Lead | Done |
| 1 | [`RT-410`](../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) | Deliver stage planner and presentation loop from ADR-0008. | Runtime Lead | Done |
| 1 | [`RG-450`](../hybrid_workflow/backlog/archive/RG-450-modular-render-pipeline.md) | Build modular render pipeline planner with node reflection, transient resources, and async scheduling. | Rendering Lead | Done |
| 2 | [`TL-310`](../hybrid_workflow/backlog/archive/TL-310-editor-foundations.md) | Re-enable editor builds and integrate tooling registry with runtime. | Tools Lead | Done (editor harness + registry baseline landed; follow-up panels tracked separately) |
| 2 | [`PM-510`](../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md) | Maintain weekly cross-module GPU/runtime/tooling integration demos and update documentation/risks. | Agent Orchestrator | Active |
| 2 | [`TL-311`](../hybrid_workflow/backlog/archive/TL-311-scene-hierarchy-panel.md) | Ship scene hierarchy diagnostics panel to surface entity graph inside editor. | Tools Lead | Done |
| 2 | [`TL-312`](../hybrid_workflow/backlog/TL-312-performance-metrics-panel.md) | Visualise profiler metrics and benchmark deltas in-editor for Bundle B demos. | Tools Lead | In Progress |
| 2 | [`TL-313`](../hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md) | Expose asset cache state and hot reload telemetry within the editor. | Tools Lead | Done |
| 2 | [`TL-314`](../hybrid_workflow/backlog/TL-314-telemetry-visualization-panel.md) | Provide live telemetry overlays and alerts to monitor runtime health. | Tools Lead | In Progress |

**Exit Criteria:** OpenGL/Vulkan execute real workloads with shader pipelines, runtime presentation loop synchronises with tooling, and the editor/tooling stack is buildable with baseline smoke coverage.

**Sequencing:**
1. **RT-410** (Priority 1) - ✅ Completed; presentation adapters archived for tooling reuse
2. **PM-510** (Priority 2) - Ongoing weekly integration demos covering GPU → runtime → tooling
3. **TL-310** (Priority 2) - ✅ Editor re-enablement complete; registry + harness baseline archived for panel follow-ups
4. **TL-311** (Priority 2) - ✅ Scene hierarchy panel landed; validation overlays now available for PM-510 rehearsals

### Bundle D — Kickoff Coordination *(Priorities 0)*

**Goal:** Process coordination and documentation alignment for AI-004 initiative.

_Note: Bundle D tasks have P0 priority for process/coordination but run in parallel with technical bundles and don't block GPU/runtime/tooling execution._

| Priority | Backlog | Intent | Owner | Status |
| --- | --- | --- | --- | --- |
| 0 | [`SPRINT-11`](../hybrid_workflow/backlog/archive/SPRINT-11-alignment.md) | Coordinate Sprint 11 execution to align backlog, roadmap, and kickoff artefacts. | Agent Orchestrator | Done |
| 0 | [`AI-004`](../hybrid_workflow/backlog/AI-004-kickoff-brief.md) | Produce kickoff brief with agenda, timeline, risks, and demo artefacts. | Agent Orchestrator | In Progress |

**Exit Criteria:** Kickoff packet consolidates deliverables with accountable owners; roadmap and sprint alignment documented.

## Active Backlog Snapshot

| Backlog | Priority | Status | Notes |
| --- | --- | --- | --- |
| [`AI-004`](../hybrid_workflow/backlog/AI-004-kickoff-brief.md) | 0 | In Progress | Kickoff brief coordination; SPRINT-11 complete, final artefacts in progress. |
| [`T-0120`](../hybrid_workflow/backlog/archive/T-0120-gpu-resource-provider.md) | 1 | Done | GPU providers for OpenGL/Vulkan ship with runtime presentation integration and telemetry. |
| [`T-0119`](../hybrid_workflow/backlog/archive/T-0119-command-encoder-integration.md) | 1 | Done | Command encoder integration is live; backends now consume encoded passes pending real resource allocation support. |
| [`RG-450`](../hybrid_workflow/backlog/archive/RG-450-modular-render-pipeline.md) | 1 | Done | Planner ships with hot-reload coverage and telemetry exports; monitoring perf delta in PM-510 demos. |
| [`TL-310`](../hybrid_workflow/backlog/archive/TL-310-editor-foundations.md) | 2 | Done | Editor harness + registry shipped; TL-311–TL-314 deliver panel follow-ups. |
| [`PM-510`](../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md) | 2 | Active | Weekly integration demos and documentation syncs keep GPU/runtime/tooling deliverables aligned; latest capture documents modular planner telemetry. |
| [`TL-312`](../hybrid_workflow/backlog/TL-312-performance-metrics-panel.md) | 2 | In Progress | Groomed scope; TL-310 registry hooks landed so implementation can begin. |
| [`TL-314`](../hybrid_workflow/backlog/TL-314-telemetry-visualization-panel.md) | 2 | In Progress | Telemetry overlays and alerting integrated with PM-510 evidence capture. |

> **Archived backlog entries:** [`SPRINT-11`](../hybrid_workflow/backlog/archive/SPRINT-11-alignment.md), [`DC-040`](backlog/archive/DC_040_AI_004_CONFIGURATION_SCHEMA.md), [`DC-041`](backlog/archive/DC_041_AI_004_KICKOFF_READINESS.md), [`RT-320`](backlog/archive/RT_320_RUNTIME_PROTOTYPING_HARNESS.md), [`TL-210`](backlog/archive/TL_210_EXPERIMENT_SANDBOX_UI.md), [`RT-321`](backlog/archive/RT_321_PROTOTYPING_CASE_STUDIES.md), [`AS-330`](backlog/archive/AS_330_REFERENCE_DATASET_PACKAGES.md), [`CC-310`](backlog/archive/CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md), [`CC-311`](backlog/archive/CC_311_BENCHMARK_VISUALISATION.md), [`PL-240`](backlog/archive/PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md), [`PM-520`](backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md), [`TL-311`](../hybrid_workflow/backlog/archive/TL-311-scene-hierarchy-panel.md), [`TL-313`](../hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md), [`TL-320`](../hybrid_workflow/backlog/archive/TL-320-task-dashboard.md), and [`RT-410`](../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md) are now in `docs/backlog/archive/` following completion of PM-520 backlog hygiene remediation.
## Risks & Mitigations

| Priority | Risk | Owner | Mitigation |
| --- | --- | --- | --- |
| 1 | Runtime stage planner delivery lags GPU work, blocking presentation. | Runtime Lead | Mitigated by RT-410 completion (2026-03-30); continue monitoring TL-310 enablement and presentation telemetry in PM-510 demos. |
| 2 | Editor/tooling reinstatement blocked by runtime hooks. | Tools Lead | Mitigated by TL-310 completion; track TL-312–TL-314 implementation readiness in PM-510 demos. |
| 3 | Legacy documentation remains out of sync with reopened tasks. | Knowledge Librarian | Capture updates from weekly demos and rerun docs validator after each milestone increment. |
| 3 | CI containers lack GLFW/Xrandr headers so geometry_viewer cannot render. | Tools Lead | Headless fallback now logs the dependency gap; coordinate with infra to install X11 dev packages before release validation. 【F:engine/tools/examples/geometry_viewer.cpp†L61-L109】【e03c46†L1-L14】 |

## Maintenance Checklist
- When a backlog item completes, move its file to [`../hybrid_workflow/backlog/archive/`](../hybrid_workflow/backlog/archive/) or [`backlog/archive/`](backlog/archive/) (for old format tasks) and update tables above.
- Review this roadmap weekly; update status/progress before each sync.
- When a backlog item completes, move its file to [`backlog/archive/`](backlog/archive/) and update tables above.
- Keep module READMEs and the root [`README.md`](../README.md) aligned with the priority bands listed here.
- Run `python scripts/validate_docs.py` after editing roadmap or backlog files to catch broken links.

