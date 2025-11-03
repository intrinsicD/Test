# Task Brief: Application Framework Phase 2 - Application Base Class

> Owner: Product Manager (Agent Orchestrator)  
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)  
> Date: November 4, 2025

## 1. Summary
- **Title:** Implement Application Base Class for Runtime Framework
- **Roadmap / Backlog Reference:** Phase 4 — GPU Execution & Tooling Readiness (Priority 2)
- **Primary Goal:** Implement Phase 2 of the Application Framework proposal: create `runtime::Application` base class that provides high-level application lifecycle, automatic window/runtime wiring, and built-in main loop.
- **Linked Workflow Artefacts:** 
  - Task brief: `agents/task_briefs/2026-11-04-application-framework-phase2.md`
  - Context package: `agents/context_packages/2026-11-04-application-framework-phase2.md`
  - Quality report: TBD
- **Dependencies:** Phase 1 (Platform Input Integration) - ✅ COMPLETE

## 2. Scope & Boundaries
**In scope:**
- Create `runtime::Application` base class with lifecycle callbacks
- Implement automatic window creation from ApplicationConfig
- Implement built-in main loop with frame timing
- Provide subsystem accessors (window, runtime, input, scene)
- Implement `run()` method that handles full application lifecycle
- Support `quit()` method for graceful shutdown
- Refactor geometry viewer to use Application base class as proof of concept
- Update runtime module documentation

**Out of scope:**
- Presentation backend integration (RT-410 not complete yet)
- GPU rendering execution (T-0119, T-0120 blocked)
- Multi-window applications
- Advanced features (VSync control, fullscreen, etc.)
- ImGui integration in Application class (deferred to future phase)

**Architectural considerations / ADRs:**
- [`ADR-0008`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) - Runtime main loop and tooling integration
- [`APPLICATION_FRAMEWORK_PROPOSAL.md`](../../docs/reviews/APPLICATION_FRAMEWORK_PROPOSAL.md) - Design proposal for Application class
- Phase 1 implementation establishes Window::input_state() pattern

## 3. Success Criteria
**Functional:**
- `Application` base class compiles and links successfully
- Geometry viewer can inherit from Application and implement lifecycle callbacks
- Main loop runs at stable framerate with accurate frame timing
- `quit()` method triggers graceful shutdown
- Window, input, runtime, and scene accessible via protected methods
- Application config supports window configuration passthrough

**Documentation:**
- Runtime module README updated with Application class usage
- Complete lifecycle callback documentation
- Migration guide from manual main loops to Application class
- Examples showing minimal and complete application structure

**Validation:**
- Runtime module tests pass
- CTest suite passes
- Geometry viewer refactored successfully
- Application runs without memory leaks

**Quality gates & benchmarks:**
- No performance regression in main loop execution
- Frame timing accurate within 1ms
- Clean shutdown with no resource leaks
- All existing tests remain green

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Runtime module at risk (⚠️), Platform stable (✅) | ✅ |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Documentation precedence chain confirmed | ✅ |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 GPU Enablement active, RT-410 in progress | ✅ |
| 4 | N/A - no specific backlog entry | New capability building on Phase 1 | N/A |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Runtime loop planning exists, needs Application class | ✅ |
| 6 | [`docs/modules/platform/README.md`](../../docs/modules/platform/README.md) | Platform input integration complete (Phase 1) | ✅ |
| 7 | [`docs/specs/ADR-0008`](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Runtime loop architecture and presentation contracts | ✅ |
| 8 | Phase 1 artifacts | Input integration complete, establishes patterns | ✅ |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | GitHub Copilot | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | GitHub Copilot | 2 & 5 | Context package, archive hand-off | Active |
| Specialist Engineer(s) | GitHub Copilot | 3 | Implementation, test updates | Active |
| Docs/DevRel | GitHub Copilot | 2, 4, 5 | Documentation updates, terminology review | Active |
| QA/Test Specialist | GitHub Copilot | 4 | Validation suite | Active |
| Performance Engineer | GitHub Copilot | 4 | Frame timing validation | Active |
| Safety Reviewer | GitHub Copilot | 4 | Resource lifecycle review | Active |
| Reviewer | GitHub Copilot | 4 | Code review | Active |
| Release Manager | GitHub Copilot | 5 | Release prep | Active |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Phase 1 complete, design reviewed | Task brief approved, scope clear | This document |
| 2 – Context Assembly | Task brief approved | Context package complete, no blockers | Context package document |
| 3 – Execution & Collaboration | Context package reviewed | Code implemented, tests updated | Git commits, code files |
| 4 – Quality Gates | Implementation complete | All tests pass, docs updated | CTest output, quality report |
| 5 – Release & Documentation Sync | Quality gates passed | Documentation synced, roadmap updated | Updated READMEs, ROADMAP.md |

## 7. Timeline & Milestones
- Kickoff: November 4, 2025
- Implementation window: November 4, 2025
- Quality gate window: November 4, 2025
- Release target: November 4, 2025 (incremental)
- Post-release monitoring: N/A (local development)

## 8. Known Risks & Dependencies
**Risks:**
- Presentation backend (RT-410) not complete, so Application can't wire rendering yet
- RuntimeHost doesn't have built-in run() method, Application will implement its own
- Scene management API might need adjustments for Application ownership model

**Dependencies:**
- ✅ Phase 1 complete (Window::input_state() available)
- ⚠️ RT-410 in progress (presentation backends will integrate later)
- Platform module window creation API stable
- RuntimeHost tick() method available

**Mitigations / contingency:**
- Phase 2 focuses on lifecycle and main loop, not rendering
- Keep rendering integration points as placeholders for RT-410
- Test thoroughly with geometry viewer as proof of concept
- Maintain backward compatibility with manual RuntimeHost usage

## 9. Communication Plan
- Async updates cadence: After each implementation phase
- Live sync triggers: N/A (solo agent work)
- Escalation path: Document blockers in decision log

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2025-11-04 | GitHub Copilot | Task brief created for Phase 2 | Proceeding with Application base class |
| 2025-11-04 | GitHub Copilot | Context package next | Starting context assembly |

**Status:** 🟡 IN PROGRESS - Phase 2 (Application Base Class)

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.

