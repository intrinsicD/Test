# Backlog Item RT-410 — Runtime Stage Planner & Presentation Loop

- **Status**: In Progress
- **Priority**: 1
- **Owner**: Runtime Lead
- **Module(s)**: Runtime, Rendering, Platform
- **Goal**: Implement the stage planner, presentation backends, and synchronisation hooks described in ADR-0008 so the runtime main loop can drive GPU presentation reliably.

## Summary
RuntimeHost::tick now executes through a declarative `RuntimeLoopPlan`, but backend-aware presentation and synchronisation hooks from ADR-0008 are still outstanding. RT-410 tracks the remaining work required to connect the stage planner to rendering/presentation backends, expose scripting hooks, and keep diagnostics/tooling aligned without ad-hoc patches. The task runs in parallel with the GPU enablement milestone so presentation adapters are ready when [`T-0120`](T_0120_GPU_RESOURCE_PROVIDER.md) and [`T-0119`](T_0119_COMMAND_ENCODER_INTEGRATION.md) deliver backend execution.

## Current Plan
- Implement stage planner adapters in lockstep with GPU milestones and surface blockers during PM-510 demos.
- Deliver presentation mock + GLFW backends incrementally, enabling tools to hook into the shared presentation path before TL-310 begins.
- Extend harness coverage early so runtime/tool teams can rehearse integration demos without manual wiring.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Maintain roadmap visibility and escalate blockers. | Agent Orchestrator |
| Product Manager | Prioritise sequencing with T-0119/T-0120 to unlock GPU execution. | Product Manager |
| Knowledge Librarian | Update ADR status and documentation cross-links. | Knowledge Librarian |
| Specialist Engineer(s) | Implement stage planner, presentation adapters, and synchronisation APIs. | Runtime Lead |
| Docs/DevRel | Refresh runtime README, prototyping playbook, and tutorials. | Docs Team |
| QA/Test Specialist | Extend harness/tests for multi-backend presentation coverage. | QA Lead |
| Performance Engineer | Profile loop/presentation latency and capture baselines. | Performance Lead |
| Safety Reviewer | Review threading and synchronisation contracts. | Security Reviewer |
| Reviewer | Conduct code review for loop/presentation changes. | Runtime Reviewer |
| Release Manager | Coordinate rollout, feature flags, and communication. | Release Manager |

## Definition of Done
- [ ] Implement ADR-0008 stage planner with declarative stage graph and scheduling policies.
- [ ] Provide presentation backends for OpenGL and Vulkan that integrate with windowing/runtime configuration.
- [ ] Expose synchronisation hooks for scripting, diagnostics, and tooling integration with documentation and examples.
- [ ] Add automated tests exercising the new loop under mock and GLFW backends plus documentation updates.
- [ ] Update roadmap, runtime README, and root README to reflect the new runtime state.
- [ ] Present progress and telemetry snapshots during PM-510 demos until GPU + runtime milestones exit review.

## Dependencies
- [`T-0119`](T_0119_COMMAND_ENCODER_INTEGRATION.md) — supplies GPU command submission required for presentation.
- [`DC-041`](../archive/DC_041_AI_004_KICKOFF_READINESS.md) — reuse capability selection guidelines.

## Related Artefacts
- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- `engine/runtime/src/runtime_host.cpp` and related scheduler files.
- `engine/platform/` window/presentation adapters.

## Notes
Coordinate with tooling to ensure ImGui overlays reuse the presentation adapters rather than creating duplicate contexts. The stage planner scaffolding landed alongside builder unit tests; presentation adapters and configuration surfaces remain open. The runtime now exposes a compiled `RuntimeLoopPlan` with per-phase telemetry and a `presentation.dispatch` hook—subsequent work must wire real presentation backends and synchronisation APIs to fulfil ADR-0008.

**2026-02-25** — RuntimeHost now surfaces `set_loop_plan()`/`loop_plan()` so tooling and scripting can
swap declarative plans at frame boundaries while keeping diagnostics serialisation in sync.
**2026-03-01** — Exported `engine_runtime_presentation_stage_active()` with Python loader bindings so scripting
surfaces detect presentation handlers without parsing diagnostics output.
