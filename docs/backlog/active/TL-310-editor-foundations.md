# Backlog Item TL-310 — Editor Foundations & Tooling Enablement

- **Status**: Sequenced
- **Priority**: 2
- **Owner**: Tools Lead
- **Module(s)**: Tools, Runtime, Rendering
- **Goal**: Enable the editor/tooling stack described in ADR-0008 by re-enabling the tools module in the build, wiring panel registries, and integrating with the runtime harness.

## Summary
The tools module is currently disabled in builds despite documentation labelling it "Modularization Complete." Editor panels, panel registries, and sandbox integration from ADR-0008 remain aspirational. TL-310 restores the work so editor builds compile by default, panels register declaratively, and tooling can consume runtime diagnostics. Work is sequenced immediately after [`RT-410`](RT-410-runtime-stage-planner.md) lands presentation adapters so tooling can reuse the shared GPU/runtime plumbing highlighted in [`PM-510`](PM-510-weekly-integration-demos.md).

## Current Plan
- Prototype editor bootstrap against the mock presentation adapter while RT-410 completes so activation is fast once hooks merge.
- Prepare panel registry documentation and samples in parallel with GPU demos to minimise lag between runtime + tooling readiness.
- Validate feature flags and CI coverage within the weekly PM-510 cadence before re-enabling builds by default.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Keep cross-module stakeholders aligned on tooling deliverables. | Agent Orchestrator |
| Product Manager | Prioritise editor readiness alongside runtime/presentation milestones. | Product Manager |
| Knowledge Librarian | Document panel registry design and editor workflows. | Knowledge Librarian |
| Specialist Engineer(s) | Re-enable module builds, implement registries, and integrate harness hooks. | Tools Lead |
| Docs/DevRel | Update tools README, tutorials, and sandbox documentation. | Docs Team |
| QA/Test Specialist | Restore editor smoke tests and regression coverage. | QA Lead |
| Performance Engineer | Profile editor frame times under ImGui overlays. | Performance Lead |
| Safety Reviewer | Review plugin loading and scripting integrations. | Security Reviewer |
| Reviewer | Provide code review for tooling changes. | Tools Reviewer |
| Release Manager | Coordinate release notes and feature flags. | Release Manager |

## Definition of Done
- [ ] Re-enable tools module targets in CMake presets and document any build flags.
- [ ] Implement the panel registry, harness bridge, and ImGui reuse strategy defined in ADR-0008.
- [ ] Provide sample editor configuration exercising runtime diagnostics and rendering overlays.
- [ ] Restore editor CI coverage or smoke tests within scripts/tests.
- [ ] Update tools README, root README, and roadmap to reflect the revived tooling path.
- [ ] Demo editor integration in PM-510 once runtime presentation hooks and GPU milestones validate the shared adapters.

## Dependencies
- [`ADR-0008`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md) — shared panel registry and loop integration guidance.
- [`RT-410`](RT-410-runtime-stage-planner.md) — supplies synchronisation hooks.

## Related Artefacts
- `engine/tools/` module sources and tests.
- `docs/modules/tools/README.md` and related tutorials.
- `scripts/prototyping/run_prototype_harness.py` integration points.

## Notes
Coordinate with rendering to share GPU contexts and avoid duplicate swap chains when embedding ImGui inside presentation backends.
