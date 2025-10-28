# Assets Module Roadmap

_Last Updated: 2025-05-05_

## Milestone Overview

| Milestone | Focus | Status |
| --- | --- | --- |
| M1 – Handle foundation | Generational handles, cache lifecycle, tests. | ✅ Complete |
| M2 – Async infrastructure | Async queue scaffolding, runtime integration hooks. | 🔄 In Progress |
| M3 – Authoring & hot reload | Material persistence, filesystem watching, diagnostics. | 🟢 Planned |

## Active Tasks (M2)

| Task ID | Description | Owner | Due | Status |
| --- | --- | --- | --- | --- |
| `AS-302` | Emit async queue telemetry and update diagnostics tooling. | Assets + Runtime | 2025-03-14 | ✅ Done |
| `AS-305` | Harden async cancellation paths with regression coverage. | Assets | 2025-03-21 | ✅ Done |

## Upcoming (M3)

| Task ID | Description | Dependency |
| --- | --- | --- |
| _TBD_ | Next milestone under evaluation. | – |

## Dependencies

- **IO module** — provides format handlers and signature validation (`RT-006`).
- **Platform module** — supplies filesystem watcher abstraction (`CC-002`).
- **Runtime module** — consumes telemetry emitted from async queue metrics.

## Notes

- Track detailed acceptance criteria in
  [`docs/archive/backlog/legacy/tasks/T-0115-assets-async-streaming-mvp.md`](../../archive/backlog/legacy/tasks/T-0115-assets-async-streaming-mvp.md).
- Coordinate hot reload work with the broader initiative documented in
  [`../../ROADMAP.md`](../../ROADMAP.md#cc-002-hot-reload-infrastructure).
- 2025-03-24: Published [`design/MATERIAL_PERSISTENCE_STRATEGY.md`](../../design/MATERIAL_PERSISTENCE_STRATEGY.md)
  completing `AS-320` and unblocking material cache implementation work.
- Diagnostics shell now enumerates recent asset reload failures with hints,
  completing `AS-330` and providing actionable triage data alongside the
  existing hot-reload telemetry.
- 2025-05-05: Asset async queue cancellation checks guard geometry detection and
  decode hand-offs, and tests verify cancellation telemetry remains
  deterministic (`AS-305`).
- 2025-05-24: Hot-reload watcher callbacks feed runtime telemetry and the
  diagnostics streaming report, completing `AS-315` and surfacing recent
  failures for tooling consumers.
- 2025-10-24: Asset streaming report outputs a textual dashboard and Chrome
  trace counters (`TL-120`), enabling CI dashboards to track reload failures and
  queue utilisation alongside runtime telemetry.
