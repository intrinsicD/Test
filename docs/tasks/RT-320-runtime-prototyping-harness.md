# Task Card: RT-320

## Title
Runtime Prototyping Harness

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
3 weeks (runtime + integration + scripting)

---

## Description

### Problem Statement
Runtime currently provides subsystem orchestration but no turnkey shell for experimenting with new rendering or geometry algorithms. Researchers must write bespoke applications to wire assets, configure scenes, and collect telemetry, dramatically slowing iteration and fragmenting workflows.

### Proposed Solution
Implement a reusable prototyping harness built on `RuntimeHost` that loads reference datasets, configures the rendering baseline (`RE-610`), exposes configurable camera/navigation controls, and integrates scripting hooks (Python/Lua) for algorithm toggles. Support headless benchmarking mode and interactive UI integration with the tools sandbox (`TL-210`).

### Success Criteria
- CLI launch loads reference scenes and selects algorithm variants via configuration files.
- Interactive mode exposes ImGui-based controls for toggling features and capturing telemetry snapshots.
- Headless mode executes scripted benchmark suites and exports comparable telemetry/plots.
- Harness packaged with documentation and sample scripts for two research case studies.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::runtime`
- `engine::scene`
- `engine::rendering`
- `engine::assets`
- `python::engine3g`
- `tools::` sandbox integration

**Files to Modify:**
- `engine/runtime/include/...` harness APIs
- `engine/runtime/src/...` bootstrapping logic
- `engine/runtime/tests/`
- `python/engine3g/loader.py`
- `docs/modules/runtime/README.md`
- `docs/ROADMAP.md`

**New Files:**
- `engine/runtime/samples/prototype_harness.cpp`
- `scripts/prototyping/run_benchmarks.py`
- `docs/design/RT-320-prototyping-harness.md`

### Dependencies
**Depends On:**
- `RE-610` baseline pipeline
- `AI-002` async streaming (complete)
- `CO-170` runtime integration sample

**Blocks:**
- `TL-210` UI binding
- `CC-310` benchmark orchestration

### Related Work
- `T-0104-runtime-frame-graph-integration.md`
- `T-0115-assets-async-streaming-mvp.md`
- `docs/specs/ADR-0004-runtime-harness.md` (if exists; otherwise create)

---

## Acceptance Criteria

### Functional Requirements
- [ ] Harness loads dataset manifest and sets up scene graph with streaming assets.
- [ ] Configuration schema supports algorithm toggles and parameter sweeps.
- [ ] Interactive mode exposes camera orbit/FPS controls, debug overlays, telemetry capture.
- [ ] Headless mode executes scenario list and writes telemetry/benchmark artefacts.

### Non-Functional Requirements
- [ ] Performance: Harness adds ≤ 5% CPU overhead vs. baseline runtime sample.
- [ ] Memory: Harness footprint ≤ 512 MB additional vs. base runtime.
- [ ] Latency: Configuration reload applies within 2 frames.

### Testing Requirements
- [ ] Integration test boots harness with mock dataset and verifies render submission.
- [ ] Python harness tests validate configuration parsing and CLI workflows.
- [ ] Regression tests ensure telemetry output schema stability.
- [ ] Coverage ≥ 85% on new runtime/scripting code.

### Documentation Requirements
- [ ] Update runtime README with harness instructions.
- [ ] Publish prototyping harness guide with CLI usage, config schema, and scripting examples.
- [ ] Record tutorial video/screenshots for onboarding.
- [ ] Cross-link dataset documentation from assets module.

---

## Test Plan

### Unit Tests
```cpp
TEST(RuntimePrototypeHarness, AppliesConfigurationOverrides) {
    PrototypeConfig config = LoadPrototypeConfig(test_config_path);
    RuntimePrototypeHarness harness{config};
    harness.Initialize();
    EXPECT_EQ(harness.SelectedAlgorithm(), "baseline");
}
```

### Integration Tests
- Launch harness in headless mode executing two benchmark scenarios and verify telemetry file generation.
- Execute interactive mode smoke test verifying UI toggles and render loop behaviour.

### Performance Tests
- Benchmark harness using Tracy integration to ensure CPU/GPU budgets satisfied; compare to baseline runtime sample results.

---

## Implementation Notes

### Design Considerations
- Use YAML/JSON schema shared with tools and rendering for configuration.
- Provide plugin interface for registering algorithm variants (DLL/shared library or script).
- Ensure deterministic seeding for reproducible benchmarks.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Complexity of configuration schema | Medium | Medium | Provide schema validation and extensive examples.
| Script integration destabilises runtime | Medium | High | Sandbox scripting environment, restrict to safe APIs.
| Asset streaming latency disrupts benchmarks | Low | Medium | Preload required assets or allow warm-up runs.

### Alternative Approaches
1. **Extend existing runtime sample** → insufficient separation between diagnostics and research workflows.
2. **External Python-only harness** → would bypass runtime integration and duplicate features.

---

## Deliverables

- [ ] Harness implementation + tests
- [ ] Configuration schema + documentation
- [ ] Sample scripts + datasets integration
- [ ] Telemetry export automation
- [ ] Updated docs + onboarding materials
- [ ] Linked PRs referencing `RT-320`

---

## Definition of Done

- [ ] CI builds/tests green including new integration suites
- [ ] Harness validated by research case studies
- [ ] Telemetry viewer displays harness metrics
- [ ] Documentation reviewed by runtime and research stakeholders
- [ ] Benchmarks reproducible in CI

---

## Assigned To
**Role**: Runtime Engineer
**Name**: @runtime-lead

## Estimated Timeline
**Start Date**: 2025-12-02
**Target Completion**: 2025-12-23
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with scripting maintainers for safe embedding.
- Provide containerised environment for benchmark reproducibility.
