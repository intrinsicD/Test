# Backlog Item T-0119 — Command Encoder Integration

- **Status**: In Progress
- **Priority**: 1
- **Owner**: Rendering Lead
- **Module(s)**: Rendering, Runtime
- **Goal**: Deliver a command encoder that records GPU work for OpenGL and Vulkan backends, bridging frame-graph compilation with backend submission.

## Summary
Without a functional command encoder, frame-graph execution stops at validation layers and never emits backend draw calls. T-0119 revives the command encoder work so passes can translate graph operations into backend command buffers, submit them through scheduler queues, and synchronise with runtime presentation. Work is now coordinated with [`T-0120`](T_0120_GPU_RESOURCE_PROVIDER.md) under the GPU enablement milestone, sharing design reviews, smoke demos, and weekly status captures in [`PM-510`](PM_510_WEEKLY_INTEGRATION_DEMOS.md).

## Current Plan
- Finalise encoder/resource handshake contracts during the joint milestone reviews before implementation checkpoints.
- Land backend submission paths in step with T-0120 so runtime teams can validate presentation hooks without blocked bindings.
- Surface telemetry snapshots and regression scripts through PM-510 to keep runtime/tools teams aligned on encoder progress.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Maintain alignment between rendering and runtime plans. | Agent Orchestrator |
| Product Manager | Sequence delivery with T-0120 and presentation tasks. | Product Manager |
| Knowledge Librarian | Document encoder architecture decisions and API references. | Knowledge Librarian |
| Specialist Engineer(s) | Implement encoder recording, validation, and submission. | Rendering Lead |
| Docs/DevRel | Update tutorials and API references for the encoder surface. | Docs Team |
| QA/Test Specialist | Create backend smoke tests verifying draw submission. | QA Lead |
| Performance Engineer | Capture baseline GPU execution metrics. | Performance Lead |
| Safety Reviewer | Review synchronization and resource ownership invariants. | Security Reviewer |
| Reviewer | Provide code review and integration validation. | Rendering Reviewer |
| Release Manager | Coordinate feature toggles and rollout sequencing. | Release Manager |

## Definition of Done
- [ ] Implement encoder APIs for draw, compute, and resource barriers aligned with frame-graph operations.
- [ ] Wire encoder submission paths into OpenGL and Vulkan schedulers with error reporting and telemetry.
- [ ] Provide integration tests that render simple geometry via both backends under CI-friendly toggles.
- [ ] Document encoder usage in rendering and runtime READMEs, including error handling patterns.
- [ ] Update roadmap, backlog summaries, and module TODOs to reflect completion.
- [ ] Supply weekly updates/demo artefacts through PM-510 until GPU milestone exit review closes.

## Dependencies
- [`T-0120`](T_0120_GPU_RESOURCE_PROVIDER.md) — requires GPU objects to bind.
- [`ADR-0008`](../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) — informs runtime submission timing.

## Related Artefacts
- `engine/rendering/include/engine/rendering/command_encoder.hpp` (pending implementation).
- `engine/rendering/src/backend/vulkan/` and `.../opengl/` scheduler paths.
- `engine/runtime/src/runtime_host.cpp` submission hooks.
- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md) — frame-graph to backend interface contract.

## Notes
Ensure encoder telemetry integrates with existing diagnostics counters so runtime tooling can attribute GPU time per pass across backends.
