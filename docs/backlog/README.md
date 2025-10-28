# Backlog Overview

This directory holds the authoritative backlog for the Test Engine. Active work that contributes to making the engine production-ready lives under [`active/`](active/). Completed items move into [`archive/`](archive/) to keep the working set focused and searchable.

## Priority Scale

Priorities use a numeric scale where **1 is the highest urgency** and **5 is the lowest**:

| Priority | Meaning | Typical Handling |
| --- | --- | --- |
| 1 | Critical blocker for the application readiness goal | Staff immediately, unblock dependencies |
| 2 | Required for the current milestone | Plan in the next iteration |
| 3 | Supports milestone polish or robustness | Schedule after priority 1–2 work |
| 4 | Enhances diagnostics or automation | Slot when capacity allows |
| 5 | Nice-to-have or exploratory | Track for future consideration |

Each backlog item includes status, ownership, definition of done, dependencies, and linked artefacts so that any agent can execute without hunting for context.

## Workflow

1. **Create** a new backlog file in `active/` using the template below.
2. **Update** the central roadmap (`../ROADMAP.md`) to reference the new file.
3. **Deliver** the work and update status to `Complete` when acceptance criteria are met.
4. **Archive** the file by moving it into `archive/` and noting the completion date.

### Backlog Template

````markdown
# Backlog Item <ID> — <Title>

- **Status**: <Planned \| In Progress \| Complete>
- **Priority**: <1-5>
- **Owner**: <Role or person>
- **Module(s)**: <Relevant modules>
- **Goal**: <What changes when this is done>

## Summary
Concise description of the problem and the intended outcome.

## Definition of Done
- [ ] Criterion 1
- [ ] Criterion 2

## Dependencies
- <Link or "None">

## Related Artefacts
- <Specs, ADRs, code, datasets>

## Notes
Free-form notes, open questions, or decisions.
````

Keep backlog files short (≤200 lines) and link to deeper design docs when needed.
