# Task Brief Template

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** RT-410 Presentation Stage Activity C API Bridge
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../docs/ROADMAP.md) · [RT-410](../../docs/backlog/active/RT-410-runtime-stage-planner.md)
- **Primary Goal:** Extend the runtime C API and Python loader so tooling can query whether the `presentation.dispatch` stage is active, fulfilling RT-410’s synchronisation hook requirement for scripting surfaces.【F:docs/ROADMAP.md†L64-L108】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-01-rt-410-presentation-stage-capi.md`), context package (`agents/context_packages/2026-03-01-rt-410-presentation-stage-capi.md`)

## 2. Scope & Boundaries
- In scope: Add an exported `engine_runtime_presentation_stage_active()` C API, wire the Python `EngineRuntimeHandle` to the new symbol, update runtime tests/loader tests, and document the bridge in the runtime module README/backlog notes.【F:python/engine3g/loader.py†L317-L360】【F:python/tests/test_loader.py†L38-L64】【F:docs/modules/runtime/README.md†L5-L128】
- Out of scope: Implementing GPU presentation backends (T-0120/T-0119) or editor/tooling enablement tracked under TL-310; this increment only surfaces the existing state flag to scripting consumers.【F:README.md†L15-L144】【F:docs/modules/rendering/README.md†L5-L40】
- Architectural considerations / ADRs: Align with ADR-0008 guidance on scripting hooks and presentation instrumentation without altering loop planning semantics.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L129】

## 3. Success Criteria
- Functional: C and Python consumers can call a stable API to determine if presentation handlers are active, mirroring the C++ accessor semantics and enabling harness gating without parsing diagnostics.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】【F:python/engine3g/loader.py†L317-L360】
- Documentation: Update the runtime README and backlog notes describing the new scripting hook so Docs/DevRel can reference it during RT-410 demos.【F:docs/modules/runtime/README.md†L5-L128】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L1-L37】
- Validation: Execute the canonical linux-gcc-debug configure/build/test pipeline plus Python/doc validators to prove the bridge behaves deterministically.【F:README.md†L120-L144】
- Quality gates & benchmarks: Existing runtime loop/presentation coverage and loader unit tests remain green; new tests verify the C/Python bridge toggles in sync with callback/backend mutations.【F:engine/runtime/tests/test_module.cpp†L2330-L2684】【F:python/tests/test_loader.py†L38-L120】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Runtime flagged “At Risk” pending RT-410, canonical build/test workflow captured for validation.【F:README.md†L15-L144】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms precedence order and template usage for brief/context capture.【F:docs/NAVIGATION.md†L5-L116】 | Product Manager |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 prioritises RT-410 with due-date sequencing against GPU milestones.【F:docs/ROADMAP.md†L64-L108】 | Product Manager |
| 4 | [RT-410 backlog](../../docs/backlog/active/RT-410-runtime-stage-planner.md) | DoD emphasises synchronisation hooks + tooling documentation; C API bridge addresses scripting gap.【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L31-L37】 | Product Manager |
| 5 | [docs/modules/runtime/README.md](../../docs/modules/runtime/README.md) | Highlights presentation stage hot-swapping and need for scripting accessors; README will note C bridge.【F:docs/modules/runtime/README.md†L5-L128】 | Product Manager |
| 6 | [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Mandates scripting hooks and presentation instrumentation alignment.【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L129】 | Product Manager |
| 7 | [python/engine3g/loader.py](../../python/engine3g/loader.py) & [runtime tests](../../engine/runtime/tests/test_module.cpp) | Loader currently lacks a presentation-stage query despite runtime API support; new tests will bridge the gap.【F:python/engine3g/loader.py†L317-L360】【F:engine/runtime/tests/test_module.cpp†L2330-L2684】 | Product Manager |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Align scope with roadmap priorities, approve gate exits. | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Maintain context package, archive artefacts. | Planned |
| Specialist Engineer(s) | Runtime/Python contributor | 3 | Implement C API bridge, tests, docs. | Planned |
| Docs/DevRel | Runtime docs reviewer | 2, 4, 5 | Review README/backlog updates. | Planned |
| QA/Test Specialist | Runtime & Python QA | 4 | Verify runtime + Python tests, capture logs. | Planned |
| Performance Engineer | N/A (no perf impact) | 4 | — | N/A |
| Safety Reviewer | N/A (no new security surface) | 4 | — | N/A |
| Reviewer | Runtime reviewer | 4 | Code review for API export + bindings. | Planned |
| Release Manager | Release coordinator | 5 | Ensure backlog/doc updates captured. | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed, backlog alignment confirmed. | Task brief + context package drafted. | This brief + context package. |
| 2 – Context Assembly | Module/ADR references reviewed; scripting needs captured. | Implementation plan recorded in brief/context. | Context package §§2–6. |
| 3 – Execution & Collaboration | Implementation plan approved by reviewer/agent lead. | C API, Python loader, docs, tests updated with citations. | Git commits, review notes. |
| 4 – Quality Gates | Build/test/doc validators executed. | Command logs captured, QA sign-off. | Test outputs attached to quality report. |
| 5 – Release & Documentation Sync | QA approvals complete. | Backlog/task brief updated; artefacts archived. | Updated brief/context + PR summary. |

## 7. Timeline & Milestones
- Kickoff: 2026-03-01
- Implementation window: 2026-03-01 (current session)
- Quality gate window: Immediate post-implementation
- Release target: 2026-03-01 (merge-ready change)
- Post-release monitoring: Verify harness/tests consume the new C API in upcoming PM-510 demos.【F:docs/ROADMAP.md†L75-L108】

## 8. Known Risks & Dependencies
- Risks: Loader integration may silently skip the new symbol if the shared library predates the change; tests must enforce symbol presence.【F:python/engine3g/loader.py†L317-L360】
- Dependencies: Runtime presentation stage toggles already validated via C++ tests; this work depends on the existing accessor semantics remaining stable.【F:engine/runtime/tests/test_module.cpp†L2330-L2684】
- Mitigations / contingency: Add loader guard rails that raise informative errors when the symbol is missing and ensure documentation clarifies version expectations.【F:python/tests/test_loader.py†L38-L120】

## 9. Communication Plan
- Async updates cadence: Document progress in RT-410 notes/backlog and PM-510 demo prep once the bridge lands.【F:docs/ROADMAP.md†L75-L108】【F:docs/backlog/active/RT-410-runtime-stage-planner.md†L12-L37】
- Live sync triggers: Notify runtime lead if C API export impacts ABI compatibility or requires release coordination.
- Escalation path: Raise blockers with Agent Orchestrator to preserve RT-410 Phase 4 timeline.【F:docs/ROADMAP.md†L64-L108】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-01 | Product Manager | Scoped RT-410 increment to expose presentation stage activity through the C/Python bridge. | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
