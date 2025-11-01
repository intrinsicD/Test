# Backlog Item RT-410 — Runtime Stage Planner & Presentation Loop

- **Status**: Planned
- **Priority**: 1
- **Owner**: Runtime Lead
- **Module(s)**: Runtime, Rendering, Platform
- **Goal**: Implement the stage planner, presentation backends, and synchronisation hooks described in ADR-0008 so the runtime main loop can drive GPU presentation reliably.

## Summary
RuntimeHost::tick currently advances subsystems sequentially without declarative stage planning, backend-aware presentation, or synchronisation hooks for tooling. ADR-0008 outlines the target architecture, but the work never landed and the backlog stopped tracking it. RT-410 reinstates the task so the runtime can orchestrate rendering submission, presentation fences, scripting callbacks, and diagnostics without ad-hoc patches.

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

## Dependencies
- [`T-0119`](T-0119-command-encoder-integration.md) — supplies GPU command submission required for presentation.
- [`DC-041`](DC-041-ai-004-kickoff-readiness.md) — reuse capability selection guidelines.

## Related Artefacts
- [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../specs/ADR-0008-runtime-main-loop-and-tooling.md)
- `engine/runtime/src/runtime_host.cpp` and related scheduler files.
- `engine/platform/` window/presentation adapters.

## Notes
Coordinate with tooling to ensure ImGui overlays reuse the presentation adapters rather than creating duplicate contexts.
