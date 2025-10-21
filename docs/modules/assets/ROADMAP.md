# Assets Module Roadmap

_Last Updated: 2025-02-19_

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
| `AS-305` | Harden async cancellation paths with regression coverage. | Assets | 2025-03-21 | 🟢 Todo |

## Upcoming (M3)

| Task ID | Description | Dependency |
| --- | --- | --- |
| `AS-315` | Integrate filesystem watcher callbacks for hot reload (`CC-002`). | `PL-222` watcher abstraction. |
| `AS-320` | Draft material persistence strategy and serialization format. | None |
| `AS-330` | Extend diagnostics shell to surface cache reload failures. | `TL-101` diagnostics MVP |

## Dependencies

- **IO module** — provides format handlers and signature validation (`RT-006`).
- **Platform module** — supplies filesystem watcher abstraction (`CC-002`).
- **Runtime module** — consumes telemetry emitted from async queue metrics.

## Notes

- Track detailed acceptance criteria in
  [`docs/tasks/T-0115-assets-async-streaming-mvp.md`](../../tasks/T-0115-assets-async-streaming-mvp.md).
- Coordinate hot reload work with the broader initiative documented in
  [`../../ROADMAP.md`](../../ROADMAP.md#cc-002-hot-reload-infrastructure).
- 2025-03-24: Published [`design/material_persistence_strategy.md`](../../design/material_persistence_strategy.md)
  completing `AS-320` and unblocking material cache implementation work.
