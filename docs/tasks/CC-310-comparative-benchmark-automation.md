# Task Card: CC-310

## Title
Comparative Benchmark Automation

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
3 weeks (cross-module tooling + CI)

---

## Description

### Problem Statement
The engine lacks an automated workflow to benchmark engine implementations against external reference code. Existing telemetry tools capture metrics but there is no orchestration to run comparative workloads, normalise datasets, or publish dashboards for research validation.

### Proposed Solution
Develop a benchmarking matrix that orchestrates engine runs (via `RT-320` harness) and reference implementations (Python/C++ scripts). Integrate with telemetry viewer to produce comparative plots, store results in versioned artefacts, and surface regressions in CI. Provide configuration schema shared with sandbox UI (`TL-210`) and dataset manifests (`AS-330`).

### Success Criteria
- Benchmarks execute engine + reference workloads with identical datasets and configuration.
- Results exported to JSON/CSV + rendered plots for FPS, GPU time, quality metrics.
- CI pipeline runs smoke benchmarks and fails on regressions beyond thresholds.
- Documentation outlines workflow for adding new algorithms and interpreting results.

---

## Technical Details

### Scope
**Modules Affected:**
- `scripts::diagnostics`
- `python::engine3g`
- `engine::runtime`
- `engine::rendering`
- `engine::tools`

**Files to Modify:**
- `scripts/diagnostics/telemetry_viewer.py`
- `scripts/tests/*`
- `python/engine3g/`
- `docs/ROADMAP.md`
- `docs/design/`

**New Files:**
- `scripts/benchmarks/run_comparative_benchmarks.py`
- `docs/design/CC-310-benchmark-playbook.md`
- CI workflow updates under `.github/workflows/` (if applicable)

### Dependencies
**Depends On:**
- `AI-004` roadmap alignment
- `RT-320` harness headless mode
- `AS-330` dataset manifests
- `TL-210` UI integration (for manual runs)

**Blocks:**
- Research case studies publication
- Regression dashboards for telemetry viewer

### Related Work
- `scripts/diagnostics/runtime_frame_telemetry.py`
- `python/scripts/validate_docs.py` (docs validation pipeline)
- `CO-170-runtime-integration-sample.md`

---

## Acceptance Criteria

### Functional Requirements
 - [x] Benchmark orchestrator executes scenarios defined in shared configuration.
- [ ] Harness collects telemetry, merges with reference results, and outputs comparison artefacts.
- [ ] CI smoke job executes reduced benchmark suite and enforces thresholds.
- [ ] Dashboard/report generation summarises results for researchers.

### Non-Functional Requirements
- [ ] Performance: Automation overhead ≤ 10% of total benchmark time.
- [ ] Reliability: Benchmarks deterministic with ±2% variance across runs.
- [ ] Scalability: Support distributed execution via job matrix (documented, even if not implemented initially).

### Testing Requirements
- [ ] Unit tests for configuration parsing and comparison logic.
- [ ] Integration tests simulate benchmark runs with stubbed harness/reference outputs.
- [ ] Regression tests ensure CI job fails when thresholds exceeded.
- [ ] Coverage ≥ 85% on new scripts.

### Documentation Requirements
- [ ] Benchmark playbook documenting setup, execution, interpretation.
- [ ] Update telemetry viewer docs with comparative plots workflow.
- [ ] Add CI documentation describing new job.
- [ ] Provide FAQ for dataset/algorithm onboarding.

---

## Test Plan

### Unit Tests
```python
def test_threshold_regression_detection():
    baseline = BenchmarkResult(fps=120.0)
    candidate = BenchmarkResult(fps=110.0)
    assert detect_regression(baseline, candidate, threshold=0.05)
```

### Integration Tests
- Run orchestrator against mock engine + reference scripts verifying outputs and exit codes.
- Execute CI smoke configuration locally to ensure determinism and manageable runtime.

### Performance Tests
- Measure orchestration overhead and ensure compliance with ≤10% budget.

---

## Implementation Notes

### Design Considerations
- Use YAML/JSON for scenario definitions to align with harness and sandbox.
- Persist benchmark results with metadata (git commit, hardware, drivers) for reproducibility.
- Provide plugin interface for metric reducers (e.g., PSNR, RMS error) to support cross-domain research.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Benchmark runtime too long for CI | Medium | High | Provide tiered suites (smoke vs. full) and gating thresholds.
| Divergent environments cause variance | Medium | Medium | Capture environment metadata, allow tolerance windows, and run on dedicated machines.
| Telemetry schema changes break comparison | Low | Medium | Version schema and validate compatibility in tests.

### Alternative Approaches
1. **Manual benchmark scripts** → rejected; lacks CI integration and repeatability.
2. **Third-party benchmarking service** → rejected; introduces external dependency and cost.

---

## Deliverables

- [ ] Benchmark orchestrator script + tests
- [ ] Updated telemetry viewer with comparison support
- [ ] CI smoke job + documentation
- [ ] Reports/dashboards for exemplar studies
- [ ] Linked PRs referencing `CC-310`

---

## Definition of Done

- [ ] CI smoke benchmarks green and gating merges
- [ ] Comparative plots available for exemplar workloads
- [ ] Documentation reviewed by research stakeholders
- [ ] Configuration schema versioned and validated
- [ ] Telemetry viewer integrates comparison workflow

---

## Assigned To
**Role**: Performance Engineer (with Tools support)
**Name**: @perf-lead

## Estimated Timeline
**Start Date**: 2025-12-16
**Target Completion**: 2026-01-06
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with infrastructure team for CI resource allocation.
- Provide sample notebooks for exploratory analysis until UI integration completes.
- 2025-12-03: Added `scripts/benchmarks/run_comparative_benchmarks.py` to parse declarative
  benchmark scenarios, execute engine/reference commands, and emit regression summaries,
  covering the orchestrator acceptance criterion.
