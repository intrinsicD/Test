# Task Brief — RT-410 Loop Plan Introspection Bridge

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** Expose runtime loop plan serialization to scripting clients
- **Roadmap / Backlog Reference:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md) · [`RT-410`](../../docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)
- **Primary Goal:** Provide an official bridge that lets Python tooling retrieve the compiled runtime loop plan so harnesses can validate presentation wiring without parsing diagnostics dumps.
- **Linked Workflow Artefacts:**
  - Task brief: `agents/task_briefs/2026-11-05-rt-410-loop-plan-introspection.md`
  - Context package: `agents/context_packages/2026-11-05-rt-410-loop-plan-introspection.md`
  - Quality report: `agents/quality_reports/2026-11-05-rt-410-loop-plan-introspection.md`

## 2. Scope & Boundaries
- In scope:
  - Extend the runtime C API with a stable export that returns the JSON serialization of the active loop plan.
  - Surface the new symbol through `engine3g.loader.EngineRuntimeHandle` with typed accessors and `.pyi` coverage.
  - Add Python unit coverage proving the binding works against the dummy runtime namespace.
  - Update runtime + Python module documentation to reference the new hook and its role in RT-410 synchronisation deliverables.
- Out of scope:
  - Implementing GPU presentation backends or encoder work tracked under T-0120/T-0119.
  - Reworking runtime diagnostics payloads beyond exposing the existing serialization string.
- Architectural considerations / ADRs:
  - [`ADR-0008`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) mandates scripting access to loop configuration/inspection.

## 3. Success Criteria
- Functional:
  - New C export `engine_runtime_loop_plan_serialization()` returns a UTF-8 string (empty when no plan is active).
  - `EngineRuntimeHandle.loop_plan_serialization()` decodes the payload and raises a descriptive error if the symbol is missing.
- Documentation:
  - Runtime module README highlights the scripting hook for loop-plan inspection.
  - Python bindings README documents the new accessor for harness authors.
- Validation:
  - Python tests cover the happy path and missing-symbol failure case.
- Quality gates & benchmarks:
  - Canonical CMake + pytest + docs validation suite passes per [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation).

### Implementation Plan
1. **Runtime export:** Add a `const char* engine_runtime_loop_plan_serialization()` C entry point in `engine/runtime/src/api.cpp` that forwards to `diagnostics().loop_plan_serialization` and returns an empty string when the plan is absent. Document lifetime expectations inline.
2. **Python binding:** Load the new symbol inside `EngineRuntimeHandle.__init__`, enforcing availability via a descriptive `RuntimeError`, and expose a `.loop_plan_serialization()` accessor plus `.pyi` stub coverage.
3. **Unit coverage:** Extend `python/tests/test_loader.py` to validate both success and missing-symbol paths using the dummy runtime namespace.
4. **Documentation:** Update `docs/modules/runtime/README.md` and `python/engine3g/README.md` to mention the scripting hook for loop-plan inspection.
5. **Backlog sync:** Note incremental RT-410 progress in the backlog entry’s notes section once implementation lands.

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Runtime module remains “At Risk,” blocked on RT-410 synchronisation hooks.【F:README.md†L26-L27】 | Product Manager |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Confirms roadmap/backlog precedence for phase-4 milestones. | Knowledge Librarian |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | RT-410 is priority-1 within the GPU enablement phase.【F:docs/ROADMAP.md†L64-L93】 | Product Manager |
| 4 | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD calls for scripting/diagnostics hooks to expose the stage planner.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】 | Specialist Engineer |
| 5 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Documents `diagnostics.loop_plan_serialization` but no direct scripting surface yet.【F:docs/modules/runtime/README.md†L229-L244】 | Specialist Engineer |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Decision 4 mandates scripting bridges for loop inspection.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L62-L115】 | Knowledge Librarian |
| 7 | Prior RT-410 context packages (archived) | Confirm previous increments focused on diagnostics/presentation hooks; no dedicated scripting export recorded. | Knowledge Librarian |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | World-Class Assistant | 1–5 | Coordinate phases, approve exits | Active |
| Knowledge Librarian | World-Class Assistant | 2 & 5 | Compile context, archive artefacts | Active |
| Specialist Engineer(s) | World-Class Assistant | 3 | Implement runtime/Python changes, add tests | Active |
| Docs/DevRel | World-Class Assistant | 2, 4, 5 | Update runtime + Python READMEs | Active |
| QA/Test Specialist | World-Class Assistant | 4 | Execute canonical build/test commands | Active |
| Performance Engineer | World-Class Assistant | 4 | Confirm no performance implications (N/A) | Not Required |
| Safety Reviewer | World-Class Assistant | 4 | Verify exported API has safe lifetime semantics | Pending |
| Reviewer | World-Class Assistant | 4 | Review implementation/doc updates | Pending |
| Release Manager | World-Class Assistant | 5 | Ensure backlog/roadmap/doc sync in PR | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap priority confirmed; backlog acceptance understood. | Task brief approved. | This document. |
| 2 – Context Assembly | Knowledge Librarian traversed context ladder. | Context package completed, open questions logged. | `agents/context_packages/2026-11-05-rt-410-loop-plan-introspection.md` |
| 3 – Execution & Collaboration | Implementation plan ratified. | Code + tests + docs ready for validation. | Git commits referenced in quality report. |
| 4 – Quality Gates | Implementation complete. | Canonical command block passes; quality report signed. | `agents/quality_reports/2026-11-05-rt-410-loop-plan-introspection.md` |
| 5 – Release & Documentation Sync | Quality report approved. | Backlog/roadmap/docs updated; artefacts archived. | PR + backlog update log. |

## 7. Timeline & Milestones
- Kickoff: 2026-11-05
- Implementation window: 2026-11-05 (single session)
- Quality gate window: Immediately after implementation
- Release target: 2026-11-05
- Post-release monitoring: Verify harness scripts adopt new accessor during next weekly demo (PM-510)

## 8. Known Risks & Dependencies
- Risks:
  - Mismanaging C-string lifetime could expose dangling pointers to scripting clients.
  - Loader initialisation might regress if symbol detection is incorrect.
- Dependencies:
  - Existing diagnostics string stored in `RuntimeDiagnostics::loop_plan_serialization`.
  - Python loader’s dummy runtime harness used in tests.
- Mitigations / contingency:
  - Return the `c_str()` owned by diagnostics (stable until plan rebuild); document empty-string semantics.
  - Add explicit missing-symbol tests to guard loader regression.

## 9. Communication Plan
- Async updates cadence: Record progress in brief after each phase.
- Live sync triggers: N/A (single-session change).
- Escalation path: Route blockers to Agent Orchestrator; architecture concerns to Runtime Lead per roadmap.

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-11-05 | World-Class Assistant | Scoped RT-410 scripting bridge task; ready for context assembly. | ✅ |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
