# Core Module Roadmap

_Last Updated: 2025-03-28_

## Priorities

| Priority | Description | Status |
| --- | --- | --- |
| Diagnostics bridge (`CR-118`) | Define telemetry routing to support `CC-001`. | ✅ Done |
| Plugin lifecycle audit (`CR-125`) | Validate `DC-001` invariants remain intact. | ✅ Done |
| Configuration docs refresh (`CR-130`) | Keep configuration APIs discoverable. | ✅ Done |
| Subsystem dependency diagnostics (`CR-135`) | Detect plugin dependency cycles and document recovery steps. | ✅ Done |
| Structured logging follow-up (`CR-136`) | Capture subsystem initialization failure context in telemetry/logs. | 🟡 Planned |

## Active Work

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `CR-118` | Core team | 2025-03-07 | ✅ Done |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| `CR-135` | Add dependency cycle detection and lifecycle telemetry guidance. | ✅ Delivered 2025-03-30 |
| `CR-136` | Document structured logging for subsystem initialization failures. | After `CR-135` |

## Notes

- Coordinate with Tools module to ensure diagnostics bridge supports viewer use
  cases.
- Reflect lifecycle updates in task records and the central roadmap when
  complete.
- Telemetry schema lives in [design/telemetry_schema.md](../../design/telemetry_schema.md);
  keep the document updated as additional modules emit metrics.
