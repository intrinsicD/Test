# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** T-0119 Vulkan Command Encoder Recording Integration
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../docs/ROADMAP.md) · [T-0119](../../docs/backlog/active/T-0119-command-encoder-integration.md)
- **Primary Goal:** Implement a Vulkan command encoder/provider pair that records frame-graph draw and dispatch commands so the Vulkan scheduler can surface submission payloads, advancing the joint GPU enablement milestone tracked in the roadmap.【F:docs/ROADMAP.md†L64-L112】【F:docs/backlog/active/T-0119-command-encoder-integration.md†L1-L37】 
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-02-t-0119-vulkan-command-encoder.md`), context package (`agents/context_packages/2026-03-02-t-0119-vulkan-command-encoder.md`)

## 2. Scope & Boundaries
- In scope: Add Vulkan command buffer recording structures, expose a `VulkanCommandEncoder` + provider, extend the Vulkan GPU scheduler to emit recorded command streams, and cover the path with unit tests mirroring the OpenGL coverage.【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L1-L81】【F:engine/rendering/tests/test_opengl_command_encoder.cpp†L1-L130】
- Out of scope: Real Vulkan device integration, shader/material binding, or presentation backends (handled by T-0120/RT-410).【F:docs/ROADMAP.md†L64-L112】【F:docs/modules/rendering/README.md†L1-L13】
- Architectural considerations / ADRs: Honour the frame-graph contract and runtime/presentation separation defined by ADR-0003 and ADR-0008 when introducing new encoder plumbing.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L119】

## 3. Success Criteria
- Functional: Vulkan scheduler submissions expose recorded draw/dispatch commands sourced from the new encoder, enabling parity with the OpenGL path required by T-0119’s DoD.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L31-L37】【F:engine/rendering/include/engine/rendering/backend/vulkan/gpu_scheduler.hpp†L1-L81】
- Documentation: Update the rendering module README to reflect Vulkan command encoder progress and note remaining backend blockers.【F:docs/modules/rendering/README.md†L1-L13】
- Validation: Execute the canonical configure/build/test/doc-validation workflow after implementation per repository guidance.【F:README.md†L120-L144】
- Quality gates & benchmarks: Maintain existing rendering unit tests and extend them with Vulkan encoder coverage; no performance benchmarks are expected until real GPU execution lands.【F:engine/rendering/tests/test_opengl_command_encoder.cpp†L1-L187】【F:docs/backlog/active/T-0119-command-encoder-integration.md†L31-L37】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Rendering module flagged blocked pending T-0119/T-0120; lists canonical validation commands.【F:README.md†L15-L144】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms backlog/ADR precedence needed when touching rendering docs.【F:docs/NAVIGATION.md†L13-L113】 | Knowledge Librarian |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 priority 1 milestone couples T-0119 with GPU resource/provider deliverables.【F:docs/ROADMAP.md†L64-L112】 | Product Manager |
| 4 | [T-0119 backlog](../../docs/backlog/active/T-0119-command-encoder-integration.md) | DoD demands encoder APIs and scheduler wiring across OpenGL + Vulkan; Vulkan path still missing.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L31-L37】 | Product Manager |
| 5 | [docs/modules/rendering/README.md](../../docs/modules/rendering/README.md) | Module README highlights missing command encoder work as blocker.【F:docs/modules/rendering/README.md†L1-L13】 | Specialist Engineer |
| 6 | [ADR-0003](../../docs/specs/ADR-0003-runtime-frame-graph.md) & [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Frame-graph contract + runtime presentation separation drive encoder design constraints.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L119】 | Chief Architect |
| 7 | [OpenGL encoder/tests](../../engine/rendering/tests/test_opengl_command_encoder.cpp) | Provides reference architecture and expected behaviours to replicate for Vulkan path.【F:engine/rendering/tests/test_opengl_command_encoder.cpp†L1-L187】 | Specialist Engineer |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Coordinate phases, approve exits | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Context package, archive hand-off | Planned |
| Specialist Engineer(s) | Rendering contributor | 3 | Implement Vulkan encoder/provider + tests | Planned |
| Docs/DevRel | Rendering docs reviewer | 2, 4, 5 | README updates, terminology review | Planned |
| QA/Test Specialist | Rendering QA | 4 | Execute build/test/doc validators | Planned |
| Performance Engineer | N/A this increment | 4 | — | N/A |
| Safety Reviewer | Rendering reviewer | 4 | Confirm no unsafe API usage | Planned |
| Reviewer | Rendering maintainer | 4 | Code review of encoder integration | Planned |
| Release Manager | Release coordinator | 5 | Ensure roadmap/backlog updates captured | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed; roadmap/backlog alignment captured. | Task brief + context package drafted. | This brief & context package. |
| 2 – Context Assembly | Module/ADR review complete; implementation risks logged. | Implementation plan recorded in context package. | Context package §§2–6. |
| 3 – Execution & Collaboration | Plan approved by reviewer/agent lead. | Vulkan encoder/provider + tests ready for validation. | Git history, inline docs. |
| 4 – Quality Gates | Build/test/doc commands executed. | Logs attached to quality report; QA sign-off. | Quality report outputs. |
| 5 – Release & Documentation Sync | QA approvals complete. | README/backlog/roadmap updates merged; artefacts archived. | Updated docs + archived context. |

## 7. Timeline & Milestones
- Kickoff: 2026-03-02
- Implementation window: 2026-03-02 (current session)
- Quality gate window: Immediately after implementation
- Release target: 2026-03-02
- Post-release monitoring: Verify Vulkan encoder telemetry during upcoming PM-510 demos once GPU backends run end-to-end.【F:docs/ROADMAP.md†L78-L112】

## 8. Known Risks & Dependencies
- Risks: Vulkan encoder may diverge from OpenGL semantics, risking inconsistent scheduler payloads; missing device bindings could block future integration.【F:docs/modules/rendering/README.md†L1-L13】
- Dependencies: Relies on Vulkan GPU resource provider stubs implemented under T-0120 for command buffer handles.【F:engine/rendering/include/engine/rendering/backend/vulkan/resource_provider.hpp†L1-L120】
- Mitigations / contingency: Mirror OpenGL architecture/tests for parity; keep encoder recording API backend-agnostic to ease later device wiring.【F:engine/rendering/tests/test_opengl_command_encoder.cpp†L1-L187】

## 9. Communication Plan
- Async updates cadence: Post progress in T-0119 notes and PM-510 demo summaries after encoder integration lands.【F:docs/backlog/active/T-0119-command-encoder-integration.md†L9-L37】【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L41】
- Live sync triggers: Escalate to Agent Orchestrator if Vulkan resource/provider contracts need changes or tests uncover design gaps.
- Escalation path: Agent Orchestrator → Rendering Lead per roadmap governance.【F:docs/ROADMAP.md†L64-L112】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-02 | Product Manager | Scoped T-0119 increment to deliver Vulkan command encoder recording parity. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
