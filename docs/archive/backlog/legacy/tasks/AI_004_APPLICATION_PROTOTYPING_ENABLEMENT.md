# Task Card: AI-004

## Title
Application Prototyping Enablement

## Type
- [x] Feature
- [ ] Bug Fix
- [x] Refactor
- [x] Documentation
- [x] Research
- [x] Performance Optimization

## Priority
- [x] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
8–10 weeks across rendering, runtime, tools, and assets teams

---

## Description

### Problem Statement
The engine is architecturally mature but cannot yet be used as a rapid prototyping environment for research ideas. Rendering lacks a curated baseline for real-time shading experiments, runtime samples do not expose an application shell, tools are diagnostic-oriented instead of workflow-driven, and there is no curated path for ingesting reference datasets or benchmarking alternative algorithms. Without an integrated initiative, teams duplicate scaffolding and cannot compare results against related work efficiently.

### Proposed Solution
Deliver a coordinated initiative that synchronises rendering, runtime, tooling, assets, and benchmarking deliverables:

1. Establish a research-grade default rendering pipeline (`RE-610`) with post-processing, debug views, and telemetry hooks.
2. Ship a runtime-hosted prototyping harness (`RT-320`) exposing hot-reloadable scenes, camera controls, and instrumentation scripting.
3. Extend the tools module with an interactive sandbox for experiment configuration and metric capture (`TL-210`).
4. Provide versioned dataset packaging plus import recipes for canonical research meshes and scenes (`AS-330`).
5. Build an automated benchmarking matrix that can execute reference implementations alongside engine variants with unified telemetry exports (`CC-310`).

### Success Criteria
- Researchers can open the prototyping harness, load curated scenes, toggle algorithm variants, and capture benchmarks without writing glue code.
- Rendering pipelines, assets, and tools share a documented configuration format surfaced through the sandbox UI.
- Benchmark runs produce comparable telemetry and plots between engine implementations and reference scripts.
- Documentation describes workflow from dataset ingestion through benchmarking for at least two exemplar research studies.

### Priority Next Steps (P0 focus)
1. **`DC-041` — Kickoff readiness:** publish the milestone plan, assign risk owners with mitigation due dates, and link a kickoff brief across roadmap and sprint docs.
2. **`RT-321` — Case study validation:** deliver geometry and rendering case studies that run end-to-end through the harness and sandbox with telemetry baselines and regression coverage.
3. **`CC-311` — Comparative visualisation:** integrate benchmark plots and CI gating so AI-004 demonstrations include side-by-side metrics.

### Kickoff Readiness Summary
- **Milestone timeline:** See [`docs/backlog/active/AI_004_KICKOFF_BRIEF.md`](../../../../backlog/active/AI_004_KICKOFF_BRIEF.md) for the sequenced Phase 1 plan covering `DC-041` (kickoff packet, due 2026-02-07), `RT-320` (harness smoke, due 2026-02-12), `TL-210` (sandbox wiring, due 2026-02-14), and `AS-330` (dataset validation, due 2026-02-16).
- **Risk owners & deadlines (all due ≥1 week before the 2026-02-20 kickoff review):**
  - Coordinated schema delivery — @pm-agent — mitigation deck due **2026-02-09**.
  - Dataset licensing approvals — @assets-lead — licensing checkpoint due **2026-02-13**.
  - Comparative plotting performance — @perf-lead — CI smoke sizing decision due **2026-02-13**.
  - Harness coverage assurance — @runtime-lead — schema + telemetry smoke confirmation due **2026-02-13**.
- **Sprint tracker cross-link:** [`SPRINT_11_ACTIVE.md`](../../../../backlog/active/SPRINT_11_ACTIVE.md) surfaces the brief in the current coordination checklist.
- **Coordination cadence:** Weekly runtime/tools/performance syncs continue through the kickoff review; slide deck outline published **2026-02-09** alongside the milestone plan.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::rendering`
- `engine::runtime`
- `engine::tools`
- `engine::assets`
- `scripts::diagnostics`
- `python::engine3g`

**Files to Modify:**
- `docs/ROADMAP.md`
- `docs/modules/*/README.md` for the affected modules
- `docs/archive/backlog/legacy/tasks/RE-610-*.md`, `RT-320-*.md`, `TL-210-*.md`, `AS-330-*.md`, `CC-310-*.md`
- `python/scripts` benchmarking utilities (exact files TBD during implementation)

**New Files:**
- Benchmark configuration schema and templates under `docs/design/`
- Sample scene packages referenced from `assets/`

### Dependencies
**Depends On:**
- Completion of `AI-001` handle validation (ensures hot reload safety)
- Stable telemetry schema (`CC-001`)
- Schema alignment task `DC-040` and ADR-0007 sign-off

**Blocks:**
- `RE-610` Research Rendering Baseline
- `RT-320` Runtime Prototyping Harness
- `TL-210` Experiment Sandbox UI
- `AS-330` Reference Dataset Packages
- `CC-310` Comparative Benchmark Automation

### Related Work
- Initiative: `AI-001`, `AI-002`, `AI-003`
- Specs: `docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`
- Tasks: `CO_170_RUNTIME_INTEGRATION_SAMPLE.md` (foundation for prototyping harness)

---

## Acceptance Criteria

### Functional Requirements
- [ ] Coordinated roadmap published with milestones for `RE-610`, `RT-320`, `TL-210`, `AS-330`, `CC-310`.
- [x] Shared configuration schema defined and reviewed by rendering/runtime/tools leads.
- [ ] Roadmap risk register lists owners and due dates for kickoff blockers.
- [ ] Prototype workflow validated on two research case studies (e.g., geometry processing algorithm and rendering technique).
- [ ] Benchmark automation produces side-by-side telemetry and plots for engine vs. reference implementations.

### Non-Functional Requirements
- [ ] Performance: Benchmark harness overhead ≤ 5% of total runtime.
- [ ] Memory: Prototyping harness maintains ≤ 15% overhead versus standalone runtime.
- [ ] Latency: Hot reload or configuration updates apply within 2 frames.

### Testing Requirements
- [x] Integration tests for prototyping harness exercises sample scenes.
- [ ] Smoke tests for sandbox UI run in CI.
- [ ] Benchmark automation validated through deterministic golden outputs.
- [ ] Coverage ≥ 85% on touched engine/tooling code.

### Documentation Requirements
- [ ] `docs/ROADMAP.md` updated with initiative timeline and task mapping.
- [x] Module READMEs reflect new workflows and entry points.
- [ ] Prototyping playbook added under `docs/design/` with screenshots and scripts.
- [ ] Benchmark quickstart guide published alongside dataset instructions.

---

## Test Plan

### Unit Tests
```cpp
TEST(RuntimePrototypeHarness, LoadsReferenceScene) {
    // Boot runtime harness with reference dataset.
    // Assert camera controls and render loop tick.
}
```

### Integration Tests
- Validate prototyping harness from CLI launch to benchmark export using scripted automation.
- Ensure rendering baseline toggles debug visualisations without invalidating frame graph caches.

### Performance Tests
- Capture GPU/CPU telemetry on target hardware for two benchmark scenes; compare to reference scripts with ≤5% variance.

---

## Implementation Notes

### Design Considerations
- Shared configuration uses YAML with strict schema validation to ease cross-module consumption.
- Benchmark exports reuse telemetry viewer infrastructure to minimise tooling duplication.
- Reference datasets distributed as git-lfs packages with version manifest to keep automation deterministic.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Divergent module timelines delay integration | Medium | High | Timebox each module milestone and gate promotion on cross-module demos.
| Benchmark reproducibility compromised by driver variance | Medium | Medium | Capture GPU/driver metadata and provide containerised runners for CI.
| Dataset licensing uncertainty | Low | High | Curate assets with permissive licenses and document provenance.

### Alternative Approaches
1. **Isolated module deliverables**: treat each module separately without shared schema → rejected because integration pain persists.
2. **External prototyping tool**: build standalone research app outside engine → rejected due to duplicated effort and maintenance burden.

---

## Deliverables

- [ ] Roadmap + module README updates
- [ ] Updated tasks for each sub-workstream
- [ ] Shared configuration schema & documentation
- [ ] Risk register entries reflecting owners/due dates for outstanding blockers
- [ ] Kickoff brief published and cross-linked from roadmap + sprint tracker
- [x] Dataset manifest and ingestion scripts
- [ ] Benchmark automation scripts + CI integration
- [ ] Demo recordings/screenshots for research workflow
- [ ] PRs linked to `AI-004`
- [ ] CI green with benchmark smoke coverage

---

## Definition of Done

- [ ] Builds cleanly on CI (Clang-22, MSVC)
- [ ] All tests pass (unit, integration, sanitizers)
- [ ] Performance regression ≤ 2%
- [ ] Code coverage ≥ 85% on touched lines
- [ ] Documentation updated and reviewed
- [ ] Code review approved by Tech Lead(s)
- [ ] PRs merged to main and roadmap status advanced

---

## Assigned To
**Role**: Product Manager → Orchestrates cross-module coordination
**Name**: @pm-agent

## Estimated Timeline
**Start Date**: 2025-11-20
**Target Completion**: 2026-01-31
**Actual Completion**: _TBD_

---

## Notes
- Schedule bi-weekly demos to maintain cross-module alignment.
- Leverage telemetry viewer CLI for quick benchmark visualisation until UI work lands.
- Capture learnings in retrospective to seed future research partnerships.
- Harness CLI now exposes `--require-schema` to enforce configuration headers
  per run while `ENGINE_AI004_SCHEMA_V1` remains opt-in during the migration
  period. Module READMEs document the migration steps for runtime, rendering,
  tools, and assets.
- Harness CLI now exports configuration descriptors (`--describe-json`) and
  dry-run execution summaries (`--summary-json`) so TL-210 can populate sandbox
  selectors and CC-310 can ingest benchmark outputs without bespoke parsers.
- Sample AI-004 configuration published at `docs/examples/ai004_sample.json`,
  pairing the harness CLI with the `assets/datasets/remesh_sample` package to
  streamline dry-run validation for new contributors.
- Bundled case studies (`geometry-baseline`, `rendering-debug`) now live under
  `assets/datasets/case_studies/` with an index consumed by
  `scripts.prototyping.run_prototype_harness --case-study <id>`, giving runtime
  and tools teams reproducible manifests for RT-321 dry runs.
- Geometry remeshing statistics/telemetry emit triangle quality metrics so dataset
  packaging can flag sliver-heavy assets before entering AI-004 workflows.
- Dataset packaging workflows use `scripts/datasets/ingest_dataset.py` to copy
  manifests into deterministic caches with checksum metadata validated by
  regression tests, seeding `AS-330` deliverables with a sample manifest in
  `assets/datasets/remesh_sample`.
- CI now exercises the AI-004 harness CLI end-to-end via `pytest
  python/tests/test_prototype_harness.py::test_cli_integration_with_sample_assets`,
  guarding configuration schema alignment across runtime, rendering, and tooling
  workstreams.
- CTest now runs `runtime_prototype_harness_geometry_case_study`, invoking the
  harness CLI with the geometry case study in dry-run mode while enforcing
  schema headers and normalising module search paths so the kickoff review
  inherits deterministic regression coverage alongside the existing pytest
  suite.
- New tasks `DC-041`, `RT-321`, and `CC-311` capture kickoff planning, case study validation, and comparative reporting deliverables to unblock the P0 milestone.
