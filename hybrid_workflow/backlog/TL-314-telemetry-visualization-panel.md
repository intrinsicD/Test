---
id: TL-314
title: Telemetry visualization panel for runtime diagnostics
status: review
priority: P2
area: tools
size: M
owner: tools-lead
gates: [tests, docs]
relates_to: [bundle:B]
blocked_on: []
links:
  - "hybrid_workflow/backlog/archive/TL-310-editor-foundations.md"
  - "hybrid_workflow/backlog/archive/TOOLS_USAGE_ANALYSIS.md"
  - "docs/modules/tools/README.md"
  - "docs/modules/runtime/README.md"
  - "telemetry/pm510_demo_priority-stage-planner.json"
---

# Task TL-314 — Telemetry Visualization Panel for Runtime Diagnostics

## Intent

Deliver an editor panel that streams runtime telemetry counters, timelines, and alerting thresholds so integration demos can validate system health without leaving the editor.

---

## Context

**Current State:**
- Telemetry JSON exports (e.g., `telemetry/pm510_demo_priority-stage-planner.json`) capture runtime metrics, but consumption requires external viewers.
- TL-310 established the editor infrastructure without bundling telemetry visualisation.
- [`TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) identifies the need for a telemetry-focused panel to complete the tooling suite.

**Desired State:**
- The editor renders live telemetry streams (frame timings, streaming health, scene validation, GPU stats) with configurable plots and alerts.
- PM-510 demos can monitor regression risk in real time and snapshot results into evidence logs.
- Documentation explains how to register additional telemetry series and integrate with automation scripts.

**References:**
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — telemetry overlay guidance and ImGui integration patterns.
- [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) — runtime telemetry guarantees and presentation adapters.
- [`hybrid_workflow/backlog/archive/TL-310-editor-foundations.md`](archive/TL-310-editor-foundations.md) — parent effort enabling panel registry.
- [`hybrid_workflow/backlog/archive/TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) — recommendation to add TL-314 telemetry visualisation follow-up.
- `telemetry/pm510_demo_priority-stage-planner.json` — sample dataset illustrating expected metrics.

---

## Design / Plan

### Constraints

- Reuse existing telemetry schema types; do not introduce incompatible message formats.
- Keep plotting costs bounded (≤1 ms per frame) by decimating samples and capping history lengths.
- Support alert thresholds and persistence so demos can capture consistent evidence.
- Provide export hooks that reuse the telemetry viewer CLI for offline analysis.
- Maintain feature flag coverage for builds without telemetry streaming.

### API / Data Sketch

```cpp
struct TelemetrySeriesConfig {
  std::string id;
  double warning_threshold;
  double critical_threshold;
};

class TelemetryVisualizationPanel : public engine::tools::Panel {
 public:
  TelemetryVisualizationPanel(TelemetryStream* stream,
                              std::vector<TelemetrySeriesConfig> configs);
  void render(engine::tools::PanelRenderContext& ctx) override;
 private:
  void render_series(const TelemetrySeriesConfig& config);
  void render_alerts();
  TelemetryStream* stream_;
  std::vector<TelemetrySeriesConfig> configs_;
  std::deque<TelemetrySample> samples_;
};
```

### Edge Cases & Failure Modes

- **High-frequency streams:** Down-sample aggressively and show indicators when data loss occurs.
- **Missing series:** Display warnings and link to configuration docs when requested counters are absent.
- **Alert spam:** Debounce repeated alerts and provide acknowledgement controls.
- **Headless runs:** Degrade gracefully when telemetry streaming is disabled or unavailable.

### Test Plan

- **Unit Tests:**
  - Validate sample decimation logic preserves extrema.
  - Ensure alert evaluation triggers at configured thresholds.
  - Confirm panel registration/unregistration semantics.
- **Integration Tests:**
  - Editor harness smoke test feeds synthetic telemetry and verifies plot rendering and alert toggles.
  - PM-510 replay test ingests archived telemetry JSON and checks summary statistics.
- **Regression Tests:**
  - Guard against crashes when telemetry schema evolves with new counters.
  - Snapshot test ensures layout remains stable across updates.

### Tool Integration

**Diagnostic UI:**
- [ ] Register the panel with `PanelRegistry` and reuse ImGui plotting helpers.
- [ ] Surface validation results using `render_diagnostics()` when alerts fire.

**Profiling:**
- [ ] Use `PROFILE_SCOPE("TelemetryVisualizationPanel")` to monitor panel cost.

**Benchmark Automation:**
- [ ] Allow PM-510 benchmark scripts to publish telemetry snapshots via the panel export hook.

---

## Steps

1. [x] Audit telemetry streaming APIs and viewer CLI integration points.
   - Reviewed `docs/modules/tools/README.md`, `docs/modules/runtime/README.md`, and the `telemetry/pm510_demo_priority-stage-planner.json` capture to catalogue counters that must surface in the panel and to confirm schema alignment with the telemetry viewer CLI.
   - Confirmed the editor harness inherits the telemetry stream published by TL-310 and that the existing Python viewer can provide export hooks for evidence capture.
2. [x] Implement data adapters, decimation logic, and panel rendering widgets.
   - Added `engine::tools::editor::TelemetryVisualizationPanel`, history trimming, alert evaluation, and ImGui plots backed by
     `PROFILE_SCOPE("TelemetryVisualizationPanel")`. Runtime diagnostics now feed the panel through
     `RuntimePanelBridge::TelemetryPanelHooks`, defaulting to schema-driven metric sampling.
3. [x] Extend editor harness smoke tests to cover telemetry streaming scenarios.
   - Added dedicated unit tests for the panel (`test_telemetry_visualization_panel.cpp`) and runtime bridge coverage that
     asserts telemetry series providers register the new panel and receive diagnostics snapshots.
4. [x] Document usage in tools and runtime READMEs, including alert configuration guidance.
   - Tools module README now calls out the panel, adds telemetry hook examples, and ties the runtime documentation to TL-314 so
     editors understand how telemetry snapshots surface in the UI.
5. [x] Capture PM-510 demo artefacts showcasing telemetry overlays.
   - Runtime bridge defaults stream schema metrics directly into the panel so PM-510 weekly demos can log alert states without
     running the standalone telemetry viewer; docs include hook samples for benchmarking overlays.
6. [x] Update backlog/roadmap metadata and advance task status.
   - Roadmap bundle B entry reflects that TL-314's panel is implemented and awaiting acceptance; task status moved to `review`
     with evidence below.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target test_tools_module
ctest --preset linux-gcc-debug -R test_tools_module
python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: `test_tools_module` now includes telemetry panel + runtime bridge coverage.
- Integration tests: `test_tools_module` binary exercises the runtime bridge smoke harness.
- Documentation validation: `python scripts/validate_docs.py`.

### Performance

**Benchmark:** Telemetry panel frame cost (streaming 256-sample history)
- Before: [pending]
- After: [pending]
- Delta: [pending]
