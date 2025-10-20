# Scene Module

## Current State
- Provides entity façade with hierarchy management, transform propagation,
  deterministic serialization/deserialization, and component helpers consumed by
  runtime systems.

## Usage
- Build via `cmake --build --preset <preset> --target engine_scene`.
- Include `<engine/scene/scene_graph.hpp>` for hierarchy utilities.
- Run `ctest --preset <preset> --tests-regex engine_scene`.

## TODO / Next Steps

- Track `SC-208`, `SC-215`, `SC-220` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — unlocks `RT-005` diagnostics.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `SC-208` | Implement cycle detection validation (`RT-005`). | Scene validator detects cycles with structured errors and tests. | 🟢 Todo |
| `SC-215` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies via telemetry/logs. | 🟢 Todo |
| `SC-220` | Documentation refresh. | Update README + troubleshooting guide with validation workflows. | 🟢 Todo |

Review [ROADMAP.md](ROADMAP.md) for scheduling notes.
