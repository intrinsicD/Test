# Backlog Item T-0120 — GPU Resource Provider Completion

- **Status**: Done (2025-03-30)
- **Priority**: 1
- **Owner**: Rendering Lead
- **Module(s)**: Rendering, Assets
- **Goal**: Ship a production-ready GPU resource provider capable of creating buffers, textures, and shader programs for real backend execution.

## Summary
GPU resource providers for OpenGL and Vulkan now allocate, recycle, and track transient resources with retention policies, exposing telemetry and runtime submission hooks. Frame-graph schedulers and the runtime presentation backend consume the live providers, replacing recording fallbacks so demos execute against real GPU memory. Work remained coordinated with [`T-0119`](T_0119_COMMAND_ENCODER_INTEGRATION.md) and PM-510 weekly demos throughout the GPU enablement milestone.

## Current Plan
- Hold joint API/design reviews with T-0119 owners before each implementation increment to ratify resource lifetimes and submission interfaces.
- Time buffer/texture/pipeline bring-up so stage planner teams can exercise GPU-backed presentation paths as soon as RT-410 adapters land.
- Publish backend smoke demo artefacts every week via PM-510, including telemetry snapshots and doc updates for rendering/runtime READMEs.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Track roadmap alignment and unblock cross-module dependencies. | Agent Orchestrator |
| Product Manager | Sequence delivery alongside runtime/presentation milestones. | Product Manager |
| Knowledge Librarian | Keep documentation/ADR cross-references current. | Knowledge Librarian |
| Specialist Engineer(s) | Implement resource creation, caching, and backend wiring. | Rendering Lead |
| Docs/DevRel | Update rendering README, tutorials, and API docs. | Docs Team |
| QA/Test Specialist | Extend GPU backend smoke tests and regression coverage. | QA Lead |
| Performance Engineer | Benchmark resource creation and capture baseline telemetry. | Performance Lead |
| Safety Reviewer | Review shader security and lifecycle guards. | Security Reviewer |
| Reviewer | Provide implementation/code review. | Rendering Reviewer |
| Release Manager | Coordinate feature flagging and rollout. | Release Manager |

## Definition of Done
- [x] Implement GPU buffer, texture, and sampler creation across OpenGL and Vulkan providers with validation parity.
- [x] Integrate shader compilation and pipeline setup into the provider with cache invalidation + hot reload hooks.
- [x] Replace recording-provider fallbacks in schedulers and frame-graph execution with the real provider.
- [x] Add automated tests that exercise resource creation on supported backends and document setup requirements.
- [x] Update rendering module README, root README module status, and roadmap to reflect the new capability.
- [x] Produce weekly integration demo notes and telemetry captures linked from PM-510 until the milestone exits.

## Dependencies
- [`T-0119`](T_0119_COMMAND_ENCODER_INTEGRATION.md) — shares command stream APIs and validation hooks.

## Related Artefacts
- `engine/rendering/src/backend/*` provider implementations.
- `engine/rendering/include/engine/rendering/resource_provider.hpp` (to be extended).
- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](../../specs/ADR_0003_RUNTIME_FRAME_GRAPH.md) — interface contracts for resource acquisition during graph execution.
- [`docs/archive/backlog/legacy/tasks/T_0120_RENDERING_GPU_RESOURCE_PROVIDER_IMPLEMENTATION.md`](../../archive/backlog/legacy/tasks/T_0120_RENDERING_GPU_RESOURCE_PROVIDER_IMPLEMENTATION.md) — historical implementation notes.
- `docs/modules/rendering/README.md` and related tutorials.

## Notes
Ensure shader compilation integrates with the asset hot-reload telemetry so editor workflows observe consistent notifications across CPU and GPU resources.

**2025-02-21** — OpenGL GPU resource provider now exposes a configurable retention window for transient resources with unit coverage so harnesses can experiment with reuse/memory trade-offs while command encoder integration progresses.

**2025-03-30** — OpenGL/Vulkan providers, runtime submission, and presentation backend now execute frame-graph workloads against live GPU resources with telemetry, replacing recording fallbacks and updating roadmap/module status.
