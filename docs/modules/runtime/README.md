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
- Scene hierarchy validation reports are published through the diagnostics
  bridge so tooling and scripts receive detailed issue metadata (`RT-005.2`).
- Hierarchy troubleshooting workflows are documented in
  [diagnostics.md](diagnostics.md#hierarchy-diagnostics-playbook) so runtime and
  tooling consumers share a common remediation playbook (`RT-005.3`).
- Detailed instrumentation and troubleshooting workflows live in
  [diagnostics.md](diagnostics.md).

## Usage
- Build with `cmake --build --preset <preset> --target engine_runtime`.
- Include `<engine/runtime/runtime_host.hpp>` for orchestration APIs.
- Run `ctest --preset <preset> --tests-regex engine_runtime`.
- Follow the [async streaming integration guide](async_streaming_integration.md)
  when wiring asset loading through the runtime and telemetry tooling (`AI-002.3`).

## TODO / Next Steps

- Coordinate with the scene module documentation refresh (`SC-220`, see the
  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation)) so
  import pipelines reference the runtime troubleshooting guide when explaining
  hierarchy validation flows and keep `RT-005` artefacts aligned.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RU-307` | Reconcile submission hooks with Vulkan backend (`RT-003`). | Unified submission struct validated by integration tests. | ✅ Done |
| `RU-315` | Expose streaming metrics to telemetry (`AI-002`). | Runtime publishes queue metrics consumed by diagnostics viewer. | ✅ Done |
| `RU-320` | Update runtime diagnostics guide. | Document lifecycle instrumentation and troubleshooting. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for detailed sequencing.
