# Runtime Module

## Current State
- `RuntimeHost` orchestrates animation, compute-driven physics, geometry
  deformation, and submission into the rendering pipeline.
- Integrates with subsystem plugins discovered via core module facilities.
- Emits diagnostics and telemetry for lifecycle monitoring, including
  serialized frame-graph metadata and transient resource lifecycle events
  captured during render submissions.

## Usage
- Build with `cmake --build --preset <preset> --target engine_runtime`.
- Include `<engine/runtime/runtime_host.hpp>` for orchestration APIs.
- Run `ctest --preset <preset> --tests-regex engine_runtime`.

## TODO / Next Steps

- Track `RU-307`, `RU-315`, `RU-320` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — closes `RT-003` integration and feeds `AI-002`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RU-307` | Reconcile submission hooks with Vulkan backend (`RT-003`). | Unified submission struct validated by integration tests. | ✅ Done |
| `RU-315` | Expose streaming metrics to telemetry (`AI-002`). | Runtime publishes queue metrics consumed by diagnostics viewer. | 🟢 Todo |
| `RU-320` | Update runtime diagnostics guide. | Document lifecycle instrumentation and troubleshooting. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for detailed sequencing.
