# Documentation Entry Point

> Always start here before touching code, assets, or automation scripts.

## Session Checklist

1. **Workspace Snapshot** — skim [`../README.md`](../README.md) for the live
   module status table and sprint horizon.
2. **Role Guidance** — confirm expectations in [`agents.md`](agents.md) and the
   repository-wide [`../AGENTS.md`](../AGENTS.md).
3. **Architecture Context** — review [`architecture.md`](architecture.md) for
   invariants and data-flow constraints relevant to the subsystem you are
   modifying.
4. **Initiative Alignment** — locate the workstream in
   [`ROADMAP.md`](ROADMAP.md) and collect the matching module README/ROADMAP
   entries under [`modules/`](modules/).
5. **Actionable Task Record** — open the specific ticket under
   [`tasks/`](tasks/) (e.g., `T-0115-*.md`). Clarify acceptance criteria before
   implementing.
6. **Specification / ADR** — check [`specs/`](specs/) and
   [`design/`](design/) for historical decisions or pending proposals that
   constrain your change.
7. **Update Everything Together** — when landing work, update the code, module
   docs, roadmap status, and task checklists within the same change.

## Navigation Aids

| Purpose | Document |
| --- | --- |
| Agent-specific workflow and escalation paths | [agents.md](agents.md) |
| Invariants, lifecycles, and data flow diagrams | [architecture.md](architecture.md) |
| Coding, testing, documentation conventions | [conventions.md](conventions.md) |
| Implementation playbook prompts | [prompts/implementation-playbook.md](prompts/implementation-playbook.md) |
| Active task inventory & acceptance criteria | [tasks/README.md](tasks/README.md) |
| Decision records, ADRs, and specifications | [specs/README.md](specs/README.md) |
| Vulkan backend readiness checklist | [modules/rendering/backend_checklist.md](modules/rendering/backend_checklist.md) |
| Telemetry schema reference | [design/telemetry_schema.md](design/telemetry_schema.md) |

Source-of-truth precedence: `../AGENTS.md` → this file → `agents.md` →
`architecture.md` → entries under `design/` or `specs/` → module READMEs → code
comments. Resolve conflicts by updating the higher-precedence document first and
linking the change downstream.

## Keeping Documentation Healthy

- **Synchronise artefacts.** Whenever a behaviour, API, dependency, or workflow
  changes, update the module README, module roadmap, relevant tasks, and the
  summary tables in [`../README.md`](../README.md) and [`ROADMAP.md`](ROADMAP.md).
- **Validate cross-references.** Run `python scripts/validate_docs.py` after
  editing Markdown to catch broken links and stale anchors.
- **Track decisions explicitly.** Complex changes require an ADR or design note
  under [`design/`](design/). Reference those records in PR descriptions and
  module READMEs.
- **Use the template.** Apply [`README_TEMPLATE.md`](README_TEMPLATE.md) when
  creating new documentation to keep structure consistent for AI agents and
  humans.
- **Log uncertainties.** If a task lacks context, record open questions in the
  relevant `docs/tasks/*.md` file before requesting clarification.

## Additional Resources

- [`design/architecture_improvement_plan.md`](design/architecture_improvement_plan.md)
  – expanded rationale and dependency graph for the improvement plan.
- [`design/error_handling_migration.md`](design/error_handling_migration.md)
  – canonical reference for the `engine::Result<T>` policy.
- [`design/resource_management.md`](design/resource_management.md) – handle and
  resource pool design used by `AI-001`.
- [`modules/`](modules/) – subsystem overviews, execution checklists, and local
  TODO boards aligned with the roadmap.
- [`modules/runtime/async_streaming_integration.md`](modules/runtime/async_streaming_integration.md)
  – runtime integration checklist for async asset streaming (`AI-002.3`).

When in doubt, document the ambiguity before coding. This keeps the agentic
workflow deterministic and reviewable.
