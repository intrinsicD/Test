# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-03-02-t-0119-vulkan-command-encoder.md`](../task_briefs/2026-03-02-t-0119-vulkan-command-encoder.md)
- **Backlog Entry:** [`docs/backlog/active/T-0119-command-encoder-integration.md`](../../../backlog/active/T-0119-command-encoder-integration.md)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../../ROADMAP.md)
- **Workflow Phase:** Phase 2 – Context Assembly (next gate: Specialist Engineer approval)

## 2. Problem Summary
- Current behaviour: Vulkan scheduler accepts submissions but lacks command encoder support, so recorded draw/dispatch payloads are absent and parity with OpenGL path is missing, leaving rendering module blocked.【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L1-L81】【F:docs/modules/rendering/README.md†L1-L13】
- Desired behaviour: Provide Vulkan command encoder/provider infrastructure that records frame-graph commands and exposes them through scheduler submissions, fulfilling T-0119 DoD requirements for backend wiring.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L31-L37】
- Constraints / invariants: Must respect frame-graph contract (resource/state metadata) and runtime presentation separation mandated by ADR-0003/ADR-0008; changes should not assume access to real Vulkan devices yet.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L119】
- Quality budgets / telemetry notes: Maintain deterministic command recording similar to OpenGL tests; no new telemetry budget is required until GPU execution integrates with runtime demos tracked via PM-510.【F:engine/rendering/tests/test_opengl_command_encoder.cpp†L1-L187】【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L41】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Flags missing GPU resource/command encoder work as blocker. | 5 |
| ADR / Spec | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../../specs/ADR-0003-runtime-frame-graph.md); [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Define frame-graph submission contract and presentation separation constraints. | 6 |
| Code excerpts | [`engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp`](../../../../engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp); [`engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp`](../../../../engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp); [`engine/rendering/tests/test_opengl_command_encoder.cpp`](../../../../engine/rendering/tests/test_opengl_command_encoder.cpp) | Resource provider currently synthesises handles; scheduler lacks command recording; OpenGL test suite provides reference behaviour. | 7 |
| Telemetry / Benchmarks | [`docs/backlog/active/PM-510-weekly-integration-demos.md`](../../../backlog/active/PM-510-weekly-integration-demos.md) | Weekly demos will surface GPU telemetry once backend executes. | 4 |
| Prior PRs / Discussions | [`docs/archive/backlog/legacy/tasks/T-0119-rendering-command-encoder-implementation.md`](../../../../docs/archive/backlog/legacy/tasks/T-0119-rendering-command-encoder-implementation.md) | Historical breakdown documents Vulkan encoder expectations. | 7 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Confirms rendering module is blocked on T-0119/T-0120 and reiterates validation workflow. | Product Manager | Ensure validation commands executed post-change. |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Establishes precedence chain requiring backlog/ADR citations in docs updates. | Knowledge Librarian | Coordinate README update to reflect new encoder work. |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 priority 1 emphasises completing T-0119 alongside GPU provider work. | Product Manager | Update roadmap notes once Vulkan encoder lands. |
| 4 | [`docs/backlog/active/T-0119-command-encoder-integration.md`](../../../backlog/active/T-0119-command-encoder-integration.md) | DoD explicitly calls for wiring encoder submission paths into Vulkan scheduler. | Specialist Engineer | Mark checklist item complete after implementation. |
| 5 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Highlights missing command encoder as outstanding work; README must mention progress. | Specialist Engineer | Update README upon delivery. |
| 6 | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../../specs/ADR-0003-runtime-frame-graph.md); [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Provide architectural guardrails for frame-graph submission and runtime presentation integration. | Chief Architect | Verify changes stay compliant; escalate deviations. |
| 7 | [`engine/rendering/tests/test_opengl_command_encoder.cpp`](../../../../engine/rendering/tests/test_opengl_command_encoder.cpp); [`engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp`](../../../../engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp) | OpenGL reference informs Vulkan test expectations; scheduler currently lacks command list copying. | Specialist Engineer | Mirror OpenGL testing strategy for Vulkan path. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied: `cmake --preset linux-gcc-debug`, `cmake --build --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`.【F:README.md†L120-L144】
- Additional presets / datasets: None required; Vulkan encoder tests remain headless and operate on synthetic handles.
- Benchmark targets & expected deltas: No GPU benchmarks expected until real device integration; ensure unit test runtime remains comparable to existing OpenGL encoder tests.
- Tooling updates required: Run `python scripts/validate_docs.py` after README/backlog edits; no additional tooling changes identified.【F:README.md†L120-L144】

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Do we need a Vulkan-specific command stream stub similar to OpenGL’s immediate stream? | Rendering Lead | Before implementation sign-off | Pending – evaluate during design; may defer if not required for recording. |
| Should resource provider retain command recordings across frames or reset during `begin_frame()`? | Specialist Engineer | During implementation | Pending – propose per-frame reset mirroring OpenGL buffer semantics. |

## 7. Attachments
- Diagrams: Refer to existing module dependency graph if integration boundaries need visualisation (`docs/architecture/module_dependency_graph.dot`).
- Data sets: Not applicable (encoder tests operate on synthetic handles).
- Additional notes: Historical backlog entry outlines eventual Vulkan device binding steps once GPU execution is enabled.【F:docs/archive/backlog/legacy/tasks/T-0119-rendering-command-encoder-implementation.md†L1-L91】

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
