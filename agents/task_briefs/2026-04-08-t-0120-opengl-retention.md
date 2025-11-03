# Task Brief — OpenGL Presentation Retention Configuration

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** T-0120 OpenGL presentation retention configuration
- **Roadmap / Backlog Reference:** [docs/ROADMAP.md](../../docs/ROADMAP.md) · [T-0120](../../docs/backlog/active/T-0120-gpu-resource-provider.md)
- **Primary Goal:** Expose retention-frame controls for the OpenGL presentation backend so runtime and tooling teams can tune transient GPU resource caching while delivering the T-0120 provider milestones.【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L12-L37】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-04-08-t-0120-opengl-retention.md`), context package (`agents/context_packages/2026-04-08-t-0120-opengl-retention.md`), quality report (TBD)

## 2. Scope & Boundaries
- In scope: Add retention-frame parameters and setters to `OpenGLRuntimeSubmission` and `OpenGLPresentationBackend`; update runtime diagnostics/tests to validate the configurability; document the new control path in rendering/runtime READMEs.【F:engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp†L29-L43】【F:engine/rendering/include/engine/rendering/backend/opengl/presentation_backend.hpp†L28-L50】【F:docs/modules/rendering/README.md†L302-L309】【F:docs/modules/runtime/README.md†L235-L252】
- Out of scope: Shader compilation, pipeline cache invalidation, and non-OpenGL backend retention policies tracked elsewhere in T-0120.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L31-L37】
- Architectural considerations / ADRs: Preserve frame-graph/provider contracts (ADR-0003) and presentation loop responsibilities (ADR-0008) when threading retention controls through the runtime submission stack.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L31-L118】

## 3. Success Criteria
- Functional: Presentation backends can configure GPU resource retention frames without directly mutating provider internals, enabling runtime hosts to balance reuse against memory pressure.【F:engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp†L29-L37】
- Documentation: Rendering/runtime READMEs note the retention tuning hook and its relationship to T-0120.【F:docs/modules/rendering/README.md†L302-L309】【F:docs/modules/runtime/README.md†L248-L252】
- Validation: Unit coverage confirms constructor/setter behaviour; runtime presentation tests assert retention propagation; canonical build/test/doc commands executed per workflow blueprint.【F:engine/runtime/tests/test_opengl_presentation_backend.cpp†L31-L95】【F:README.md†L120-L142】
- Quality gates & benchmarks: Standard linux-gcc-debug CMake build, ctest, pytest, and docs validation succeed; no new performance benchmarks required.【F:README.md†L120-L142】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [README.md](../../README.md) | Rendering/runtime flagged as blocked/at-risk pending GPU enablement; canonical validation commands captured.【F:README.md†L15-L29】【F:README.md†L120-L142】 | Product Manager |
| 2 | [docs/NAVIGATION.md](../../docs/NAVIGATION.md) | Confirms precedence, module README expectations, and telemetry references.【F:docs/NAVIGATION.md†L5-L114】 | Knowledge Librarian |
| 3 | [docs/ROADMAP.md](../../docs/ROADMAP.md) | Phase 4 priority bands emphasise T-0120/T-0119/RT-410 integration.【F:docs/ROADMAP.md†L64-L95】 | Agent Orchestrator |
| 4 | [T-0120 backlog](../../docs/backlog/active/T-0120-gpu-resource-provider.md) | Retention tuning noted in notes; DoD targets GPU provider maturity.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L31-L55】 | Specialist Engineer |
| 5 | [Rendering module README](../../docs/modules/rendering/README.md) | Module blocked awaiting real GPU execution; telemetry expectations documented.【F:docs/modules/rendering/README.md†L5-L31】【F:docs/modules/rendering/README.md†L302-L309】 | Specialist Engineer |
| 6 | [Runtime module README](../../docs/modules/runtime/README.md) | Presentation backend guidance and diagnostics responsibilities highlighted.【F:docs/modules/runtime/README.md†L125-L171】【F:docs/modules/runtime/README.md†L235-L252】 | Specialist Engineer |
| 7 | [ADR-0003](../../docs/specs/ADR-0003-runtime-frame-graph.md) & [ADR-0008](../../docs/specs/ADR-0008-runtime-main-loop-and-tooling.md) | Frame-graph/provider contracts and presentation loop separation must remain intact.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L118】 | Chief Architect |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Assigned this session | 1–5 | Maintain roadmap alignment, approve gate exits | Planned |
| Knowledge Librarian | Assigned this session | 2 & 5 | Maintain context package, archive artefacts | Planned |
| Specialist Engineer(s) | Rendering/runtime contributor | 3 | Implement retention API, tests, docs | Planned |
| Docs/DevRel | Docs reviewer | 2, 4, 5 | Review README updates and terminology | Planned |
| QA/Test Specialist | QA reviewer | 4 | Execute validation commands, review logs | Planned |
| Performance Engineer | N/A for this change | 4 | — | N/A |
| Safety Reviewer | Rendering reviewer | 4 | Confirm API changes preserve safety/invariants | Planned |
| Reviewer | Rendering/runtime maintainer | 4 | Code review | Planned |
| Release Manager | Release coordinator | 5 | Ensure backlog/roadmap notes updated if criteria met | Planned |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Context ladder traversed; backlog alignment confirmed. | Task brief approved. | This brief. |
| 2 – Context Assembly | Module/ADR review complete; risks recorded. | Context package reviewed/accepted. | `agents/context_packages/2026-04-08-t-0120-opengl-retention.md`. |
| 3 – Execution & Collaboration | Plan acknowledged by reviewer. | Retention API, tests, docs ready for validation. | Git history, inline notes. |
| 4 – Quality Gates | Implementation complete; validation plan defined. | Standard build/test/doc commands green; QA sign-off. | Quality report (TBD). |
| 5 – Release & Documentation Sync | Quality gates passed. | PR merged; backlog/doc status updated; artefacts archived. | Updated brief/context + PR. |

## 7. Timeline & Milestones
- Kickoff: 2026-04-08
- Implementation window: 2026-04-08 → 2026-04-09
- Quality gate window: Immediately post-implementation using linux-gcc-debug toolchain【F:README.md†L120-L142】
- Release target: 2026-04-09 (merge-ready patch)
- Post-release monitoring: Confirm PM-510 demo telemetry reflects retention adjustments during subsequent GPU integration syncs.【F:docs/ROADMAP.md†L64-L95】

## 8. Known Risks & Dependencies
- Risks: API adjustments may desynchronise with future T-0120 shader/pipeline work; insufficient documentation could leave tooling unaware of retention tuning knobs.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L31-L37】【F:docs/modules/rendering/README.md†L5-L31】【F:docs/modules/rendering/README.md†L302-L309】
- Dependencies: OpenGL GPU resource provider implementation, runtime presentation loop, PM-510 integration cadence.【F:engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp†L29-L43】【F:docs/modules/runtime/README.md†L235-L252】【F:docs/ROADMAP.md†L64-L95】
- Mitigations / contingency: Mirror existing provider configuration semantics; ensure docs/tests illustrate usage; coordinate with PM-510 demo owners for telemetry validation.【F:engine/rendering/include/engine/rendering/backend/opengl/resource_provider.hpp†L26-L41】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L12-L37】【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L41】

## 9. Communication Plan
- Async updates cadence: Daily notes in task brief/context until merge; highlight retention telemetry impact in PM-510 summaries.【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L41】
- Live sync triggers: Escalate to Agent Orchestrator if retention API requires broader provider interface changes or conflicts with upcoming shader work.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L31-L37】
- Escalation path: Rendering lead → Agent Orchestrator per roadmap governance.【F:docs/ROADMAP.md†L64-L95】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-04-08 | Product Manager | Scoped retention configuration increment for OpenGL presentation backend under T-0120. | In Progress |
| 2026-04-09 | Product Manager | Reviewed context ladder, confirmed retention propagation scope, and planned validation run (linux-gcc-debug build → ctest → pytest → docs).【F:README.md†L120-L142】 | In Progress |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
