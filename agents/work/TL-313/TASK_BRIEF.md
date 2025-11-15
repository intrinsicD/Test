# Task Brief — TL-313 Asset Browser Panel

> Owner: Product Manager (Role 10)
> Linked Workflow: [`AGENTS.md`](../../AGENTS.md)

## 1. Summary
- **Title:** Ship asset browser panel for hybrid editor workflow
- **Roadmap / Backlog Reference:** [`hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md`](../../hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md)
- **Primary Goal:** Deliver an ImGui asset browser panel that surfaces cached asset metadata, filtering, and hot reload status for demo operators.
- **Linked Workflow Artefacts:**
  - Task brief: (this document)
  - Context package: [`CONTEXT_PACKAGE.md`](CONTEXT_PACKAGE.md)
  - Quality report: [`QUALITY_REPORT.md`](QUALITY_REPORT.md)

## 2. Scope & Boundaries
- In scope:
  - Panel implementation in `engine/tools` with ImGui rendering.
  - Runtime bridge integration so panel registers automatically.
  - Asset cache introspection hooks to enumerate loaded resources safely.
  - Documentation updates for tools and assets modules.
  - Workflow artefact updates (backlog status, evidence logging).
- Out of scope:
  - New asset types beyond existing caches (mesh, point cloud, graph, texture, shader, material).
  - GPU residency tracking or streaming pipeline changes.
  - Telemetry visualisation beyond textual metadata.
- Architectural considerations / ADRs:
  - Respect [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) contracts for editor/runtime separation.
  - Align with tooling guidelines in [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) and asset cache invariants in [`docs/modules/assets/README.md`](../../docs/modules/assets/README.md).

## 3. Success Criteria
- Functional:
  - Asset browser lists loaded assets across supported cache types with deterministic ordering and filtering.
  - Panel integrates with `RuntimePanelBridge` and honours feature toggles.
- Documentation:
  - Tools and assets module READMEs describe the new panel and introspection helpers.
  - Backlog and roadmap remain synchronised with task status.
- Validation:
  - Unit tests cover panel data management and runtime bridge registration.
  - Aggregation logic validated through tests using real cache instances where feasible.
- Quality gates & benchmarks:
  - Build, unit tests (`ctest`), Python test suites, and documentation validation succeed per workflow checklist.

## 4. Context Ladder Snapshot *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Notes / Outstanding Questions | Owner |
| --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Confirms module layout and hybrid workflow emphasis. | tools-lead |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Located tooling & assets documentation anchors for updates. | knowledge-librarian |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Bundle B requires tooling panels; TL-313 is active. | knowledge-librarian |
| 4 | [`hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md`](../../hybrid_workflow/backlog/archive/TL-313-asset-browser-panel.md) | Acceptance criteria specify metadata coverage, filters, docs/tests updates. | tools-lead |
| 5 | [`docs/modules/tools/README.md`](../../docs/modules/tools/README.md) | ImGui patterns, panel registry usage, profiling expectations. | specialist-engineer |
| 5 | [`docs/modules/assets/README.md`](../../docs/modules/assets/README.md) | Cache invariants, telemetry, handle validation constraints. | specialist-engineer |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Editor/runtime separation, panel registration responsibilities. | specialist-engineer |
| 7 | [`docs/reviews/2025-03-22-SCENE-DOCS.MD`](../../docs/reviews/2025-03-22-SCENE-DOCS.MD) | Prior panel rollout learnings; ensure determinism & diagnostics. | specialist-engineer |

## 5. Role Roster & Phase Ownership
| Role | Name / Agent | Workflow Phases | Responsibilities | Status |
| --- | --- | --- | --- | --- |
| Agent Orchestrator | tools-lead | 1–5 | Coordinate phases, approve exits | ✅ |
| Knowledge Librarian | docs-devrel | 2 & 5 | Compile context, doc sync | ✅ |
| Specialist Engineer(s) | tools-lead | 3 | Panel implementation, tests | ✅ |
| Docs/DevRel | docs-devrel | 2, 4, 5 | Update READMEs, backlog evidence | ✅ |
| QA/Test Specialist | qa-owner | 4 | Execute validation suite | ⚙️ pending execution |
| Performance Engineer | perf-owner | 4 | Confirm panel incurs negligible cost | ⚙️ pending (visual inspection + profiling hooks) |
| Safety Reviewer | security-owner | 4 | N/A for UI-only change | 🚫 not required |
| Reviewer | reviewer-oncall | 4 | Code/document review | ⚙️ pending PR |
| Release Manager | release-owner | 5 | Bundle notes, archive artefacts | ⚙️ pending |

## 6. Phase Gate Plan *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Phase | Entry Criteria | Exit Criteria | Evidence / Linked Artefacts |
| --- | --- | --- | --- |
| 1 – Intake & Scoping | TL-313 marked ready, roadmap alignment confirmed | Task brief drafted & approved | This brief |
| 2 – Context Assembly | Brief approved, context ladder traversed | Context package populated, open questions logged | [`CONTEXT_PACKAGE.md`](CONTEXT_PACKAGE.md) |
| 3 – Execution & Collaboration | Implementation plan ratified | Code/tests/docs updated, rationale captured in backlog steps | Commit history, backlog notes |
| 4 – Quality Gates | Implementation complete | Build + test matrix green, quality report summarises results | [`QUALITY_REPORT.md`](QUALITY_REPORT.md) |
| 5 – Release & Documentation Sync | Quality report approved | Backlog status updated, artefacts archived, roadmap synced | Backlog file, docs diffs |

## 7. Timeline & Milestones
- Kickoff: 2026-05-09
- Implementation window: 2026-05-09 → 2026-05-10
- Quality gate window: 2026-05-10
- Release target: Next hybrid workflow integration build (2026-05-11)
- Post-release monitoring: Observe PM-510 demo feedback + telemetry dashboards for asset diagnostics adoption

## 8. Known Risks & Dependencies
- Risks:
  - Asset cache introspection may violate encapsulation if not added via explicit APIs.
  - Large datasets could cause UI stalls without list clipping.
- Dependencies:
  - Asset caches providing iteration hooks.
  - Runtime bridge consumers wiring caches into hooks.
- Mitigations / contingency:
  - Extend `AssetCacheLifecycle` with safe `for_each_asset` helper.
  - Use `ImGuiListClipper` and cached filtering to maintain responsiveness.

## 9. Communication Plan
- Async updates cadence: Notes in backlog **Steps** section after each milestone.
- Live sync triggers: If cache API constraints emerge or build validation fails.
- Escalation path: Agent Orchestrator → Chief Architect for API disputes → Build Engineer for CI blockers.

## 10. Decision & Status Log
| Date | Author | Note | Outcome |
| --- | --- | --- | --- |
| 2026-05-09 | tools-lead | Confirmed TL-313 scope & artefact requirements with Docs/DevRel. | Proceed to implementation |
| 2026-05-09 | tools-lead | Plan to expose cache iteration via `AssetCacheLifecycle::for_each_asset`. | Approved |
