# Context Package — RT-410 Presentation Diagnostics Submission Coverage

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-04-12-RT_410_PRESENTATION_DIAGNOSTICS.md`](../task_briefs/2026-04-12-RT_410_PRESENTATION_DIAGNOSTICS.md)【F:docs/ROADMAP.md†L64-L95】【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
- **Backlog Entry:** [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md)【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L1-L37】
- **Roadmap Link:** [`docs/ROADMAP.md`](../../../ROADMAP.md)【F:docs/ROADMAP.md†L64-L95】
- **Workflow Phase:** Phase 2 — Context Assembly (handoff to Specialist Engineer)

## 2. Problem Summary
- Current behaviour: Presentation stage supplies `submit_render_graph`, but runtime diagnostics tests do not verify frame-graph serialization, resource events, or GPU usage after submission.【F:engine/runtime/src/api.cpp†L2836-L2856】【F:engine/runtime/tests/test_module.cpp†L2782-L2839】
- Desired behaviour: Extend presentation tests to exercise `submit_render_graph` and confirm diagnostics capture serialized graphs, resource events, and GPU usage snapshots, satisfying RT-410 instrumentation requirements before GPU backends land.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】
- Constraints / invariants: Preserve ADR-0008 contracts, rely on recording providers while rendering remains blocked, and avoid altering diagnostics schema consumed by tooling.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】【F:docs/modules/rendering/README.md†L5-L13】【F:docs/modules/runtime/README.md†L5-L256】
- Quality budgets / telemetry notes: Maintain deterministic runtime telemetry; canonical validation commands remain mandatory.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】【F:README.md†L120-L142】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Describes presentation stage semantics, diagnostics responsibilities, and outstanding RT-410 work.【F:docs/modules/runtime/README.md†L5-L256】 | 5 |
| ADR / Spec | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Mandates presentation backend separation and telemetry hooks.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】 | 6 |
| Code excerpts | [`engine/runtime/src/api.cpp`](../../../../engine/runtime/src/api.cpp) | `submit_render_graph` populates diagnostics fields post submission.【F:engine/runtime/src/api.cpp†L2836-L2856】 | — |
| Code excerpts | [`engine/runtime/tests/test_module.cpp`](../../../../engine/runtime/tests/test_module.cpp) | Existing presentation context tests to extend for diagnostics assertions.【F:engine/runtime/tests/test_module.cpp†L2782-L2839】 | — |
| Roadmap | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 prioritises RT-410; milestone timeline expects presentation coverage before GPU demos.【F:docs/ROADMAP.md†L64-L83】 | 3 |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Runtime flagged “At Risk”; follow canonical validation block for QA evidence.【F:README.md†L15-L29】【F:README.md†L120-L142】 | Knowledge Librarian | None |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Reinforces precedence order and template usage for agent artefacts.【F:docs/NAVIGATION.md†L5-L43】 | Knowledge Librarian | None |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | RT-410 priority 1 with milestone due to exercise presentation mocks before GPU demos.【F:docs/ROADMAP.md†L64-L83】 | Agent Orchestrator | Track milestone slip risk |
| 4 | [`docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md`](../../../backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md) | DoD requires automated presentation tests and diagnostics updates; dependencies include T-0119/T-0120.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L48】 | Specialist Engineer | Ensure tests align with DoD |
| 5 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Documents presentation stage, diagnostics expectations, and mock backend usage.【F:docs/modules/runtime/README.md†L5-L256】 | Specialist Engineer | Align assertions with documentation |
| 6 | [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../../../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md) | Confirms presentation backend separation and telemetry obligations.【F:docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md†L33-L118】 | Specialist Engineer | Verify compliance |
| 7 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Rendering blocked; rely on recording providers for validation coverage.【F:docs/modules/rendering/README.md†L5-L13】 | Knowledge Librarian | None |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None; rely on CPU-only recording providers for deterministic execution.【F:docs/modules/rendering/README.md†L5-L13】
- Benchmark targets & expected deltas: Monitor runtime diagnostics to ensure frame-graph serialization and resource telemetry remain stable; no new performance thresholds defined.【F:docs/backlog/active/RT_410_RUNTIME_STAGE_PLANNER.md†L31-L37】
- Tooling updates required: None; existing diagnostics consumers continue to read populated fields.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Do diagnostics need reset between ticks to avoid residual data in assertions? | Specialist Engineer | Before test implementation | Verify existing teardown resets recording providers; adjust fixtures if necessary. |
| Are additional documentation updates required if diagnostics fields become mandatory for tooling? | Docs/DevRel | Before release | Pending — update runtime README only if behaviour changes beyond enforced coverage. |

## 7. Attachments
- Diagrams: None.
- Data sets: Reuse deterministic handle fixtures in runtime tests; no new assets required.【F:engine/runtime/tests/test_module.cpp†L2786-L2835】
- Additional notes: Ensure tests capture diagnostics snapshot after tick but before shutdown to avoid clearing telemetry.

> **Checklist:** Ensure every link resolves, cite the relevant sections in [`CONTRIBUTION.md`](../../../../CONTRIBUTION.md), and confirm documentation owners are tagged in the task brief.
