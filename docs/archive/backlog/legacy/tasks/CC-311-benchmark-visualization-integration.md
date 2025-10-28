# Task Card: CC-311

## Title
Benchmark Visualization Integration

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
1.5 weeks (performance + tools collaboration)

---

## Description

### Problem Statement
`CC-310` introduced the comparative benchmark orchestrator, but AI-004 still lacks an integrated visualisation and CI gate that consumes orchestrator outputs. Without automated plots, dashboards, and regression enforcement, the initiative cannot demonstrate side-by-side telemetry as required for the kickoff review.

### Proposed Solution
Extend the telemetry viewer and diagnostics tooling to ingest comparative benchmark outputs, generate shareable plots, and publish artefacts through CI. Wire the workflow into the AI-004 configuration schema, document usage, and ensure smoke coverage protects the new paths.

### Success Criteria
- Telemetry viewer renders comparative plots (FPS, GPU time, quality metrics) from orchestrator artefacts.
- CI job executes a reduced benchmark suite and fails on regressions beyond configured thresholds.
- Documentation guides researchers through generating and interpreting comparative reports.

---

## Technical Details

### Scope
**Modules Affected:**
- `scripts::diagnostics`
- `python::engine3g`
- `tools::telemetry`
- `docs::design`

**Files to Modify:**
- `scripts/diagnostics/telemetry_viewer.py`
- `scripts/benchmarks/run_comparative_benchmarks.py`
- `scripts/tests/test_telemetry_viewer.py`
- `python/tests/test_prototype_harness.py`
- `docs/design/CC-310-benchmark-playbook.md`
- `docs/archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md`

**New Files:**
- `scripts/diagnostics/templates/comparative_report.html`
- `docs/design/CC-311-visualisation-guide.md`

### Dependencies
**Depends On:**
- `CC-310` orchestrator outputs
- `RT-321` case study telemetry baselines
- `DC-040` configuration schema alignment

**Blocks:**
- AI-004 kickoff review demo requirements
- `TL-210` sandbox telemetry overlays

### Related Work
- `docs/archive/backlog/legacy/tasks/CC-310-comparative-benchmark-automation.md`
- `docs/archive/backlog/legacy/tasks/TL-210-experiment-sandbox-ui.md`
- `docs/archive/backlog/legacy/tasks/AI-004-application-prototyping-enablement.md`

---

## Acceptance Criteria

### Functional Requirements
- [ ] Add a telemetry viewer subcommand (e.g., `compare`) that loads orchestrator JSON/CSV outputs and renders plots to disk.
- [ ] Publish HTML/PNG artefacts for at least the two RT-321 case studies and store them under `assets/benchmarks/`.
- [ ] Create a CI job that runs a lightweight comparative suite and enforces ≤2% regression thresholds.
- [ ] Expose comparative report links through the sandbox UI telemetry panel.

### Non-Functional Requirements
- [ ] Performance: Report generation completes in ≤60 seconds for case study datasets.
- [ ] Memory: Tooling stays under 1.5 GB peak RSS when processing comparative artefacts.
- [ ] Latency: CI job adds ≤5 minutes to pipeline runtime.

### Testing Requirements
- [ ] Extend `scripts/tests/test_telemetry_viewer.py` with coverage for the new subcommand.
- [ ] Add golden artefact regression tests to ensure plots remain stable.
- [ ] Update `python/tests/test_prototype_harness.py` to emit comparative artefacts in smoke coverage.

### Documentation Requirements
- [ ] Update `docs/design/CC-310-benchmark-playbook.md` with the comparative workflow and troubleshooting tips.
- [ ] Document CI invocation and environment requirements.
- [ ] Reference comparative outputs and thresholds in the AI-004 initiative card.

---

## Test Plan

### Unit Tests
- Extend telemetry viewer unit tests to cover argument parsing and chart generation for the comparative subcommand.

### Integration Tests
- End-to-end CLI invocation: `python -m scripts.diagnostics.telemetry_viewer compare --input benchmarks/ai004/*.json --output reports/`.
- CI smoke test executed via new preset calling the orchestrator + viewer in sequence.

### Performance Tests
- Benchmark viewer execution on sample artefacts and record timings in the design guide.

---

## Implementation Notes

### Design Considerations
- Reuse existing matplotlib/plotly adapters within telemetry viewer to avoid duplicating plotting logic.
- Store HTML and static image outputs side by side so both web and offline workflows are covered.
- Parameterise thresholds and chart templates via configuration to keep CI and local runs aligned.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Plot generation introduces heavy dependencies | Low | Medium | Prefer existing plotting stack; document optional extras |
| CI runtime exceeds budget | Medium | High | Start with minimal datasets and allow nightly expansion |

### Alternative Approaches
1. **Manual notebooks**: require researchers to run Jupyter notebooks → rejected for lack of automation and CI coverage.
2. **External dashboard tooling**: integrate Grafana/Metabase → rejected due to infrastructure overhead relative to scope.

---

## Deliverables
- [ ] Telemetry viewer comparative subcommand
- [ ] CI job + preset executing comparative smoke suite
- [ ] Documentation updates and visualisation guide
- [ ] Reference artefacts stored under `assets/benchmarks/`

---

## Definition of Done
- [ ] Comparative plots generated for RT-321 case studies and linked from AI-004 docs
- [ ] CI job green on main and required for PRs touching AI-004 modules
- [ ] Documentation reviewed by performance + tools leads

---

## Assigned To
**Role**: Performance Lead (with tools support)
**Name**: @perf-lead

## Estimated Timeline
**Start Date**: 2026-01-02
**Target Completion**: 2026-01-19
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with runtime/tools to expose comparative artefact locations in sandbox UI.
- Work with infrastructure to size CI runners for plotting workload.
