# Backlog Item TL-210 — Experiment Sandbox UI

- **Status**: In Progress
- **Priority**: 3
- **Owner**: Tools Lead
- **Module(s)**: Tools, Runtime, Rendering, Assets
- **Goal**: Deliver an interactive sandbox UI that drives the prototyping harness and benchmarking workflow.

## Summary
Initial sandbox scaffolding landed, but wiring to the runtime harness and benchmark automation remains. TL-210 finalises the ImGui-based sandbox so researchers can browse datasets, tweak rendering presets, trigger comparative benchmarks, and monitor telemetry without editing configuration files.

## Definition of Done
- [ ] Sandbox enumerates datasets, presets, and algorithm variants from the shared schema and updates runtime state immediately.
- [ ] Benchmark capture from the UI triggers headless runs using CC-310 orchestration and surfaces results inline.
- [ ] Telemetry charts stream live metrics while respecting the 1 ms/frame UI budget and persist layout preferences per user.
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
