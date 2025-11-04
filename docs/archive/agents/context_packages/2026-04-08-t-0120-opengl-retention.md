# Context Package — OpenGL Presentation Retention Configuration

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-04-08-t-0120-opengl-retention.md`](../task_briefs/2026-04-08-t-0120-opengl-retention.md)
- **Backlog Entry:** [`docs/backlog/active/T-0120-gpu-resource-provider.md`](../../../backlog/active/T-0120-gpu-resource-provider.md)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../../ROADMAP.md)
- **Workflow Phase:** Phase 3 — Execution & Collaboration (next gate owner: QA/Test Specialist)

## 2. Problem Summary
- Current behaviour: `OpenGLRuntimeSubmission` and `OpenGLPresentationBackend` always instantiate the GPU resource provider with default retention (zero), offering no public knobs for runtime/presentation tooling to adjust transient caching; only the provider’s direct API exposes `set_retention_frames`, which backends do not surface.【F:engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp†L29-L37】【F:engine/rendering/src/backend/opengl/runtime_adapter.cpp†L7-L25】【F:engine/rendering/include/engine/rendering/backend/opengl/presentation_backend.hpp†L28-L42】【F:engine/rendering/include/engine/rendering/backend/opengl/resource_provider.hpp†L26-L41】
- Desired behaviour: Allow presentation backends to configure retention frames when constructed or at runtime so T-0120 stakeholders can balance reuse vs. memory pressure during PM-510 demos and RT-410 presentation bring-up.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L12-L37】【F:docs/ROADMAP.md†L64-L95】【F:docs/modules/runtime/README.md†L235-L252】
- Constraints / invariants: Honour ADR-0003 frame-graph/provider contracts and ADR-0008 presentation loop semantics; avoid introducing backend-specific leakage into runtime APIs beyond controlled setters; keep runtime diagnostics in sync.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L118】
- Quality budgets / telemetry notes: Rendering module README tracks telemetry expectations (GPU resource gauges); retention tuning must preserve existing diagnostics coverage and support PM-510 reporting cadence.【F:docs/modules/rendering/README.md†L5-L31】【F:docs/modules/rendering/README.md†L288-L309】【F:docs/backlog/active/PM-510-weekly-integration-demos.md†L1-L41】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Documents GPU provider gap and telemetry expectations for PM-510 demos. | 5 |
| Module README | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Describes presentation backend usage and diagnostics integration. | 6 |
| ADR / Spec | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../../specs/ADR-0003-runtime-frame-graph.md) | Provider contract for frame-graph execution. | 7 |
| ADR / Spec | [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Presentation backend + tooling requirements. | 7 |
| Code excerpts | [`engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp`](../../../../engine/rendering/include/engine/rendering/backend/opengl/runtime_adapter.hpp) | Submission adapter currently hardcodes provider construction. | — |
| Code excerpts | [`engine/rendering/include/engine/rendering/backend/opengl/presentation_backend.hpp`](../../../../engine/rendering/include/engine/rendering/backend/opengl/presentation_backend.hpp) | Presentation backend lacks retention configuration. | — |
| Code excerpts | [`engine/runtime/tests/test_opengl_presentation_backend.cpp`](../../../../engine/runtime/tests/test_opengl_presentation_backend.cpp) | Existing test coverage for OpenGL backend execution. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../../../README.md) | Rendering module blocked; runtime at-risk pending GPU enablement; standard validation commands recorded.【F:README.md†L15-L29】【F:README.md†L120-L142】 | Agent | None |
| 2 | [`docs/NAVIGATION.md`](../../../NAVIGATION.md) | Confirms precedence order and module README requirements for documentation updates.【F:docs/NAVIGATION.md†L5-L114】 | Agent | None |
| 3 | [`docs/ROADMAP.md`](../../../ROADMAP.md) | Phase 4 prioritises T-0120/T-0119/RT-410; telemetry & demo cadence depend on GPU provider maturity.【F:docs/ROADMAP.md†L64-L95】 | Agent | Coordinate updates with PM-510 |
| 4 | [`docs/backlog/active/T-0120-gpu-resource-provider.md`](../../../backlog/active/T-0120-gpu-resource-provider.md) | Notes retention-window tuning as an active deliverable; DoD emphasises provider parity across backends.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L12-L37】 | Agent | Ensure change satisfies DoD bullet on caching |
| 5 | [`docs/modules/rendering/README.md`](../../../modules/rendering/README.md) | Highlights missing GPU execution/telemetry; update required once retention controls available.【F:docs/modules/rendering/README.md†L5-L31】【F:docs/modules/rendering/README.md†L288-L309】 | Specialist Engineer | Plan doc edits |
| 6 | [`docs/modules/runtime/README.md`](../../../modules/runtime/README.md) | Presentation backend guidance relies on configurable presenters; doc needs mention of retention tuning.【F:docs/modules/runtime/README.md†L125-L171】【F:docs/modules/runtime/README.md†L235-L252】 | Specialist Engineer | Prepare doc snippet |
| 7 | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../../specs/ADR-0003-runtime-frame-graph.md), [`docs/specs/ADR-0008-runtime-main-loop-and-tooling.md`](../../../specs/ADR-0008-runtime-main-loop-and-tooling.md) | Architectural constraints: provider API remains backend-neutral; presentation stage must rebuild when handlers change.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/specs/ADR-0008-runtime-main-loop-and-tooling.md†L33-L118】 | Chief Architect | No action beyond adherence |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../../../AGENTS.md#05-quality-instrumentation))*
- Canonical command block copied:
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- Additional presets / datasets: None required; unit tests operate on stubbed OpenGL provider.
- Benchmark targets & expected deltas: No performance benchmarks; telemetry verification handled via runtime diagnostics inspection.
- Tooling updates required: Update rendering/runtime READMEs; ensure PM-510 documentation references new retention control if necessary.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Should retention changes trigger loop-plan rebuilds or telemetry notifications? | Specialist Engineer | During implementation | Retention updates will call provider setter directly; no loop-plan rebuild required; document behaviour in README. |

## 7. Attachments
- Diagrams: Not applicable.
- Data sets: Not required (tests operate on in-memory resources).
- Additional notes: Coordinate doc wording with Docs/DevRel to align with existing telemetry terminology.

