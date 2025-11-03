# Context Package Template

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [2026-03-03-t-0119-vulkan-scheduler-validation](../task_briefs/2026-03-03-t-0119-vulkan-scheduler-validation.md)
- **Backlog Entry:** [T-0119 — Command Encoder Integration](../../docs/backlog/active/T-0119-command-encoder-integration.md)
- **Roadmap Link:** [docs/ROADMAP.md §Phase 4](../../docs/ROADMAP.md)
- **Workflow Phase:** Phase 2 – Context Assembly complete; hand-off to Specialist Engineer for execution.

## 2. Problem Summary
- Current behaviour: Vulkan scheduler queue selection only matches capitalised substrings and submissions are untested despite piping command encoder output into submission structures, leaving regressions undetected for non-standard pass naming and synchronisation metadata.【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L53-L121】
- Desired behaviour: Case-insensitive heuristics steer compute/transfer passes to the right queues while unit tests assert that submissions expose recorded commands, barriers, waits, signals, and fence handles per T-0119’s definition of done.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L31-L37】【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L29-L121】
- Constraints / invariants: Must respect frame-graph contract and runtime presentation interfaces laid out in ADR-0003 and ADR-0008; scheduler changes cannot violate resource provider semantics shared with T-0120.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L119】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L31-L37】
- Quality budgets / telemetry notes: Continue running the canonical configure/build/test/doc validation commands; performance telemetry is deferred until real GPU execution lands per roadmap and module status.【F:README.md†L120-L144】【F:docs/modules/rendering/README.md†L1-L13】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [docs/modules/rendering/README.md](../../docs/modules/rendering/README.md) | Flags command encoder/provider work as blockers. | 5 |
| ADR / Spec | [docs/specs/ADR-0003-runtime-frame-graph.md](../../docs/specs/ADR-0003-runtime-frame-graph.md); [docs/specs/ADR-0008-runtime-main-loop-and-tooling.md](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Define scheduler/resource and runtime submission contracts. | 6 |
| Code excerpts | [engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp](../../engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp) | Current queue heuristics and submission wiring. | 3/4 |
| Code excerpts | [engine/rendering/include/engine/rendering/backend/opengl/gpu_scheduler.hpp](../../engine/rendering/include/engine/rendering/backend/opengl/gpu_scheduler.hpp) | Reference implementation with richer tests. | 3/4 |
| Code excerpts | [engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp](../../engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp) | Provides command buffer ownership and sync handles relied on by scheduler tests. | 4 |
| Design note | [docs/design/RESOURCE_MANAGEMENT.md](../../docs/design/RESOURCE_MANAGEMENT.md) | Captures handle lifetime model reused by GPU providers. | 7 |
| Historical context | [docs/modules/rendering/PROGRESS_2025-10-27.md](../../docs/modules/rendering/PROGRESS_2025-10-27.md) | Documents GPU enablement priorities and scheduler expectations. | 7 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Rendering remains blocked on T-0119/T-0120; canonical validation block recorded. | Product Manager | Ensure commands executed post-change. |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms precedence chain (AGENTS → NAVIGATION → ADRs → module docs). | Knowledge Librarian | None. |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 prioritises T-0119 alongside resource provider/runtime work; risks highlight GPU slip. | Product Manager | Update roadmap if scheduler milestone closes. |
| 4 | [T-0119 backlog](../../docs/backlog/active/T-0119-command-encoder-integration.md) | DoD calls for scheduler wiring and integration tests; telemetry expected. | Product Manager | Add test results to backlog notes after completion. |
| 5 | [Rendering README](../../docs/modules/rendering/README.md) | Module outstanding work lists command encoder/provider completion. | Specialist Engineer | No README change planned until broader milestone closes. |
| 6 | [ADR-0003](../../docs/specs/ADR-0003-runtime-frame-graph.md) & [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Reinforce need for deterministic submission metadata compatible with runtime presentation planners. | Chief Architect | Raise ADR update if scheduler semantics change materially. |
| 7 | [Rendering progress update](../../docs/modules/rendering/PROGRESS_2025-10-27.md) | Historic milestone stresses finishing T-0120/T-0119 for real rendering; scheduler readiness tied to these tasks. | Knowledge Librarian | Archive new context artefacts post-delivery. |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None required; tests remain CPU-only.
- Benchmark targets & expected deltas: Not applicable until GPU execution is functional.
- Tooling updates required: None; existing CI scripts already collect rendering test results.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Real Vulkan device execution still unavailable; is simulated command recording sufficient for upcoming demos? | Rendering Lead | GPU enablement review (2026-03-22) | Pending – depends on T-0120 follow-up. |

## 7. Attachments
- Diagrams: None.
- Data sets: None.
- Additional notes: Maintain parity with OpenGL scheduler tests when expanding Vulkan coverage to prevent regressions during real GPU bring-up.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
