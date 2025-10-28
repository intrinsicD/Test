# Backlog Item CC-310 — Comparative Benchmark Automation

- **Status**: Planned
- **Priority**: 4
- **Owner**: Performance Lead
- **Module(s)**: Scripts, Python Tooling, Runtime, Rendering, Tools
- **Goal**: Automate comparative benchmarks between the engine and reference implementations with CI enforcement.

## Summary
Telemetry capture exists but researchers still run comparisons manually. CC-310 introduces a benchmarking orchestrator that schedules AI-004 scenarios, runs engine + reference workloads, exports structured results, and feeds CI gates plus telemetry viewer dashboards. This automation ensures regressions are caught before the kickoff demo and provides reproducible artefacts for case studies.

## Definition of Done
- [ ] Benchmark orchestrator executes configuration-driven scenarios that run both engine and reference binaries.
- [ ] Result reduction produces JSON/CSV outputs and comparative plots consumable by the telemetry viewer and sandbox UI.
- [ ] CI smoke job runs a reduced suite and fails when thresholds (>2% regressions) are exceeded.
- [ ] Benchmark playbook documents setup, execution, and integration steps for new algorithms.

## Dependencies
- [`docs/backlog/active/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)
- [`docs/backlog/active/AS-330-reference-dataset-packages.md`](AS-330-reference-dataset-packages.md)
- [`docs/backlog/active/TL-210-experiment-sandbox-ui.md`](TL-210-experiment-sandbox-ui.md)
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/CC-310-comparative-benchmark-automation.md`](../../archive/backlog/legacy/tasks/CC-310-comparative-benchmark-automation.md)
- [`scripts/benchmarks/run_comparative_benchmarks.py`](../../../scripts/benchmarks/run_comparative_benchmarks.py)
- `docs/design/CC-310-benchmark-playbook.md` *(author during implementation)*

## Notes
When adding CI coverage, keep runtime under five minutes by creating smoke presets. Align metrics with telemetry schema (`CC-001`).
