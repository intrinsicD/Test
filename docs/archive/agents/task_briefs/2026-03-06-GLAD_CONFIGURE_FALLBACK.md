# Task Brief — GLAD-Optional Geometry Viewer Build Guard

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../../../AGENTS.md)

## 1. Summary
- **Title:** Guard geometry_viewer example behind GLAD/GLFW availability
- **Roadmap / Backlog Reference:** [`PM-520`](../../../backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) follow-up quality gate to restore canonical build flow
- **Primary Goal:** Allow the canonical build configure step to succeed when GLAD code generation is unavailable so PM-520 validation can proceed.
- **Linked Workflow Artefacts:** Task brief (this file); context package ([`../context_packages/2026-03-06-GLAD_CONFIGURE_FALLBACK.md`](../context_packages/2026-03-06-GLAD_CONFIGURE_FALLBACK.md)); quality report TBD

## 2. Scope & Boundaries
- In scope:
  - Update build scripting to skip the geometry_viewer example when GLFW/GLAD targets are missing.
  - Document the optional dependency behaviour in the tools module README.
- Out of scope:
  - Re-enabling the tools module or editor feature flags tracked by [`TL-310`](../../../backlog/active/TL_310_EDITOR_FOUNDATIONS.md).
  - Installing system dependencies inside the workspace container.
- Architectural considerations / ADRs:
  - Respect platform backend fallback guidance in [`docs/modules/platform/README.md`](../../../modules/platform/README.md) and ensure we do not assume GLFW is available when dependencies are absent.【F:docs/modules/platform/README.md†L474-L526】

## 3. Success Criteria
- Functional:
  - `cmake --preset linux-gcc-debug` configures successfully even when `glad::gl_core` is not generated.【f7515f†L1-L19】
- Documentation:
  - Tools module README explains that the geometry_viewer example requires GLFW/GLAD and will be skipped automatically when unavailable.【F:docs/modules/tools/README.md†L1-L24】
- Validation:
  - Canonical quality instrumentation commands execute and their results are recorded in the quality report.【F:README.md†L122-L142】
- Quality gates & benchmarks:
  - No change to performance budgets; ensure test matrix remains stable.

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Canonical command block fails due to missing GLAD target; need guard so configure succeeds.【F:README.md†L74-L142】【f7515f†L1-L19】 | PM |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Confirms workflow precedence and requirement to sync module docs after changes.【F:docs/NAVIGATION.md†L1-L113】 | PM |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 milestones depend on GPU/tooling readiness; build stability unblocks demo cadence.【F:docs/ROADMAP.md†L64-L118】 | PM |
| 4 | [`docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md`](../../../backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md) | Quality gate follow-up requires rerunning canonical build commands once dependencies restored.【F:docs/backlog/archive/PM_520_BACKLOG_HYGIENE_REMEDIATION.md†L1-L64】 | PM |
| 5 | [`docs/modules/tools/README.md`](../../../modules/tools/README.md) | Geometry_viewer lives in disabled tools module; docs need to explain conditional build behaviour.【F:docs/modules/tools/README.md†L1-L24】 | PM |
| 6 | [`third_party/cmake/glad.cmake`](../../../../third_party/cmake/glad.cmake) | Confirms `glad::gl_core` missing when Python/Jinja absent; example must tolerate it.【F:third_party/cmake/glad.cmake†L1-L33】 | PM |
| 7 | [`engine/tools/examples/CMakeLists.txt`](../../../../engine/tools/examples/CMakeLists.txt) | Current linking unconditionally requires `glad::gl_core`, causing configure failure.【F:engine/tools/examples/CMakeLists.txt†L1-L19】 | PM |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Agent Orchestrator | 1–5 | Approve scope and ensure backlog alignment | Pending |
| Knowledge Librarian | Knowledge Librarian | 2 & 5 | Maintain context package, archive artefacts | Pending |
| Specialist Engineer(s) | Geometry Processing Specialist | 3 | Implement CMake guard and doc update | Active |
| Docs/DevRel | Docs Team | 2, 4, 5 | Review README update | Pending |
| QA/Test Specialist | QA Lead | 4 | Confirm canonical build/test suite | Pending |
| Performance Engineer | Performance Lead | 4 | Monitor for regressions | Pending |
| Safety Reviewer | Security Reviewer | 4 | N/A (no new security surface) | Notified |
| Reviewer | Rendering Reviewer | 4 | Review CMake/doc changes | Pending |
| Release Manager | Release Manager | 5 | Coordinate release notes if required | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed; failure reproduced | Task brief approved | CMake failure log (`f7515f`) |
| 2 – Context Assembly | Task brief approved | Context package reviewed | [`../context_packages/2026-03-06-GLAD_CONFIGURE_FALLBACK.md`](../context_packages/2026-03-06-GLAD_CONFIGURE_FALLBACK.md) |
| 3 – Execution & Collaboration | Plan accepted | CMake/doc updates merged locally; tests updated | Git diff, code review |
| 4 – Quality Gates | Implementation complete | Canonical commands logged; no regressions | Quality report (TBD) |
| 5 – Release & Documentation Sync | Quality gates approved | README/roadmap alignment confirmed | PR description, docs validator |

## 7. Timeline & Milestones
- Kickoff: 2026-03-06
- Implementation window: 2026-03-06 (single session)
- Quality gate window: Immediately after implementation
- Release target: Upon successful validation and review
- Post-release monitoring: Watch nightly CI configure logs for geometry_viewer skips

## 8. Known Risks & Dependencies
- Risks:
  - Conditional build guard might hide regressions when dependencies return; mitigate with status log entries.
- Dependencies:
  - GLAD generation requires Python/Jinja installation; unavailable in current container.【f7515f†L1-L19】
- Mitigations / contingency:
  - Emit clear CMake status message when skipping the example so future engineers reinstall dependencies if desired.

## 9. Communication Plan
- Async updates cadence: Update task brief after each phase gate.
- Live sync triggers: None (single-agent session).
- Escalation path: Agent Orchestrator if configure fails after guard.

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-06 | Geometry Processing Specialist | Logged configure failure due to missing glad::gl_core | Recorded |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](.) to keep audit trails coherent.
