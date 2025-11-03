# Task Brief — GPU Resource Usage Telemetry Integration

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** GPU resource usage telemetry integration
- **Roadmap / Backlog Reference:** Phase 4 GPU enablement focus with [`T-0120`](../../docs/backlog/active/T-0120-gpu-resource-provider.md) at priority 1 on the central roadmap.【F:docs/ROADMAP.md†L64-L94】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L50】
- **Primary Goal:** Extend the GPU resource provider surface so runtime diagnostics expose live GPU memory usage telemetry for OpenGL/Vulkan backends, satisfying T-0120’s telemetry deliverables.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L9-L37】【F:docs/modules/rendering/README.md†L1-L13】
- **Linked Workflow Artefacts:** Task brief (`agents/task_briefs/2026-03-27-gpu-resource-telemetry.md`), context package (`agents/context_packages/2026-03-27-gpu-resource-telemetry.md`), quality report (TBD).

## 2. Scope & Boundaries
- **In scope:**
  - Introduce a resource usage snapshot API on `rendering::resources::IGpuResourceProvider` and implement it for OpenGL, Vulkan, and recording providers.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L32-L37】【F:docs/modules/rendering/README.md†L1-L13】
  - Capture the snapshot inside `RuntimeHost` and publish GPU memory gauges through runtime metrics so PM-510 demos receive consistent telemetry.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L15-L37】【F:docs/modules/runtime/README.md†L1-L24】
  - Update rendering/runtime READMEs to document the new telemetry output per workflow guidance.【F:README.md†L5-L107】
- **Out of scope:** Command encoder API changes, presentation backend feature work, shader compilation correctness beyond telemetry hooks.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L33-L37】
- **Architectural considerations / ADRs:** Respect frame-graph contracts in [`ADR-0003`](../../docs/specs/ADR-0003-runtime-frame-graph.md) while extending provider telemetry.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】

## 3. Success Criteria
- **Functional:** Runtime diagnostics expose GPU buffer/texture usage from active providers each frame; API consumers can query totals without backend-specific casts.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L15-L37】
- **Documentation:** Rendering and runtime module READMEs describe the new telemetry, with roadmap/backlog references updated if scope completes.【F:docs/modules/rendering/README.md†L1-L13】【F:docs/modules/runtime/README.md†L1-L24】
- **Validation:** Unit coverage for provider snapshots plus runtime metric expectations; telemetry visible through existing metric enumeration tests.【F:README.md†L122-L144】【F:engine/runtime/tests/test_module.cpp†L2000-L2065】
- **Quality gates & benchmarks:** Standard build/tests (`cmake`, `ctest`, `pytest`, `python scripts/validate_docs.py`) remain green per workflow blueprint.【F:README.md†L122-L144】

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Confirms GPU enablement focus and quality workflow expectations.【F:README.md†L5-L144】 | Agent |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Establishes precedence, telemetry references, and module documentation paths.【F:docs/NAVIGATION.md†L1-L114】 | Agent |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 priority highlights T-0120 and telemetry deliverables.【F:docs/ROADMAP.md†L64-L118】 | Agent |
| 4 | [`docs/backlog/active/T-0120-gpu-resource-provider.md`](../../docs/backlog/active/T-0120-gpu-resource-provider.md) | Defines telemetry alignment and DoD checkpoints for resource provider work.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L50】 | Agent |
| 5 | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Notes resource provider gap blocking rendering; frames telemetry context.【F:docs/modules/rendering/README.md†L1-L150】 | Agent |
| 6 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Runtime telemetry responsibilities and outstanding RT-410 dependencies.【F:docs/modules/runtime/README.md†L1-L124】 | Agent |
| 7 | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../docs/specs/ADR-0003-runtime-frame-graph.md) | Interface contracts for resource acquisition/telemetry integration.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】 | Agent |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | Product Manager proxy | 1–5 | Approve scope, monitor roadmap alignment | Pending |
| Knowledge Librarian | Docs/DevRel proxy | 2 & 5 | Context package, documentation sync | Pending |
| Specialist Engineer(s) | Runtime/Rendering engineer (agent) | 3 | Implement provider API + telemetry integration | Active |
| Docs/DevRel | Docs reviewer | 2, 4, 5 | Review README updates | Pending |
| QA/Test Specialist | QA reviewer | 4 | Validate new metrics/tests | Pending |
| Performance Engineer | Performance reviewer | 4 | Confirm telemetry usefulness for PM-510 demos | Pending |
| Safety Reviewer | Security reviewer | 4 | Confirm API changes don’t violate safety policies | Pending |
| Reviewer | Rendering/runtime reviewer | 4 | Code review | Pending |
| Release Manager | Release coordinator | 5 | Changelog & backlog sync | Pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | Roadmap/backlog alignment confirmed, scope bounded.【F:docs/ROADMAP.md†L64-L118】【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L1-L37】 | Task brief approved. | This brief |
| 2 – Context Assembly | Context ladder traversed, questions logged.【F:docs/NAVIGATION.md†L1-L114】 | Context package reviewed. | Pending (`agents/context_packages/2026-03-27-gpu-resource-telemetry.md`) |
| 3 – Execution & Collaboration | Plan agreed with reviewers. | Code/tests/docs updated, telemetry exposed. | Pending implementation |
| 4 – Quality Gates | Implementation ready; test plan defined.【F:README.md†L122-L144】 | Build/test/docs validation successful, reviewers sign off. | Pending quality report |
| 5 – Release & Documentation Sync | Quality gates passed. | PR merged, backlog/roadmap updated if criteria met. | Pending |

## 7. Timeline & Milestones
- **Kickoff:** 2026-03-27 (context assembled).
- **Implementation window:** 2026-03-27 → 2026-03-28 (single iteration scope).
- **Quality gate window:** Immediately post-implementation with standard pipeline.【F:README.md†L122-L144】
- **Release target:** Merge once telemetry verified in metrics snapshot.
- **Post-release monitoring:** Confirm PM-510 telemetry scripts reflect new gauges in next demo cadence.【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L37】

## 8. Known Risks & Dependencies
- **Risks:**
  - API change ripple across rendering backends; risk of breaking existing scheduler/provider unit tests.【F:docs/modules/rendering/README.md†L1-L13】
  - Telemetry inflation if metrics mis-reported, confusing PM-510 dashboards.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L15-L37】
- **Dependencies:** Runtime diagnostics pipeline, rendering providers, telemetry schema consumers.【F:docs/modules/runtime/README.md†L1-L124】【F:docs/design/TELEMETRY_SCHEMA.md†L94-L105】
- **Mitigations / contingency:** Maintain backwards-compatible defaults (zero usage), extend unit coverage for each provider, coordinate with Docs/DevRel for schema notes.【F:docs/modules/rendering/README.md†L130-L157】【F:docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md†L87-L127】

## 9. Communication Plan
- **Async updates cadence:** Daily notes in task brief until merge; highlight telemetry results to PM-510 stakeholders.【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L37】
- **Live sync triggers:** Escalate if API review or telemetry schema changes required before merge.【F:docs/NAVIGATION.md†L97-L114】
- **Escalation path:** Rendering lead → Agent Orchestrator for scope conflicts; Docs/DevRel for schema documentation issues.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L17-L29】

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-03-27 | Specialist Engineer | Logged scope and telemetry objective for GPU resource provider. | Pending |
| 2026-03-28 | Specialist Engineer | Executed linux-gcc-debug configure/build, full ctest/pytest/doc validation; GPU telemetry gauges now exposed in runtime metrics and tests.【ea453e†L1-L11】【423eb4†L1-L2】【3c05a6†L1-L9】【c2be1c†L1-L6】【73f94b†L1-L2】【cdf811†L23-L36】【db9233†L31-L61】 | Quality gates passed locally |

> **Reminder:** Update this brief whenever scope, roster, or schedule changes. Link completed templates from [`agents/TEMPLATES`](../TEMPLATES) to keep audit trails coherent.
