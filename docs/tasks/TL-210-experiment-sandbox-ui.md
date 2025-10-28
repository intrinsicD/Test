# Task Card: TL-210

## Title
Experiment Sandbox UI

## Type
- [x] Feature
- [ ] Bug Fix
- [x] Refactor
- [x] Documentation
- [x] Research
- [ ] Performance Optimization

## Priority
- [x] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
2.5 weeks (tools + UI integration)

## Status
🟡 In Progress — Experiment sandbox UI scaffolding merged: dataset/preset browser, rendering controls, benchmark trigger callbacks, telemetry panels, and persistence helpers shipped. Harness wiring and benchmark automation remain outstanding.

---

## Description

### Problem Statement
The tools module currently provides diagnostics viewers but no interactive workspace for configuring experiments, switching algorithm variants, or capturing benchmark snapshots. Researchers rely on ad-hoc ImGui windows or manual configuration edits, which is error-prone and lacks reproducibility hooks.

### Proposed Solution
Build an ImGui-powered sandbox application embedded within the prototyping harness (`RT-320`). Provide panels for dataset selection, rendering preset toggles, algorithm parameter sliders, telemetry graphs, and benchmark capture controls. Persist layouts to disk, expose scriptable hooks, and integrate with benchmarking automation (`CC-310`).

### Success Criteria
- Sandbox UI detects available datasets, rendering presets, and algorithm variants dynamically.
- Researchers can configure experiments, trigger benchmark runs, and export snapshots without editing configuration files.
- Telemetry charts update in real time and persist to session logs for reproducibility.
- UI customisation stored per-user enabling quick recall of layouts.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::tools`
- `engine::runtime`
- `engine::rendering`
- `engine::assets`
- `python::engine3g` (scripting bindings)

**Files to Modify:**
- `engine/tools/include/...` UI utilities
- `engine/tools/src/...`
- `engine/tools/tests/`
- `docs/modules/tools/README.md`
- `docs/ROADMAP.md`
- `python/scripts/telemetry_viewer.py`

**New Files:**
- `engine/tools/include/engine/tools/sandbox/experiment_sandbox.hpp`
- `engine/tools/src/sandbox/experiment_sandbox.cpp`
- `docs/design/TL-210-experiment-sandbox.md`

### Dependencies
**Depends On:**
- `RT-320` harness scaffolding
- `RE-610` telemetry exports
- `CC-001` telemetry viewer infrastructure

**Blocks:**
- `CC-310` benchmark automation UI integration

### Related Work
- `scripts/diagnostics/telemetry_viewer.py`
- `docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md`
- `T-0121-rendering-standard-passes-library.md`

---

## Acceptance Criteria

### Functional Requirements
- [x] Sandbox enumerates datasets, rendering presets, and algorithm variants from configuration manifests.
- [x] UI provides parameter editing with validation and immediate feedback.
- [x] Telemetry charts display FPS, GPU time, memory usage, and algorithm-specific metrics.
- [ ] Benchmark capture button triggers headless run and surfaces success/failure summaries.

### Non-Functional Requirements
- [ ] Performance: UI update cost ≤ 1 ms/frame on target hardware.
- [ ] Memory: Sandbox retains ≤ 200 MB additional footprint.
- [ ] Latency: UI interactions propagate to runtime within 1 frame.

### Testing Requirements
- [x] Tools module unit tests cover widget validation and layout persistence.
- [ ] Integration test verifies sandbox ↔ runtime communication.
- [ ] Golden screenshot tests guard UI regressions (where feasible).
- [ ] Coverage ≥ 85% on new tooling code.
- [ ] Telemetry load benchmark simulates ≥5 concurrent comparative runs with no more than 5% frame time regression while charts stream live metrics.

### Documentation Requirements
- [x] Update tools README with sandbox usage and screenshots.
- [x] Add UI customization guide referencing layout persistence.
- [ ] Document telemetry charts and benchmark workflow in prototyping playbook.
- [ ] Provide accessibility checklist for UI components.

---

## Test Plan

### Unit Tests
```cpp
TEST(ExperimentSandbox, PersistsLayoutPreferences) {
    ExperimentSandbox sandbox;
    sandbox.SaveLayout("test_layout.json");
    sandbox.LoadLayout("test_layout.json");
    EXPECT_TRUE(sandbox.HasLayout("test_layout.json"));
}
```

### Integration Tests
- Launch sandbox with prototyping harness and ensure UI controls update runtime state.
- Execute benchmark capture from UI and validate output files.

### Performance Tests
- Profile UI update loop using Tracy zones to ensure ≤1 ms/frame overhead.
- Execute telemetry load test while recording benchmark runs; verify streaming buffers avoid dropped samples and stay within target latency.

---

## Implementation Notes

### Design Considerations
- Use modular widget system so research teams can plug in new controls without recompiling base UI.
- Store configuration in YAML/JSON shared with runtime harness.
- Provide keyboard shortcuts and command palette for power users.
- Validate sandbox-generated manifests with the shared schema tooling:
  `scripts.validate_ai004_config` and the prototyping harness `--require-schema`
  option ensure UI exports remain compatible once strict validation is
  mandatory.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| UI complexity overwhelms users | Medium | Medium | Provide progressive disclosure and templated layouts.
| Telemetry streaming overloads main thread | Low | Medium | Buffer updates and throttle chart refresh rate.
| Layout persistence conflicts with git-managed configs | Low | Low | Store layouts in user config directory.

### Alternative Approaches
1. **CLI-only workflow** → rejected; fails to provide interactive exploration.
2. **External web UI** → rejected; requires additional infrastructure and duplicates telemetry pipeline.

---

## Deliverables

- [x] Sandbox UI implementation + tests
- [x] Widget library + documentation
- [x] Telemetry chart integration
- [ ] Benchmark control workflow
- [ ] Screenshots/videos for docs
- [ ] Linked PRs referencing `TL-210`

---

## Definition of Done

- [ ] Tools CI suites green including new UI tests
- [ ] Sandbox validated in research dry run
- [ ] Documentation reviewed by tools + research leads
- [ ] UI accessibility checklist completed
- [ ] Benchmark automation integrated with UI

---

## Assigned To
**Role**: Tools Engineer
**Name**: @tools-lead

## Estimated Timeline
**Start Date**: 2025-12-09
**Target Completion**: 2025-12-31
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with design to ensure UI conventions align with diagnostics viewer.
- Consider plugin system for algorithm-specific widgets.
- 2025-12-19: Harness CLI gained `--describe-json`/`--summary-json` exports so the sandbox can ingest dataset/preset metadata
  and render execution summaries without bespoke parsers.
- 2025-12-20: Landed `ExperimentSandbox` scaffolding with dataset browser, rendering controls, telemetry panel, benchmark
  callbacks, and preference/layout persistence. Tools documentation and architecture notes updated alongside unit coverage.
- 2025-12-21: Added a JSON configuration loader that ingests harness summaries into `ExperimentSandbox`, flattening metrics and
  runtime descriptors so UI integrations can hydrate datasets, presets, and camera/simulation notes directly from
  `run_prototype_harness.py --describe-json` outputs.
