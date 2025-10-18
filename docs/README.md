# Documentation Entry Point

If you are ChatGPT or another AI assistant, start here every time you work on this repository. Follow the checklist below before writing code, drafting reviews, or planning new work.

0. **Review [`../README.md`](../README.md) for the current subsystem snapshot and build/test workflow.**
1. **Read [`docs/agents.md`](agents.md).** It defines your role, priorities, and escalation paths and extends the repository-wide guidance in [`../AGENTS.md`](../AGENTS.md).
2. **Read [`docs/architecture.md`](architecture.md).** Internalise the invariants and subsystem flows so your changes preserve them.
3. **For each task, open the matching record under [`docs/tasks/`](tasks/).** If there is no record, ask for the precise path you need.
4. **Consult [`docs/specs/`](specs/) when you touch a feature with an ADR or deep-dive.** They are the canonical source of technical decisions.
5. **Adhere to [`docs/conventions.md`](conventions.md) and cross-check definitions in [`docs/glossary.md`](glossary.md).**

## Quick Navigation

| Purpose | Document |
| --- | --- |
| Working agreement & guardrails | [docs/agents.md](agents.md) |
| System invariants & data flow | [docs/architecture.md](architecture.md) |
| Coding, testing, and docs rules | [docs/conventions.md](conventions.md) |
| Implementation & self-review workflow | [docs/prompts/implementation-playbook.md](prompts/implementation-playbook.md) |
| Task backlog & acceptance criteria | [docs/tasks/README.md](tasks/README.md) |
| Decision records & specifications | [docs/specs/README.md](specs/README.md) |

Source of truth precedence: **`../AGENTS.md` → this file → `architecture.md` → `specs/*` → module READMEs (`docs/modules/**`) → code comments.** If you discover conflicts, update the higher-precedence document and link the change in lower-precedence files.

## How to Work with This Repository

- **Stay aligned with the architecture improvement plan.** Every change touching system design must reference [`docs/ROADMAP.md`](ROADMAP.md) and the relevant `DC-`, `AI-`, or `RT-` identifier. Use [`docs/design/architecture_improvement_plan.md`](design/architecture_improvement_plan.md) for extended rationale when priorities shift.
- **Keep module documentation in sync.** Each subsystem has a README and roadmap under [`docs/modules/`](modules/). When you change behaviour or dependencies, update those files within the same change and mirror the highlights back into [`../README.md`](../README.md).
- **Validate documentation cross-references.** Run `python scripts/validate_docs.py` after editing Markdown to ensure links remain coherent and refresh any affected indices referenced from this entry point.

## Additional Resources

- [`docs/README_TEMPLATE.md`](README_TEMPLATE.md) standardises module documentation.
- [`docs/design/architecture_improvement_plan.md`](design/architecture_improvement_plan.md) expands on the roadmap and decision history.
- [`docs/design/resource_management.md`](design/resource_management.md) documents the generational handle and resource pool infrastructure backing asset caches.
- [`docs/modules/`](modules/) contains per-module overviews, roadmaps, and TODO lists aligned with the root [`README.md`](../README.md).
- [`docs/design/error_handling_migration.md`](design/error_handling_migration.md) documents the `engine::Result`/`ErrorCode`
  pattern rolled out by `DC-004`.

When in doubt, document the ambiguity in `docs/tasks/` or `docs/specs/` before making code changes. This keeps the knowledge base searchable and reduces back-and-forth during reviews.
