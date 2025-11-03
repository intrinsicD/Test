# Context Package — GPU Resource Usage Telemetry Integration

> Owner: Knowledge Librarian (Role 12)

## 1. Task Reference
- **Task Brief:** [`agents/task_briefs/2026-03-27-gpu-resource-telemetry.md`](../task_briefs/2026-03-27-gpu-resource-telemetry.md)
- **Backlog Entry:** [`docs/backlog/active/T-0120-gpu-resource-provider.md`](../../docs/backlog/active/T-0120-gpu-resource-provider.md)
- **Roadmap Link:** [`docs/ROADMAP.md`](../../docs/ROADMAP.md)
- **Workflow Phase:** Phase 2 — Context Assembly (next gate owner: Specialist Engineer)

## 2. Problem Summary
- **Current behaviour:** GPU providers track buffer/texture allocations internally (e.g., OpenGL exposes `active_buffer_bytes()` / `active_texture_bytes()` accessors) but `IGpuResourceProvider` lacks a common API, preventing runtime diagnostics from reporting GPU memory usage.【F:engine/rendering/include/engine/rendering/backend/opengl/resource_provider.hpp†L65-L116】 Runtime metric generation currently emits lifecycle, streaming, and research telemetry without GPU resource gauges, so PM-510 demos cannot surface provider utilisation data.【F:engine/runtime/src/api.cpp†L962-L1104】
- **Desired behaviour:** Extend the provider interface to return a resource-usage snapshot that runtime code consumes to publish standard telemetry counters/gauges, aligning with T-0120’s telemetry expectations and roadmap risk mitigations.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L15-L37】【F:docs/ROADMAP.md†L64-L118】
- **Constraints / invariants:** Changes must preserve frame-graph contracts from ADR-0003, maintain compatibility with existing tests, and avoid regressing rendering module status noted in the module README.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】【F:docs/modules/rendering/README.md†L1-L13】
- **Quality budgets / telemetry notes:** Telemetry schema already enumerates rendering research metrics; new gauges should follow the documented naming/unit conventions so tooling continues to ingest runtime snapshots without schema drift.【F:docs/design/TELEMETRY_SCHEMA.md†L94-L127】

## 3. Key Artefacts
| Type | Location | Notes | Context Ladder Step |
| --- | --- | --- | --- |
| Module README | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Highlights GPU provider gap blocking rendering progress. | 5 |
| Module README | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Describes runtime telemetry responsibilities and RT-410 dependency. | 6 |
| ADR / Spec | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../docs/specs/ADR-0003-runtime-frame-graph.md) | Defines resource acquisition interface expectations. | 7 |
| Code excerpts | [`engine/rendering/include/engine/rendering/backend/opengl/resource_provider.hpp`](../../engine/rendering/include/engine/rendering/backend/opengl/resource_provider.hpp) | Provider tracks active bytes but only via backend-specific accessors. | — |
| Code excerpts | [`engine/runtime/src/api.cpp`](../../engine/runtime/src/api.cpp) | Metric snapshot omits GPU resource gauges. | — |
| Telemetry / Benchmarks | [`docs/design/TELEMETRY_SCHEMA.md`](../../docs/design/TELEMETRY_SCHEMA.md) | Runtime telemetry catalogue underpinning diagnostics tooling. | — |

## 4. Context Ladder Trace *(See [`AGENTS.md` §0.2](../../AGENTS.md#02-context-ladder))*
| Step | Document / Location | Insight / Decision | Owner | Follow-up |
| --- | --- | --- | --- | --- |
| 1 | [`README.md`](../../README.md) | Confirms GPU enablement is current top priority; sets quality workflow expectations.【F:README.md†L5-L144】 | Agent | None |
| 2 | [`docs/NAVIGATION.md`](../../docs/NAVIGATION.md) | Establishes documentation precedence and telemetry references for coordination.【F:docs/NAVIGATION.md†L1-L114】 | Agent | None |
| 3 | [`docs/ROADMAP.md`](../../docs/ROADMAP.md) | Phase 4 requires T-0120 telemetry outputs; informs risk owners.【F:docs/ROADMAP.md†L64-L118】 | Agent | None |
| 4 | [`docs/backlog/active/T-0120-gpu-resource-provider.md`](../../docs/backlog/active/T-0120-gpu-resource-provider.md) | Telemetry snapshots flagged as deliverable for provider work.【F:docs/backlog/active/T-0120-gpu-resource-provider.md†L15-L37】 | Agent | None |
| 5 | [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) | Rendering blocked until provider delivers GPU execution/telemetry.【F:docs/modules/rendering/README.md†L1-L13】 | Agent | None |
| 6 | [`docs/modules/runtime/README.md`](../../docs/modules/runtime/README.md) | Runtime telemetry must integrate with presentation flow once GPU work lands.【F:docs/modules/runtime/README.md†L1-L24】 | Agent | None |
| 7 | [`docs/specs/ADR-0003-runtime-frame-graph.md`](../../docs/specs/ADR-0003-runtime-frame-graph.md) | Confirms provider interface is the hook for telemetry extensions.【F:docs/specs/ADR-0003-runtime-frame-graph.md†L1-L24】 | Agent | Ensure API change documented |

## 5. Build, Validation & Telemetry Plan *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
- **Canonical command block copied:**
  ```bash
  cmake --preset linux-gcc-debug
  cmake --build --preset linux-gcc-debug
  ctest --preset linux-gcc-debug
  pytest python/tests scripts/tests
  python scripts/validate_docs.py
  ```
- **Additional presets / datasets:** None anticipated; unit coverage suffices.
- **Benchmark targets & expected deltas:** No performance benchmarks expected; telemetry gauges should add negligible overhead.
- **Tooling updates required:** Update runtime diagnostics documentation and ensure PM-510 telemetry scripts recognise new metric names if necessary.

## 6. Assumptions & Open Questions
| Question | Owner | Due Date | Resolution |
| --- | --- | --- | --- |
| Do existing telemetry consumers require schema change notice for new gauges? | Docs/DevRel | Before PR review | Runtime telemetry tests now enumerate the new GPU usage gauges without schema failures; existing consumers remain compatible.【cdf811†L23-L36】【db9233†L31-L61】 |

## 7. Attachments
- Diagrams: None.
- Data sets: Not required.
- Additional notes: Coordinate with PM-510 stakeholders if metric naming convention needs pre-approval.

