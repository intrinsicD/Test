# Scene Module

## Current State
- Provides entity façade with hierarchy management, transform propagation,
  deterministic serialization/deserialization, and component helpers consumed by
  runtime systems.
- `SceneGraphValidator` exposes a fast cycle-detection check returning
  structured `SceneGraphErrorCode` values for diagnostics pipelines.

## Usage
- Build via `cmake --build --preset <preset> --target engine_scene`.
- Include `<engine/scene/scene_graph.hpp>` for hierarchy utilities.
- Include `<engine/scene/graph/scene_graph_validator.hpp>` for cycle detection checks.
- Run `ctest --preset <preset> --tests-regex engine_scene`.

## TODO / Next Steps

- Track `SC-220` in the [central roadmap](../../ROADMAP.md) and update the
  execution checklist below when status changes — captures ongoing diagnostics
  documentation work aligned with `RT-005`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `SC-208` | Implement cycle detection validation (`RT-005`). | Scene validator detects cycles with structured errors and tests. | ✅ Done |
| `SC-215` | Runtime diagnostics bridge. | Runtime reports invalid hierarchies via telemetry/logs. | ✅ Done |
| `SC-220` | Documentation refresh. | Update README + troubleshooting guide with validation workflows. | 🟢 Todo |

Review [ROADMAP.md](ROADMAP.md) for scheduling notes.
