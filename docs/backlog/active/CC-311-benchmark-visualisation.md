# Backlog Item CC-311 — Benchmark Visualisation Integration

- **Status**: Planned
- **Priority**: 4
- **Owner**: Performance Lead (with Tools support)
- **Module(s)**: Scripts, Tools, Python, Docs
- **Goal**: Surface comparative benchmark outputs in telemetry tooling and CI to prove AI-004 readiness.

## Summary
Once CC-310 produces comparative artefacts they must be consumable by humans and automation. CC-311 extends the telemetry viewer with a comparative mode, publishes HTML/PNG reports, and wires smoke coverage into CI. The sandbox UI links to these reports so researchers can inspect regressions directly inside the workflow.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Coordinate outputs from CC-310 and RT-321 to feed visualisation milestones. | Agent Orchestrator (11) |
| Product Manager | Validate deliverables against roadmap reporting expectations. | Product Manager (10) |
| Knowledge Librarian | Catalogue benchmark artefacts, viewer docs, and historical comparisons for traceability. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Performance/tooling engineers extend telemetry viewer and CI pipelines; runtime/tools teams supply integration hooks. | Performance Lead; Tools engineer; Runtime integration engineer |
| Docs/DevRel | Document viewer workflows, sandbox linkages, and CI usage. | Docs/DevRel (95) |
| QA/Test Specialist | Add CI smoke coverage for comparative mode and visual regression tests. | QA/Test Specialist (90) |
| Performance Engineer | Analyse comparative visuals for regressions, tune thresholds. | Performance Engineer (80) |
| Safety Reviewer | Confirm report publication complies with data handling policies. | Safety Reviewer (15) |
| Reviewer | Review visualisation code and documentation updates. | Reviewer (99) |
| Release Manager | Publish generated reports and ensure they are packaged with release artefacts. | Release Manager (98) |

## Definition of Done
- [ ] Telemetry viewer offers a `compare` (or equivalent) subcommand that renders plots from CC-310 outputs.
- [ ] Generated reports for both RT-321 case studies stored under `assets/benchmarks/` and linked from AI-004 docs.
- [ ] CI job executes a reduced comparative suite and enforces ≤2% regression thresholds.
- [ ] Benchmark playbook and telemetry documentation describe usage, troubleshooting, and CI integration.

## Dependencies
- [`docs/backlog/active/CC-310-comparative-benchmark-automation.md`](CC-310-comparative-benchmark-automation.md)
- [`docs/backlog/active/RT-321-prototyping-case-studies.md`](RT-321-prototyping-case-studies.md)
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/CC-311-benchmark-visualization-integration.md`](../../archive/backlog/legacy/tasks/CC-311-benchmark-visualization-integration.md)
- [`scripts/diagnostics/telemetry_viewer.py`](../../../scripts/diagnostics/telemetry_viewer.py)
- `docs/design/CC-311-visualisation-guide.md` *(author during implementation)*

## Notes
Coordinate with tools to expose comparative artefacts inside the sandbox telemetry panel. Track CI runtime budgets to keep added latency under five minutes.
