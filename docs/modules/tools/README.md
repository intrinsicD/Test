# Tools Module

## Current State
- Staging area for editor, profiling, and pipeline automation features with
  roadmap-driven scaffolding.
- Houses initial diagnostics scripting hooks, including the telemetry viewer
  CLI that renders runtime snapshots exported by
  `scripts/diagnostics/runtime_frame_telemetry.py`.
- Shares the cross-module telemetry instrumentation guidance captured in
  [`docs/design/telemetry_instrumentation_guide.md`](../../design/telemetry_instrumentation_guide.md).

## Usage
- Build with `cmake --build --preset <preset> --target engine_tools`.
- Run Python tooling under `python/tools/` and scripts in `scripts/` as
  documented by specific utilities.
- Inspect telemetry snapshots with
  ``python scripts/diagnostics/telemetry_viewer.py --input <path>`` and adjust
  `--metric-prefix` to focus on specific metric namespaces.

## TODO / Next Steps

- Track `TL-101`, `TL-110`, `TL-115` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — required for `CC-001` viewer work.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `TL-101` | Stand up diagnostics shell MVP (`CC-001`). | CLI/UI viewer renders telemetry, smoke tests documented. | ✅ Done |
| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | 🟢 Todo |
| `TL-115` | Profiling capture export. | Implement export path with regression coverage. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for sequencing.
