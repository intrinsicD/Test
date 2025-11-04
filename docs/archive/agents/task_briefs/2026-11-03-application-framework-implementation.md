# Task Brief: Application Framework Implementation

> Owner: Product Manager (Agent Orchestrator)  
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)  
> Date: November 3, 2025

## 1. Summary
- **Title:** Implement Application Framework and Platform Input Integration
- **Roadmap / Backlog Reference:** Phase 4 — GPU Execution & Tooling Readiness (Priority 1-2)
- **Primary Goal:** Implement the application framework proposed in APPLICATION_FRAMEWORK_PROPOSAL.md to reduce boilerplate and provide consistent patterns for engine applications, starting with Platform Input Integration.
- **Linked Workflow Artefacts:** 
  - Task brief: `agents/task_briefs/2026-11-03-application-framework-implementation.md`
  - Context package: `agents/context_packages/2026-11-03-application-framework-implementation.md`
  - Quality report: TBD

## 2. Scope & Boundaries
**In scope:**
- Phase 1: Platform Input Integration
  - Add `Window::input_state()` accessor to Window interface
  - Implement `InputState` integration in GLFW backend
  - Wire GLFW callbacks to update InputState automatically
  - Update platform module tests
- Phase 2: Basic Application Class (Deferred to follow-up)
  - Core Application base class design
  - Lifecycle callback integration
- Phase 3: RT-410 Integration (Deferred, blocked by RT-410)
  - Presentation backend wiring
  - Automatic swapchain management

**Out of scope:**
- GPU resource provider (T-0120) and command encoder (T-0119) - separate tasks
- Full presentation backend implementation - waiting on RT-410
- Multi-window applications
- Dear ImGui integration in Application class

**Architectural considerations / ADRs:**
- [`ADR-0008`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) - Runtime main loop and tooling integration
- [`APPLICATION_FRAMEWORK_PROPOSAL.md`](../../../reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) - Design proposal for Application class

## 3. Success Criteria
**Functional:**
- Window interface exposes `input_state()` method returning `InputState&`
- GLFW backend automatically updates InputState during `pump_events()`
- Input state tracks keyboard, mouse button, cursor position, and scroll events
- Geometry viewer can use `window->input_state()` instead of raw GLFW callbacks

**Documentation:**
- Platform module README updated with input integration examples
- Module API documentation for Window::input_state()
- Migration guide for existing GLFW callback code

**Validation:**
- Platform module tests pass
- CTest suite passes
- Geometry viewer refactored to use new API (proof of concept)

**Quality gates & benchmarks:**
- No performance regression in input handling
- No memory leaks in InputState management
- All existing tests remain green

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Platform module marked ✅ Stable, needs input integration | ✅ |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Documentation precedence chain confirmed | ✅ |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 GPU Enablement, RT-410 in progress | ✅ |
| 4 | N/A - no specific backlog entry | Creating new capability, not backlog-driven | N/A |
| 5 | [`docs/modules/platform/README.md`](../../../modules/platform/README.md) | Window backend selection documented, input examples partial | ✅ |
| 6 | [`docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md`](../../../reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) | Complete design proposal for application framework | ✅ |
| 7 | [`docs/reviews/MISSING_COMPONENTS_SUMMARY.md`](../../../reviews/MISSING_COMPONENTS_SUMMARY.md) | Gap analysis identifies Window::input_state() as missing | ✅ |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | GitHub Copilot | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | GitHub Copilot | 2 & 5 | Context package, archive hand-off | Active |
| Specialist Engineer(s) | GitHub Copilot | 3 | Implementation, test updates | Active |
| Docs/DevRel | GitHub Copilot | 2, 4, 5 | Documentation updates, terminology review | Active |
| QA/Test Specialist | GitHub Copilot | 4 | Validation suite | Active |
| Performance Engineer | N/A | 4 | Benchmarks (minimal for this change) | N/A |
| Safety Reviewer | GitHub Copilot | 4 | Security & safety (minimal concerns) | Active |
| Reviewer | GitHub Copilot | 4 | Code review | Active |
| Release Manager | GitHub Copilot | 5 | Release prep | Active |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Design proposal reviewed | Task brief approved, scope clear | This document |
| 2 – Context Assembly | Task brief approved | Context package complete, no blockers | Context package document |
| 3 – Execution & Collaboration | Context package reviewed | Code implemented, tests updated | Git commits, code files |
| 4 – Quality Gates | Implementation complete | All tests pass, docs updated | CTest output, quality report |
| 5 – Release & Documentation Sync | Quality gates passed | Documentation synced, roadmap updated | Updated READMEs, ROADMAP.md |

## 7. Timeline & Milestones
- Kickoff: November 3, 2025
- Implementation window: November 3, 2025 (same day)
- Quality gate window: November 3, 2025
- Release target: November 3, 2025 (incremental)
- Post-release monitoring: N/A (local development)

## 8. Known Risks & Dependencies
**Risks:**
- GLFW backend refactoring might break existing examples temporarily
- InputState API design might need iteration based on usage

**Dependencies:**
- None - this is foundational work that enables future tasks
- Enables easier implementation of Application class (Phase 2)
- Simplifies RT-410 integration (Phase 3)

**Mitigations / contingency:**
- Keep changes minimal and focused on input integration
- Maintain backward compatibility with existing event queue
- Test thoroughly with geometry viewer as proof of concept

## 9. Communication Plan
- Async updates cadence: After each implementation phase
- Live sync triggers: N/A (solo agent work)
- Escalation path: Document blockers in decision log

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2025-11-03 | GitHub Copilot | Task brief created, Phase 1 scoped | Proceeding with Platform Input Integration |
| 2025-11-03 | GitHub Copilot | Context package next | Starting context assembly |
| 2025-11-03 | GitHub Copilot | Phase 1 implementation complete | Window::input_state() integrated successfully |
| 2025-11-03 | GitHub Copilot | Geometry viewer refactored | Removed ~120 lines of GLFW callbacks |
| 2025-11-03 | GitHub Copilot | Documentation updated | Platform README rewritten with new API examples |
| 2025-11-03 | GitHub Copilot | Quality report created | Phase 1 APPROVED, all gates passed |

**Status:** ✅ PHASE 1 COMPLETE

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../../../../agents/TEMPLATES) to keep audit trails coherent.

