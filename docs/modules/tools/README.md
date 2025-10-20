# Tools Module

## Current State
- Staging area for editor, profiling, and pipeline automation features with
  roadmap-driven scaffolding.
- Houses initial diagnostics scripting hooks.

## Usage
- Build with `cmake --build --preset <preset> --target engine_tools`.
- Run Python tooling under `python/tools/` and scripts in `scripts/` as
  documented by specific utilities.

## TODO / Next Steps

- Track `TL-101`, `TL-110`, `TL-115` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — required for `CC-001` viewer work.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `TL-101` | Stand up diagnostics shell MVP (`CC-001`). | CLI/UI viewer renders telemetry, smoke tests documented. | 🟢 Todo |
| `TL-110` | Document tooling invocation. | Update README with commands, environment requirements, and troubleshooting. | 🟢 Todo |
| `TL-115` | Profiling capture export. | Implement export path with regression coverage. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for sequencing.
