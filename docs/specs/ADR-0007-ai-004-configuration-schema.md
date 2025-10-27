# ADR-0007: AI-004 Shared Configuration Schema

**Status**: Accepted

**Date**: 2025-12-05

**Authors**: Product Manager (@pm-agent), Chief Architect (@arch-agent)

---

## Context

`AI-004` aligns rendering (`RE-610`), runtime (`RT-320`), tools (`TL-210`), assets (`AS-330`), and benchmarking (`CC-310`) workstreams to deliver a reproducible prototyping workflow. Each stream currently maintains bespoke configuration fragments: rendering presets describe frame-graph variants, the runtime harness exposes scene manifests, the sandbox UI persists widget layouts, and dataset packages ship ingestion manifests. Without a documented schema, modules duplicate parsing logic and risk drift before the 2025-12-05 kickoff review.

**Constraints:**
- Performance budget: configuration parsing must remain ≤1 ms on harness startup to protect frame bootstrap latency.
- Platform requirements: schema must be consumable from native C++ (libyaml-cpp) and Python tooling (PyYAML) without diverging definitions.
- Compatibility: sustain existing prototype manifests while introducing migration hooks to avoid blocking ongoing feature branches.
- Technical debt: align with telemetry schema (`CC-001`) and dataset manifest format defined under `AS-330` to prevent redundant metadata stores.

**Related Documents:**
- Task: `AI-004`
- Task: `DC-040`
- Task: `RE-610`, `RT-320`, `TL-210`, `AS-330`, `CC-310`
- Epic/Roadmap: [`docs/ROADMAP.md#ai-004`](../ROADMAP.md)
- Related ADRs: `ADR-0003`

---

## Decision

Document and ratify a shared YAML-based configuration schema that captures:
- Dataset manifests selected by the prototyping harness and sandbox UI
- Rendering presets (forward/deferred variants, debug overlays)
- Runtime execution parameters (scene graph, simulation cadence)
- Benchmark automation directives (reference baselines, regression thresholds)

The schema will be versioned, validated via JSON Schema-compatible tooling, and published with migration guidance for existing manifests. Module owners will consume generated bindings to keep validation rules centralized.

**Chosen Approach:**
- Author a schema specification under `docs/design/AI-004-configuration-schema.md` describing sections, validation rules, and ownership responsibilities.
- Provide code generation or helper utilities so runtime, rendering, and tools share type-safe accessors.
- Register schema version in telemetry output to correlate benchmark artefacts with configuration revisions.

**Rationale:**
- **Pro 1**: Consolidates configuration validation, reducing drift between modules.
- **Pro 2**: Enables automation (CI smoke tests, benchmark orchestration) to enforce consistent experiment setups.
- **Con 1**: Requires short-term migration work for existing manifests.
- **Con 2**: Introduces dependency on schema tooling that must be maintained across languages.

**Alternatives Considered:**
1. **Module-specific configurations** → Rejected because coordination overhead remains high and risks schema divergence during integration.
2. **Runtime-owned configuration translation layer** → Rejected because it centralizes logic in runtime, hiding requirements from rendering/tools teams and complicating ownership.

---

## Consequences

### Positive
- Shared schema reduces integration defects across AI-004 workstreams.
- Schema versioning enables reproducible benchmarking and dataset ingestion workflows.

### Negative
- Teams must migrate existing manifests; temporary duplication may appear during transition.
- Schema validation errors could block harness startup if not surfaced gracefully; mitigated via staged rollout.

### Neutral
- Requires coordination cadence (bi-weekly) already planned for AI-004; schedule updates in existing demos.

---

## Implementation Details

### Interfaces
- Publish schema reference helpers under `docs/design/` and mirror generated headers in `engine/runtime/include/engine/runtime/config_schema.hpp` (TBD during implementation).
- Provide Python loader in `python/engine3g/config_schema.py` to keep tooling aligned.

### Data Layout
- YAML documents with top-level sections: `datasets`, `rendering`, `runtime`, `benchmarks`, `telemetry`.
- Use strongly typed enums for rendering presets; map to runtime capabilities to preserve determinism.

### Cross-Module Impact
- **Runtime**: consumes canonical configuration for harness bootstrap.
- **Rendering**: maps presets to frame-graph variants and debug overlays.
- **Tools**: sandbox UI surfaces schema-backed controls and persists per-user overlays.
- **Assets**: dataset ingestion registers manifest metadata referenced by configuration.
- **Scripts**: benchmarking automation reads schema to schedule runs and validations.

---

## Migration Plan

### Breaking Changes
- Consolidation of legacy runtime `config.yaml` keys under versioned schema.
- Deprecation of ad-hoc sandbox layout manifest sections once schema lands.

### Migration Steps
1. Publish schema draft and gather sign-off from module leads (`RE`, `RT`, `TL`, `AS`, `CC`).
2. Implement validators in runtime/tools Python utilities behind feature flag `AI004_SCHEMA_V1`.
3. Migrate existing manifests to new structure with conversion scripts.
4. Remove feature flag after CI validates new manifests across modules.

### Compatibility Strategy
- Deprecation timeline: preview in 2025-12 sprint; removal targeted for 2026-01 milestone.
- Feature flag: `ENGINE_AI004_SCHEMA_V1` toggles new parser.
- Fallback behavior: legacy parser remains available until migration completes; telemetry warns when legacy schema is used.

---

## Testing Strategy

### Test Coverage
- **Unit tests**: schema parsing/validation across runtime/tools/benchmark scripts.
- **Integration tests**: end-to-end harness launch using schema-driven manifests.
- **Performance tests**: measure startup parsing overhead on representative hardware.

### Invariants to Maintain
- Deterministic configuration ordering for reproducible hashing.
- Telemetry exports include schema version ID.

### Acceptance Criteria
- [ ] All tests pass on CI (Clang-22, MSVC)
- [ ] Performance regression ≤ 2%
- [ ] Documentation updated
- [ ] Migration guide published

---

## Hard Rules

- Maintain backward compatibility until all AI-004 deliverables switch to schema v1.
- Avoid module-specific schema forks; all modifications require cross-module review.
- Provide deterministic serialization to support configuration hashing in benchmark automation.

---

## References

- **Research papers**: TBD during schema authoring (expected to reference reproducible research guidelines)
- **External libraries**: [YAML Schema Specifications](https://yaml.org/spec/)
- **Benchmarks**: `CC-310` comparative benchmark plan
- **Prior art**: `docs/design/TELEMETRY_SCHEMA.md`

---

## Review History

| Date | Reviewer | Decision | Comments |
|------|----------|----------|----------|
| 2025-12-09 | Rendering Engineer (AI agent) | Accepted | Schema sections documented across rendering/runtime/benchmarks/telemetry with shared validator. |

