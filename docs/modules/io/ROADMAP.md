# IO Module Roadmap

_Last Updated: 2025-03-24_

## Goals

| Goal | Description | Status |
| --- | --- | --- |
| Signature hardening (`IO-221`) | Build signature DB and fuzz harness coverage. | 🔄 In Progress |
| Error catalog (`IO-230`) | Document structured errors for tooling. | ✅ Done |
| Telemetry alignment (`IO-240`) | Surface import/export failures via diagnostics. | ✅ Done |

## Active Tasks

| Task ID | Owner | Due | Status |
| --- | --- | --- | --- |
| `IO-221` | IO team | 2025-03-14 | 🔄 In Progress |

## Upcoming

| Task ID | Description | Dependency |
| --- | --- | --- |
| — | — | — |

## Completed

| Task ID | Description | Completion Notes |
| --- | --- | --- |
| `IO-230` | Publish structured error catalog referencing `DC-004`. | Catalog lives in `docs/modules/io/README.md#error-catalog` and feeds diagnostics tooling. |
| `IO-240` | Align geometry import/export telemetry with runtime diagnostics bridge. | Metrics available under `io.geometry.*` counters; failure logs include file provenance for operators. |

## Notes

- Coordinate fuzz harness resource usage with CI owners before enabling.
- Detection & fuzzing playbook published (`RT-006.3`); keep it synced when
  catalogue or harness steps change.
- Update task records (`T-0112`) and central roadmap when milestones move.
- Consume the shared telemetry schema when extending telemetry coverage beyond geometry import/export pipelines.
