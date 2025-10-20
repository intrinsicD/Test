# Runtime Module

## Current State
- `RuntimeHost` orchestrates animation, compute-driven physics, geometry
  deformation, and submission into the rendering pipeline.
- Integrates with subsystem plugins discovered via core module facilities.
- Emits diagnostics and telemetry for lifecycle monitoring, including
  serialized frame-graph metadata and transient resource lifecycle events
  captured during render submissions.
- Runtime diagnostics capture asynchronous streaming queue metrics mirrored via
  `scripts/diagnostics/runtime_frame_telemetry.py` for `AI-002` observability.
- Detailed instrumentation and troubleshooting workflows live in
  [diagnostics.md](diagnostics.md).

## Usage
- Build with `cmake --build --preset <preset> --target engine_runtime`.
- Include `<engine/runtime/runtime_host.hpp>` for orchestration APIs.
- Run `ctest --preset <preset> --tests-regex engine_runtime`.

## TODO / Next Steps

- Track `RT-005.2` (runtime diagnostics bridge) in the [central roadmap](../../ROADMAP.md) and update the execution checklist below as hierarchy validation telemetry is wired into tooling.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RU-307` | Reconcile submission hooks with Vulkan backend (`RT-003`). | Unified submission struct validated by integration tests. | ✅ Done |
| `RU-315` | Expose streaming metrics to telemetry (`AI-002`). | Runtime publishes queue metrics consumed by diagnostics viewer. | ✅ Done |
| `RU-320` | Update runtime diagnostics guide. | Document lifecycle instrumentation and troubleshooting. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for detailed sequencing.
