# Tools Module Roadmap

_Last Updated: 2025-04-05_

## Roadmap

| Task ID | Description | Status |
| --- | --- | --- |
| `TL-101` | Implement diagnostics shell to visualise telemetry (`CC-001`). | ✅ Done |
| `TL-110` | Document tooling usage + troubleshooting. | ✅ Done |
| `TL-115` | Add profiling capture/export workflow. | ✅ Done |

## Plan

| Sprint | Tasks | Notes |
| --- | --- | --- |
| Sprint 2 | `TL-101` | Telemetry schema (`CC-001.1`) landed; begin viewer implementation. |
| Sprint 3 | `TL-110`, `TL-115` | Update docs/tests alongside feature work. |

## Notes

- 2025-04-05: Profiling capture export emits Chrome trace payloads via
  `--profile-trace`, satisfying TL-115 with regression coverage in
  `test_runtime_frame_telemetry.py`.
- 2025-03-26: README now documents `runtime_frame_telemetry.py` prefix filters
  and `--metrics-all`, partially satisfying `TL-110` while additional
  troubleshooting guidance remains pending.
- 2025-04-05: Completed TL-110 by documenting runtime library discovery,
  variance-tuning, and log capture workflows for telemetry tooling.

Coordinate with Core (`CR-118`) for telemetry bridge requirements.
