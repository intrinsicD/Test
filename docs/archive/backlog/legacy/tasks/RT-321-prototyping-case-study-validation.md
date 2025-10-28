# Task Card: RT-321

## Title
Prototyping Harness Case Study Validation

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
2 weeks (runtime + assets + tools coordination)

---

## Description

### Problem Statement
`RT-320` delivered the core prototyping harness but the AI-004 acceptance criteria still require two validated research case studies with reproducible metrics. Without curated scenarios, integration tests, and benchmark baselines, downstream teams cannot rely on the harness to evaluate geometry or rendering algorithms ahead of the kickoff demo.

### Proposed Solution
Author two end-to-end case studies (one geometry-focused, one rendering-focused) that exercise the harness through the shared configuration schema. Each scenario should load curated datasets from `AS-330`, configure the `RE-610` baseline, capture telemetry, and export benchmark artefacts for `CC-310`. Extend the CLI and sandbox bindings as needed, add integration tests, and document the workflow.

### Success Criteria
- Harness CLI and sandbox UI can execute each case study via declarative manifests.
- Telemetry captures performance (FPS, GPU/CPU time) and quality metrics for every run.
- Regression coverage protects case study execution with deterministic baselines.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::runtime`
- `engine::scene`
- `engine::rendering`
- `engine::assets`
- `tools::sandbox`
- `python::engine3g`

**Files to Modify:**
- `engine/runtime/samples/` harness entry points
- `python/scripts/prototyping/` CLI wrappers
- `scripts/tests/` + `python/tests/` integration suites
- `docs/design/RT-320-prototyping-harness.md`
- `docs/modules/runtime/README.md`
- `docs/archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md`

**New Files:**
- `assets/datasets/<case-study>/manifest.yaml`
- `scripts/prototyping/case_studies/<name>.json`
- `docs/design/RT-321-case-studies.md`

### Dependencies
**Depends On:**
- `RT-320` harness infrastructure
- `AS-330` dataset packaging
- `DC-040` configuration schema alignment

**Blocks:**
- `CC-310` comparative benchmark visualisation
- `TL-210` sandbox demo checklist

### Related Work
- `docs/archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md`
- `docs/archive/backlog/legacy/tasks/AS-330-reference-dataset-packages.md`
- `docs/archive/backlog/legacy/tasks/TL-210-experiment-sandbox-ui.md`

---

## Acceptance Criteria

### Functional Requirements
- [x] Deliver two case study manifests (geometry + rendering) stored under `assets/datasets/` and referenced by the harness.
- [x] Extend harness CLI with a `--case-study <id>` shortcut that loads presets, datasets, and telemetry export destinations.
- [ ] Sandbox UI enumerates the case studies with pre-baked parameter sets and benchmark capture toggles.
- [x] Integration test exercises each case study end-to-end (configure → run → export metrics) using deterministic seeds.

### Non-Functional Requirements
- [ ] Performance: Each case study reports baseline FPS ±2% across repeated runs.
- [ ] Memory: Harness allocates ≤15% overhead compared to manual scene setup.
- [ ] Latency: Configuration reload applies within two frames when switching case studies.

### Testing Requirements
- [x] Add `pytest` coverage for CLI case study execution.
- [x] Extend `ctest` integration harness to run at least one case study in headless mode.
- [ ] Update CI pipeline to publish benchmark artefacts for the case studies.

### Documentation Requirements
- [ ] Document case study workflow in `docs/design/RT-320-prototyping-harness.md` (or new RT-321 design note).
- [ ] Update runtime, rendering, and tools module READMEs with case study quickstarts.
- [ ] Reference case studies and expected metrics in the AI-004 initiative card.

---

## Test Plan

### Unit Tests
```cpp
TEST(RuntimePrototypeHarness, LoadsGeometryCaseStudy) {
    // Configure harness for geometry case study and verify execution + telemetry outputs.
}
```

### Integration Tests
- CLI smoke test invoking `python -m scripts.prototyping.run_prototype_harness --case-study geometry-baseline --summary-json`.
- Sandbox UI automation script (headless) toggling the rendering case study and exporting metrics.

### Performance Tests
- Use `scripts/benchmarks/run_comparative_benchmarks.py` to capture baseline metrics for each case study and record them under `docs/design/RT-321-case-studies.md`.

---

## Implementation Notes

### Design Considerations
- Reuse dataset ingestion tooling (`scripts/datasets/ingest_dataset.py`) to avoid duplicate manifest handling.
- Normalise telemetry channel names so CC-310 visualisation can ingest outputs without adapters.
- Provide fallback asset packs for CI to keep runtime deterministic even when full datasets are unavailable.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Dataset licensing for case studies stalls integration | Medium | High | Coordinate with AS-330 owner to prioritise permissive assets |
| Sandbox automation flakes in CI | Medium | Medium | Add headless smoke test with retries and deterministic seeds |

### Alternative Approaches
1. **Manual documentation only**: describe case studies without automation → rejected because acceptance criteria demand reproducible runs.
2. **Split into per-module tasks**: separate runtime/tools/assets tickets → rejected to keep cross-module coordination under one lead for the kickoff deadline.

---

## Deliverables
- [ ] Case study manifests + ingestion scripts
- [ ] Harness CLI `--case-study` entry point
- [ ] Sandbox UI bindings
- [x] Integration + CI coverage
- [ ] Updated documentation and initiative card

---

## Definition of Done
- [ ] All new/updated tests pass in CI
- [ ] Telemetry captures stored with documented baselines
- [ ] Documentation updates reviewed by runtime, assets, and tools leads
- [ ] References to case studies added to AI-004 kickoff brief

---

## Assigned To
**Role**: Runtime Lead (with assets/tools support)
**Name**: @runtime-lead

## Estimated Timeline
**Start Date**: 2025-12-29
**Target Completion**: 2026-01-16
**Actual Completion**: _TBD_

---

## Notes
- Align scenario selection with research stakeholders so demo content reflects upcoming publications.
- Capture screenshots or short clips for the kickoff review deck once scenarios stabilise.
- 2025-12-28: `assets/datasets/case_studies/index.json` tracks the bundled presets and
  `scripts.prototyping.run_prototype_harness --case-study <id>` loads
  `geometry-baseline` and `rendering-debug`, exporting JSON summaries during
  dry runs and providing pytest coverage via `test_cli_case_study_support`.
- 2025-12-29: Added `ctest` integration target `runtime_prototype_harness_geometry_case_study`
  invoking the harness CLI with `--case-study geometry-baseline --dry-run --require-schema`
  so AI-004 case studies execute under `ctest` with regression coverage, write
  JSON summaries into the build artefacts directory, and normalised the CLI
  search paths so `engine3g` modules resolve when the harness runs from the
  build tree.
