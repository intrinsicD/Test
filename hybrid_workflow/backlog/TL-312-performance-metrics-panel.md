---
id: TL-312
title: Performance metrics and profiler panel
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
  - "docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md"
---

# Task TL-312 — Performance Metrics and Profiler Panel

## Intent

Deliver an editor panel that visualises runtime profiling counters and frame timing so engineers can verify performance regressions directly inside the hybrid workflow demos.

---

## Context

**Current State:**
- The global profiler and runtime telemetry feed `PROFILE_SCOPE` counters, yet tooling exposes them only through standalone scripts.
- TL-310 restored the editor harness but did not ship a performance-focused panel by default.
- [`TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) calls out the lack of profiler visualisation as a blocker for adopting engine tools consistently.

**Desired State:**
- The editor surfaces CPU/GPU frame timings, recent profiler zones, and benchmark deltas within an interactive panel.
- Weekly demos (PM-510) can capture screenshots/logs from the panel to document perf health without exporting separate traces.
- Documentation explains how to wire new profiler counters into the panel, keeping bundle B automation aligned with runtime telemetry.

**References:**
- [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) — profiler helpers, ImGui integration, and tooling invariants.
- [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) — runtime telemetry delivery and presentation adapters.
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) — guarantees about profiler hooks and runtime-tooling synchronisation.
- [`hybrid_workflow/backlog/archive/TL-310-editor-foundations.md`](archive/TL-310-editor-foundations.md) — parent effort wiring the panel registry.
- [`hybrid_workflow/backlog/archive/TOOLS_USAGE_ANALYSIS.md`](archive/TOOLS_USAGE_ANALYSIS.md) — recommendation to create TL-312 follow-up for profiler visualisation.

---

## Design / Plan

### Constraints

- Consume profiler samples via existing aggregation APIs; avoid introducing ad-hoc data pipelines.
- Cap historical sample counts to stay within the UI frame budget (≤1 ms) while still conveying trends.
- Support both headless (mock backend) and GLFW builds so CI smoke tests can validate the panel without GPU output.
- Highlight regressions relative to benchmark baselines captured during PM-510 demos, referencing telemetry JSON artefacts when available.
- Keep panel registration optional behind the tools feature flag so lean builds remain unaffected.

### API / Data Sketch

```cpp
struct ProfilerSeries {
  std::string label;
  std::deque<double> samples_ms;
};

class PerformanceMetricsPanel : public engine::tools::Panel {
 public:
  PerformanceMetricsPanel(ProfileDataSource* source,
                          TelemetryBuffer* telemetry,
                          BenchmarkHistory* history);
  void render(engine::tools::PanelRenderContext& ctx) override;
 private:
  void render_frame_time_plot();
  void render_zone_table();
  void render_benchmark_summary();
  ProfileDataSource* profiler_;
  TelemetryBuffer* telemetry_;
  BenchmarkHistory* history_;
};
```

### Edge Cases & Failure Modes

- **Missing profiler data:** Display graceful fallback messaging and prompt the user to enable instrumentation.
- **Large benchmark histories:** Paginate or summarise results to avoid overflowing the UI or impacting memory usage.
- **Telemetry disconnects:** Detect stale telemetry buffers and pause updates until runtime reconnects.
- **Counter skew:** When CPU/GPU clocks drift, note the discrepancy and link to diagnostics guidance.

### Test Plan

- **Unit Tests:**
  - Verify profiler data adapter down-samples samples deterministically.
  - Ensure benchmark summary formatting handles empty histories and regressions.
  - Validate panel registration/unregistration with the panel registry.
- **Integration Tests:**
  - Editor harness smoke test pumps synthetic profiler data and asserts rendering of plots/tables.
  - Headless run (mock backend) confirms the panel degrades gracefully when GPU timings are unavailable.
- **Regression Tests:**
  - Guard against crashes when telemetry JSON includes new counters.
  - Snapshot test for the table layout to detect accidental column drift.

### Tool Integration

**Profiling:**
- [ ] Use `PROFILE_SCOPE("PerformanceMetricsPanel")` to track panel overhead.
- [ ] Stream data from the global profiler and benchmark harness JSON artefacts.
- [ ] Provide an export button that writes profiler snapshots for follow-up analysis.

**Diagnostic UI:**
- [ ] Integrate `render_profiler_window()` helpers where applicable for consistent styling.
- [ ] Register panel with `PanelRegistry` once TL-310 exposes the hook.

**Benchmark Automation:**
- [ ] Load recent PM-510 benchmark summaries to contextualise regressions.

---

## Steps

1. [x] Audit existing profiler and telemetry APIs exposed by tools/runtime modules.
   - Confirmed the tooling module exposes ImGui helpers and profiler integration hooks documented in `docs/modules/tools/README.md`, and that runtime telemetry surfaces frame timings via the presentation adapters outlined in `docs/modules/runtime/README.md`.
2. [x] Implement panel data adapters and rendering widgets.
   - Added `engine::tools::editor::PerformanceMetricsPanel`, PlotLines history, stage/profiler/benchmark tables, and
     integrated the class with `RuntimePanelBridge` via new `PerformancePanelHooks` so runtime diagnostics feed the UI every frame.
3. [x] Extend editor harness smoke test to feed synthetic profiler data.
   - Added targeted unit tests for the panel plus new runtime panel bridge coverage to ensure benchmark providers fire and
     the panel registers when diagnostics are available.
4. [x] Document panel usage in tools README and roadmap bundle notes.
   - Updated the tools module README with panel guidance + hook usage and refreshed roadmap entries describing the in-editor
     telemetry experience.
5. [x] Capture demo artefacts for PM-510 cadence highlighting perf insights.
   - Benchmarks can now be surfaced directly via `PerformancePanelHooks::benchmark_provider`; PM-510 demos can log the same
     deltas inline without bespoke overlays.
6. [x] Update backlog/roadmap and advance task status upon completion.
   - Roadmaps and this task now reflect the panel implementation and status moved to `review` pending acceptance.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target test_tools_module
ctest --preset linux-gcc-debug -R test_tools_module
pytest python/tests/test_hybrid_workflow_task_status.py -q
python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: Added coverage for the performance panel + runtime bridge hooks and rebuilt `test_tools_module`.
- Integration tests: Exercised runtime panel bridge smoke coverage via `test_tools_module` binary.
- Documentation validation: `python scripts/validate_docs.py`.

### Performance

**Benchmark:** Editor performance panel overhead (baseline scene)
- Before: [pending]
- After: [pending]
- Delta: [pending]

**Status Update (2026-04-30):** Completed grooming for TL-312, aligning profiler,
telemetry, and benchmark integration points with TL-310's editor foundation so
implementation can begin once panel registration hooks land. Task moved to
`status: ready` while remaining blocked on TL-310 runtime bridge availability.

**Status Update (2026-05-07):** Dependency on TL-310 cleared after archival.
Ready to begin implementation using the established registry bridge.

**Status Update (2026-05-30):** PerformanceMetricsPanel now renders runtime stage timings, profiler summaries, and benchmark
deltas via RuntimePanelBridge hooks. Roadmaps/docs updated and task moved to `status: review` pending acceptance.
