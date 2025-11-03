# Backlog Item RT-321 — Prototyping Case Study Validation

- **Status**: Complete
- **Priority**: 3
- **Owner**: Runtime Lead (with Assets & Tools support)
- **Module(s)**: Runtime, Assets, Rendering, Tools, Python
- **Goal**: Demonstrate the harness across two reproducible case studies that capture telemetry and benchmark artefacts.

## Summary
After RT-320 delivers the harness we must prove it supports real workflows. RT-321 curates two case studies—one geometry-heavy, one rendering-heavy—that run end to end through the AI-004 configuration schema. Each scenario loads packaged datasets, configures the research baseline, exports telemetry compatible with CC-310, and is callable from both the CLI and the sandbox UI.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Sequence dependencies (RT-320, AS-330, TL-210) and coordinate validation timelines. | Agent Orchestrator (11) |
| Product Manager | Align case study acceptance criteria with roadmap success metrics. | Product Manager (10) |
| Knowledge Librarian | Archive case study documentation, telemetry baselines, and dataset provenance references. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Runtime lead develops presets; assets and tools engineers wire datasets/UI toggles. | Runtime Lead; Assets Lead; Tools Lead |
| Docs/DevRel | Document case study workflows in prototyping playbook and module READMEs. | Docs/DevRel (95) |
| QA/Test Specialist | Add integration tests covering CLI and sandbox execution; record baseline telemetry artefacts. | QA/Test Specialist (90) |
| Performance Engineer | Evaluate comparative metrics for each case study and feed data into CC-310/CC-311. | Performance Engineer (80) |
| Safety Reviewer | Review dataset licensing and telemetry handling before publication. | Safety Reviewer (15) |
| Reviewer | Validate case study implementation and documentation updates. | Reviewer (99) |
| Release Manager | Publish packaged case study presets and ensure availability for kickoff review. | Release Manager (98) |

## Definition of Done
- [x] Case study manifests live under `assets/datasets/` and are referenced by harness CLI/SDK helpers.
- [x] Harness CLI exposes `--case-study` presets that configure datasets, algorithms, and telemetry sinks.
- [x] Sandbox UI enumerates case studies with default parameter sets and benchmark capture toggles.
- [x] Integration + CI coverage executes both scenarios and records baseline metrics in documentation (`docs/design/RT-321-case-studies.md`).

## Dependencies
- [`docs/backlog/archive/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)
- [`docs/backlog/archive/AS-330-reference-dataset-packages.md`](AS-330-reference-dataset-packages.md)
- [`docs/backlog/archive/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/RT-321-prototyping-case-study-validation.md`](../../archive/backlog/legacy/tasks/RT-321-prototyping-case-study-validation.md)
- To document baselines: extend [`docs/design/RT-320-prototyping-harness.md`](../../design/RT-320-prototyping-harness.md) or author `RT-321-case-studies.md`.

## Notes
Case study telemetry feeds CC-310/CC-311. Coordinate dataset licensing early to avoid blocking integration tests.

**2026-02-16** — Added rendering case study dry-run coverage to CTest
(`runtime_prototype_harness_rendering_case_study`) and published
[`docs/design/RT-321-case-studies.md`](../../design/RT-321-case-studies.md)
capturing baseline dataset metrics, rendering defaults, and telemetry targets
for both packaged scenarios. Runtime and tools READMEs now cross-link the
document so UI selectors, harness CLI, and automation share the same source of
truth.
**2026-02-18** — Roadmap tables and README next steps now reflect case study completion and outline maintenance work for future scenario additions.
