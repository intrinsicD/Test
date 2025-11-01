# Backlog Item T-0120 — GPU Resource Provider Completion

- **Status**: Planned
- **Priority**: 1
- **Owner**: Rendering Lead
- **Module(s)**: Rendering, Assets
- **Goal**: Ship a production-ready GPU resource provider capable of creating buffers, textures, and shader programs for real backend execution.

## Summary
The prototype renderer still routes all resource activity through the recording provider, preventing the OpenGL and Vulkan backends from allocating GPU buffers, uploading textures, or compiling shader programs. T-0120 reinstates the dedicated GPU resource provider so schedulers can transition from stubbed handles to real GPU objects and unlock end-to-end rendering.

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
- [ ] Implement GPU buffer, texture, and sampler creation across OpenGL and Vulkan providers with validation parity.
- [ ] Integrate shader compilation and pipeline setup into the provider with cache invalidation + hot reload hooks.
- [ ] Replace recording-provider fallbacks in schedulers and frame-graph execution with the real provider.
- [ ] Add automated tests that exercise resource creation on supported backends and document setup requirements.
- [ ] Update rendering module README, root README module status, and roadmap to reflect the new capability.

## Dependencies
- [`T-0119`](T-0119-command-encoder-integration.md) — shares command stream APIs and validation hooks.

## Related Artefacts
- `engine/rendering/src/backend/*` provider implementations.
- `engine/rendering/include/engine/rendering/resource_provider.hpp` (to be extended).
- [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../specs/ADR-0003-runtime-frame-graph.md) — interface contracts for resource acquisition during graph execution.
- [`docs/archive/backlog/legacy/tasks/T-0120-rendering-gpu-resource-provider-implementation.md`](../../archive/backlog/legacy/tasks/T-0120-rendering-gpu-resource-provider-implementation.md) — historical implementation notes.
- `docs/modules/rendering/README.md` and related tutorials.

## Notes
Ensure shader compilation integrates with the asset hot-reload telemetry so editor workflows observe consistent notifications across CPU and GPU resources.
