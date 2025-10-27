# Task Card: DC-040

## Title
AI-004 Configuration Schema Alignment

## Type
- [x] Feature
- [ ] Bug Fix
- [x] Refactor
- [x] Documentation
- [x] Research
- [ ] Performance Optimization

## Priority
- [x] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
8 person-days (distributed across rendering, runtime, tools, assets leads)

---

## Description

### Problem Statement
`AI-004` relies on a shared configuration schema that spans rendering presets, runtime harness options, sandbox UI controls, dataset manifests, and benchmark automation parameters. The roadmap review identified that no coordinated task exists to lock this schema ahead of the 2025-12-05 kickoff review, creating schedule risk and ambiguity across dependent workstreams.

### Proposed Solution
Stand up a cross-cutting task to author, review, and ratify the shared configuration schema. Produce a draft specification, gather module sign-off, wire validation hooks into runtime/tooling prototypes, and publish migration notes so downstream teams can implement against a stable contract.

### Success Criteria
- Schema specification drafted and reviewed by rendering (`RE`), runtime (`RT`), tools (`TL`), assets (`AS`), and benchmarking (`CC`) leads.
- Validators available in both C++ and Python harnesses behind a feature flag.
- Migration guidance published for existing manifests and sandbox layouts.
- Roadmap and README updated to reflect schema ownership, risks, and due dates.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::runtime`
- `engine::rendering`
- `engine::tools`
- `engine::assets`
- `scripts::diagnostics`
- `python::engine3g`

**Files to Modify:**
- `docs/design/AI-004-configuration-schema.md` (new)
- `docs/ROADMAP.md`
- `docs/tasks/AI-004-application-prototyping-enablement.md`
- `docs/tasks/TL-210-experiment-sandbox-ui.md`
- `docs/tasks/AS-330-reference-dataset-packages.md`
- `docs/tasks/RT-320-runtime-prototyping-harness.md`
- `docs/tasks/RE-610-research-rendering-baseline.md`
- `docs/tasks/CC-310-comparative-benchmark-automation.md`

**New Files:**
- `docs/specs/ADR-0007-ai-004-configuration-schema.md`
- `docs/design/AI-004-configuration-schema.md`

### Dependencies
**Depends On:**
- `AI-001`, `AI-002`, `AI-003` groundwork already complete (telemetry + resource consistency)
- Access to existing manifests from runtime and tools prototypes

**Blocks:**
- `RE-610`, `RT-320`, `TL-210`, `AS-330`, `CC-310` milestone acceptance

### Related Work
- Roadmap review dated 2025-12-05
- `docs/specs/ADR-0003-runtime-frame-graph.md`
- `docs/tasks/AI-004-application-prototyping-enablement.md`

---

## Acceptance Criteria

### Functional Requirements
- [x] Publish schema draft covering datasets, rendering presets, runtime execution, sandbox layout bindings, and benchmark automation.
- [ ] Hold cross-module review meeting with sign-off recorded in ADR review history.
- [ ] Integrate schema validation checks into prototyping harness startup (behind feature flag).
- [ ] Document migration steps for legacy manifests in module READMEs.

### Non-Functional Requirements
- [ ] Schema parsing adds ≤1 ms overhead to harness startup (measured on reference hardware).
- [ ] Schema tooling consumable from Python and C++ without duplicated definitions.

### Testing Requirements
- [ ] Add unit tests for schema validation utilities (runtime + Python bindings).
- [ ] Add integration smoke test that loads schema-driven configuration in CI.
- [ ] Coverage ≥ 85% on new validation code.

### Documentation Requirements
- [ ] ADR published and linked from roadmap + task cards.
- [ ] README risk register updated with owner/due dates.
- [ ] Prototyping playbook updated with schema overview.

---

## Test Plan

### Unit Tests
```cpp
TEST(PrototypeConfig, ValidSchemaLoads) {
    auto config = LoadPrototypeConfig("fixtures/ai004_schema_v1.yaml");
    EXPECT_TRUE(config.HasDataset("reference"));
    EXPECT_EQ(config.rendering.presets.size(), 2);
}
```

### Integration Tests
- Launch runtime harness with schema-enabled configuration and verify sandbox UI enumerates datasets and presets.
- Run comparative benchmark automation using schema-defined runs and confirm telemetry export references schema version.

### Performance Tests
- Measure schema parsing time under Tracy instrumentation; ensure ≤1 ms budget is respected.

---

## Implementation Notes

### Design Considerations
- Align schema naming with telemetry schema (`CC-001`) to reuse dashboards.
- Provide version field and compatibility table to support future schema evolutions.
- Publish dataset manifest validator in `python/engine3g/config_schema.py` so geometry tooling and dataset packaging pipelines
  receive immediate feedback before broader schema adoption.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Schema draft stalls awaiting consensus | Medium | High | Timebox review cycle; escalate to Chief Architect after 2 business days.
| Tooling divergence between C++ and Python | Medium | Medium | Share canonical schema definition and generate bindings from single source.
| Performance regression at startup | Low | Medium | Profile early and cache parsed schema in runtime harness.

### Alternative Approaches
1. **Ad-hoc coordination via individual tasks** → rejected; lacks centralized accountability and misses kickoff deadline.
2. **Postpone schema until after module deliverables** → rejected; would delay integration demos and duplicate work.

---

## Deliverables

- [ ] Schema specification + ADR
- [ ] Validation tooling (runtime + Python)
- [ ] Updated roadmap and risk register
- [ ] Migration guide across affected modules
- [ ] Demo/notes confirming schema adoption in sandbox + harness

---

## Definition of Done

- [ ] Builds/tests updated to include schema validation
- [ ] Kickoff review materials include schema summary slide
- [ ] Risks in roadmap marked with owner/due dates
- [ ] Follow-up issues logged for post-v1 enhancements (if required)

---

## Assigned To
**Role**: Product Manager coordinating with Chief Architect + module leads
**Name**: @pm-agent

## Estimated Timeline
**Start Date**: 2025-11-29
**Target Completion**: 2025-12-10
**Actual Completion**: _TBD_

---

## Notes
- Schedule schema review alongside AI-004 integration demos.
- Ensure dataset licensing constraints from `AS-330` are represented in schema metadata.
- 2025-12-09: Rendering/runtime/benchmark/telemetry schema tables published and Python validator extended to parse full
  configuration manifests (`python/engine3g/config_schema.py`).
- 2025-12-10: Added `scripts/validate_ai004_config.py` CLI + regression tests so CI and local workflows can validate manifests
  without writing bespoke harness code, unblocking DC-040 validation deliverable work.
- 2025-12-12: `ENGINE_AI004_SCHEMA_V1` gate added to harness loaders so teams can opt-in to strict schema validation while
  legacy manifests remain operational during migration.
