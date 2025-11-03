# Backlog Item CC-310 — Comparative Benchmark Automation

- **Status**: Complete
- **Priority**: 4
- **Owner**: Performance Lead
- **Module(s)**: Scripts, Python Tooling, Runtime, Rendering, Tools
- **Goal**: Automate comparative benchmarks between the engine and reference implementations with CI enforcement.

## Summary
Telemetry capture exists but researchers still run comparisons manually. CC-310 introduces a benchmarking orchestrator that schedules AI-004 scenarios, runs engine + reference workloads, exports structured results, and feeds CI gates plus telemetry viewer dashboards. This automation ensures regressions are caught before the kickoff demo and provides reproducible artefacts for case studies.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Coordinate harness, dataset, and sandbox inputs; align CI runtime budgets. | Agent Orchestrator (11) |
| Product Manager | Confirm benchmarks cover roadmap success metrics and stakeholder needs. | Product Manager (10) |
| Knowledge Librarian | Track benchmark scripts, telemetry schemas, and historical results for traceability. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Performance lead owns automation; runtime/tools engineers expose hooks; scripting team implements orchestration. | Performance Lead (primary); Runtime & Tools engineers; Scripts maintainer |
| Docs/DevRel | Update benchmark playbook, telemetry docs, and roadmap references. | Docs/DevRel (95) |
| QA/Test Specialist | Integrate CI smoke suite, capture logs, and manage regression artefacts. | QA/Test Specialist (90) |
| Performance Engineer | Analyse comparative results, set thresholds, and oversee telemetry viewer integration. | Performance Engineer (80) |
| Safety Reviewer | Review benchmark scripts for dependency/security risks before CI integration. | Safety Reviewer (15) |
| Reviewer | Inspect orchestration code and documentation for compliance with CONTRIBUTION.md. | Reviewer (99) |
| Release Manager | Publish benchmark presets and ensure CI artifacts are retained for releases. | Release Manager (98) |

## Definition of Done
- [x] Benchmark orchestrator executes configuration-driven scenarios that run both engine and reference binaries.
- [x] Result reduction produces JSON/CSV outputs and comparative plots consumable by the telemetry viewer and sandbox UI.
- [x] CI smoke job runs a reduced suite and fails when thresholds (>2% regressions) are exceeded.
- [x] Benchmark playbook documents setup, execution, and integration steps for new algorithms.

## Dependencies
- [`docs/backlog/archive/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)
- [`docs/backlog/archive/AS-330-reference-dataset-packages.md`](AS-330-reference-dataset-packages.md)
- [`docs/backlog/archive/TL-210-experiment-sandbox-ui.md`](TL-210-experiment-sandbox-ui.md)
- [`docs/backlog/archive/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/CC-310-comparative-benchmark-automation.md`](../../archive/backlog/legacy/tasks/CC-310-comparative-benchmark-automation.md)
- [`scripts/benchmarks/run_comparative_benchmarks.py`](../../../scripts/benchmarks/run_comparative_benchmarks.py)
- `docs/design/CC-310-benchmark-playbook.md` *(author during implementation)*

## Notes
When adding CI coverage, keep runtime under five minutes by creating smoke presets. Align metrics with telemetry schema (`CC-001`).

**2026-02-20** — Landed SVG plot generation, AI-004 comparative fixtures under
`assets/benchmarks/ai004/`, CI smoke helper, and the CC-310 benchmark playbook.
Telemetry viewer comparative mode now renders reports from the generated
summaries, enabling TL-210 to surface artefacts directly in the sandbox.
