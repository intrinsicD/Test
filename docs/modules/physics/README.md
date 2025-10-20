# Physics Module

## Current State
- Provides rigid-body simulation with mass clamping, damping, configurable
  sub-stepping, sphere/AABB/capsule colliders, and sweep-and-prune broad phase.
- Integrates with runtime scheduler via compute dispatcher.

## Usage
- Build with `cmake --build --preset <preset> --target engine_physics`.
- Include `<engine/physics/api.hpp>` for world configuration and simulation
  steps.
- Run `ctest --preset <preset> --tests-regex engine_physics`.

## TODO / Next Steps

- Track `PH-401`, `PH-410`, `PH-420` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — delivers `RT-002` goals.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `PH-401` | Implement persistent manifold storage (`RT-002`). | Cache retains contacts across frames with regression coverage. | 🔄 In Progress |
| `PH-410` | Add benchmarking harness. | Automated benchmark for collision throughput tracked in CI. | 🟢 Todo |
| `PH-420` | Wire telemetry for manifold churn. | Emit metrics and document consumption in diagnostics shell. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for full schedule.
