# Backlog Item DC-040 — AI-004 Configuration Schema Alignment

- **Status**: In Progress
- **Priority**: 1
- **Owner**: Agent Orchestrator + Module Leads
- **Module(s)**: Runtime, Rendering, Tools, Assets, Benchmarking
- **Goal**: Establish a shared configuration contract so every AI-004 component consumes the same schema.

## Summary
AI-004 spans rendering presets, runtime harness controls, sandbox layouts, dataset manifests, and benchmark automation. Without a ratified configuration schema the teams ship incompatible knobs and the kickoff review stalls. This backlog item drives a single, versioned schema with validators in both C++ and Python so downstream work can integrate against a stable interface.

## Definition of Done
- [ ] Schema specification published with module sign-off recorded in ADR-0007.
- [ ] Runtime harness and Python tooling load and validate configurations via the shared schema (feature-flagged until adoption completes).
- [ ] Migration guidance added to affected module READMEs and the prototyping playbook.
- [ ] Roadmap and risk register reference the schema owner and version.

## Dependencies
- AI-001/AI-002/AI-003 groundwork completed (resource + telemetry invariants).

## Related Artefacts
- [`docs/specs/ADR-0007-ai-004-configuration-schema.md`](../../specs/ADR-0007-ai-004-configuration-schema.md)
- [`docs/design/AI-004-configuration-schema.md`](../../design/AI-004-configuration-schema.md)
- Legacy context: [`docs/archive/backlog/legacy/tasks/DC-040-ai-004-configuration-schema-alignment.md`](../../archive/backlog/legacy/tasks/DC-040-ai-004-configuration-schema-alignment.md)

## Notes
Kickoff readiness (`DC-041`) depends on this task closing; escalate blockers within two business days.
- Python stubs for the schema validators (`engine3g.config_schema.pyi`) now mirror the dataclass surface so IDEs and CI type
  checks can rely on the shared contract while runtime integration proceeds.
- Cross-references now validate dataset slugs across runtime and benchmark sections, rejecting manifests that point at missing
  or duplicated dataset identifiers.
