# Backlog Item DC-040 — AI-004 Configuration Schema Alignment

- **Status**: Complete
- **Priority**: 1
- **Owner**: Agent Orchestrator + Module Leads
- **Module(s)**: Runtime, Rendering, Tools, Assets, Benchmarking
- **Goal**: Establish a shared configuration contract so every AI-004 component consumes the same schema.

## Summary
AI-004 spans rendering presets, runtime harness controls, sandbox layouts, dataset manifests, and benchmark automation. Without a ratified configuration schema the teams ship incompatible knobs and the kickoff review stalls. This backlog item drives a single, versioned schema with validators in both C++ and Python so downstream work can integrate against a stable interface.

## Role Roster
| Role | Responsibilities | Owner |
| --- | --- | --- |
| Agent Orchestrator | Schedule schema review cadence, capture module approvals in the task brief. | Agent Orchestrator (11) |
| Product Manager | Maintain roadmap alignment, confirm Definition of Done remains representative of kickoff needs. | Product Manager (10) |
| Knowledge Librarian | Curate ADR-0007, design notes, and historical schema context; ensure citations stay current. | Knowledge Librarian (12) |
| Specialist Engineer(s) | Runtime, tools, and assets leads implement validators, loaders, and integration tests. | Runtime Lead; Tools Lead; Assets Lead |
| Docs/DevRel | Update prototyping playbook, module READMEs, and NAVIGATION pointers for the shared schema. | Docs/DevRel (95) |
| QA/Test Specialist | Expand C++/Python validator coverage, record execution logs in the quality report. | QA/Test Specialist (90) |
| Performance Engineer | Review telemetry changes for overhead, ensure schema instrumentation meets budgets. | Performance Engineer (80) |
| Safety Reviewer | Audit config parsing for injection/sandboxing risks; verify dependency diffs. | Safety Reviewer (15) |
| Reviewer | Apply CONTRIBUTION.md review checklist across cross-module diffs. | Reviewer (99) |
| Release Manager | Tag schema version updates and publish release notes for downstream teams. | Release Manager (98) |

## Definition of Done
- [x] Schema specification published with module sign-off recorded in ADR-0007.
- [x] Runtime harness and Python tooling load and validate configurations via the shared schema (feature-flagged until adoption completes).
- [x] Migration guidance added to affected module READMEs and the prototyping playbook.
- [x] Roadmap and risk register reference the schema owner and version.

## Dependencies
- AI-001/AI-002/AI-003 groundwork completed (resource + telemetry invariants).

## Related Artefacts
- [`docs/specs/ADR_0007_AI_004_CONFIGURATION_SCHEMA.md`](../../specs/ADR_0007_AI_004_CONFIGURATION_SCHEMA.md)
- [`docs/design/AI_004_CONFIGURATION_SCHEMA.md`](../../design/AI_004_CONFIGURATION_SCHEMA.md)
- Legacy context: [`docs/archive/backlog/legacy/tasks/DC_040_AI_004_CONFIGURATION_SCHEMA_ALIGNMENT.md`](../../archive/backlog/legacy/tasks/DC_040_AI_004_CONFIGURATION_SCHEMA_ALIGNMENT.md)

## Notes
Kickoff readiness (`DC-041`) depends on this task closing; escalate blockers within two business days.
- Python stubs for the schema validators (`engine3g.config_schema.pyi`) now mirror the dataclass surface so IDEs and CI type
  checks can rely on the shared contract while runtime integration proceeds.
- Cross-references now validate dataset slugs across runtime and benchmark sections, rejecting manifests that point at missing
  or duplicated dataset identifiers.
- Native runtime loader (`engine::runtime::config::load_configuration`) now ships in
  [`engine/runtime/config_schema.hpp`](../../../engine/runtime/include/engine/runtime/config_schema.hpp) backed by `yaml-cpp`
  with regression tests under
  [`engine/runtime/tests/test_config_schema.cpp`](../../../engine/runtime/tests/test_config_schema.cpp).
- The AI-004 prototyping workflow is documented in
  [`docs/design/AI_004_PROTOTYPING_PLAYBOOK.md`](../../design/AI_004_PROTOTYPING_PLAYBOOK.md), providing migration guidance for
  runtime, tools, assets, and benchmarking teams.
