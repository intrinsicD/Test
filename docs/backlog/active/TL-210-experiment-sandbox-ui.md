# Backlog Item TL-210 — Experiment Sandbox UI

- **Status**: In Progress
- **Priority**: 3
- **Owner**: Tools Lead
- **Module(s)**: Tools, Runtime, Rendering, Assets
- **Goal**: Deliver an interactive sandbox UI that drives the prototyping harness and benchmarking workflow.

## Summary
Initial sandbox scaffolding landed, but wiring to the runtime harness and benchmark automation remains. TL-210 finalises the ImGui-based sandbox so researchers can browse datasets, tweak rendering presets, trigger comparative benchmarks, and monitor telemetry without editing configuration files.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Synchronise runtime, tools, and benchmarking deliverables; manage UI/performance trade-offs. | Agent Orchestrator (11) |
| Product Manager | Ensure UI scope aligns with kickoff demo goals and roadmap commitments. | Product Manager (10) |
| Knowledge Librarian | Maintain references to sandbox design docs, accessibility checklist, and historical discussions. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Tools lead implements UI; runtime/rendering engineers expose configuration hooks; assets team supplies dataset selectors. | Tools Lead (primary); Runtime & Rendering engineers; Assets engineer |
| Docs/DevRel | Update tools README, prototyping playbook, and screenshot gallery. | Docs/DevRel (95) |
| QA/Test Specialist | Extend sandbox smoke tests and golden screenshot coverage; log outcomes. | QA/Test Specialist (90) |
| Performance Engineer | Profile UI frame budget and telemetry streaming, ensuring ≤1 ms/frame overhead. | Performance Engineer (80) |
| Safety Reviewer | Review UI-triggered benchmark execution for safe file handling and dataset permissions. | Safety Reviewer (15) |
| Reviewer | Validate UI code quality and integration boundaries. | Reviewer (99) |
| Release Manager | Package sandbox builds/screenshots for kickoff review distribution. | Release Manager (98) |

## Definition of Done
- [x] Sandbox enumerates datasets, presets, and algorithm variants from the shared schema and updates runtime state immediately.
- [ ] Benchmark capture from the UI triggers headless runs using CC-310 orchestration and surfaces results inline.
- [x] Telemetry charts stream live metrics while respecting the 1 ms/frame UI budget and persist layout preferences per user.
- [ ] Tools README, prototyping playbook, and accessibility checklist document the workflow with updated screenshots.

## Dependencies
- [`docs/backlog/active/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)
- [`docs/backlog/active/CC-310-comparative-benchmark-automation.md`](CC-310-comparative-benchmark-automation.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/TL-210-experiment-sandbox-ui.md`](../../archive/backlog/legacy/tasks/TL-210-experiment-sandbox-ui.md)
- [`engine/tools/src/sandbox/`](../../modules/tools) implementation scaffolding

## Notes
Golden screenshot coverage is desirable but optional; prioritise deterministic smoke tests first. Coordinate UI telemetry with CC-001 to avoid duplication.
- Sandbox now replays dataset/rendering callbacks whenever configuration or
  preference changes are applied, keeping runtime integrations synchronised
  without requiring manual reselection.
- Algorithm selection callbacks are replayed on registration so runtime profiles
  remain in sync with harness integrations (`engine/tools/tests/test_experiment_sandbox.cpp`).
- Telemetry updates clamp series to 256 samples while recomputing min/max bounds
  to keep the ImGui rendering cost within the 1 ms/frame target
  (`engine/tools/src/sandbox/experiment_sandbox.cpp`).
