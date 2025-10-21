# Runtime Module

## Current State
- `RuntimeHost` orchestrates animation, compute-driven physics, CPU linear blend
  skinning, geometry deformation, and submission into the rendering pipeline in
  line with [ADR-0006](../../specs/ADR-0006-animation-deformation.md).
- Integrates with subsystem plugins discovered via core module facilities.
- Emits diagnostics and telemetry for lifecycle monitoring, including
  serialized frame-graph metadata and transient resource lifecycle events
  captured during render submissions.
- Rendering submissions consume the shared
  `engine::rendering::RuntimeSubmissionContext` struct via the
  `RuntimeHost::RenderSubmissionContext` alias so runtime APIs stay aligned with
  backend contracts (`RT-003.1`).
- Runtime diagnostics capture asynchronous streaming queue metrics mirrored via
  `scripts/diagnostics/runtime_frame_telemetry.py` for `AI-002` observability.
- Scene hierarchy validation reports are published through the diagnostics
  bridge so tooling and scripts receive detailed issue metadata (`RT-005.2`).
- Hierarchy troubleshooting workflows are documented in
  [diagnostics.md](diagnostics.md#hierarchy-diagnostics-playbook) so runtime and
  tooling consumers share a common remediation playbook (`RT-005.3`).
- Detailed instrumentation and troubleshooting workflows live in
  [diagnostics.md](diagnostics.md).
- Handle validation telemetry exposes `runtime.handles.*` counters populated by
  `engine::assets::validate_handle`, allowing diagnostics tooling to detect
  stale assets referenced by rendering submissions (`AI-001.2`).

## Usage
- Build with `cmake --build --preset <preset> --target engine_runtime`.
- Include `<engine/runtime/runtime_host.hpp>` for orchestration APIs.
- Run `ctest --preset <preset> --tests-regex engine_runtime`.
- Follow the [async streaming integration guide](async_streaming_integration.md)
  when wiring asset loading through the runtime and telemetry tooling (`AI-002.3`).

### Skinned Mesh Workflow

- Populate `RuntimeHostDependencies::mesh.rest_positions` and
  `RuntimeHostDependencies::binding` with matching vertex counts; the runtime
  rejects mismatched bindings via `RuntimeError::dependency_invalid_binding` to
  prevent undefined deformation results.【F:engine/runtime/src/api.cpp†L85-L134】
- Provide inverse bind matrices and normalised weights for every joint in the
  binding; `skinning::validate_binding` enforces these invariants on startup and
  every tick before dispatching deformation work.【F:engine/animation/include/engine/animation/rigging/rig_binding.hpp†L17-L116】【F:engine/runtime/src/api.cpp†L1102-L1170】
- During `RuntimeHost::tick` the dispatcher evaluates the animation pose,
  applies physics-driven root motion, builds global joint transforms, and runs
  `geometry::deform::apply_linear_blend_skinning` to update mesh positions and
  normals deterministically.【F:engine/runtime/src/api.cpp†L1075-L1182】【F:engine/geometry/src/deform/linear_blend_skinning.cpp†L16-L73】
- The resulting scene graph entries mirror joint transforms, allowing rendering
  and diagnostics tooling to inspect joint hierarchies alongside the skinned
  mesh.【F:engine/runtime/src/api.cpp†L949-L1101】
- Capture performance baselines with
  `python scripts/diagnostics/runtime_frame_telemetry.py --library-dir <build>/engine/runtime --frames 32 --variance-check geometry.deform:10 --variance-trim 0.1`.
  The default debug build records ~22 ms per `geometry.deform` dispatch using the
  cloth stress test mesh (trimmed mean over 32 frames).【ba1696†L1-L38】

## TODO / Next Steps

- Support the scene module follow-ups (`SC-225` diagnostics samples,
  `SC-230` alert thresholds) by providing telemetry examples and
  cross-linking new fixtures once they land in the shared tooling docs;
  these items extend `RT-005` and remain tracked in the
  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `RU-307` | Reconcile submission hooks with Vulkan backend (`RT-003`). | Unified submission struct validated by integration tests. | ✅ Done |
| `RU-315` | Expose streaming metrics to telemetry (`AI-002`). | Runtime publishes queue metrics consumed by diagnostics viewer. | ✅ Done |
| `RU-320` | Update runtime diagnostics guide. | Document lifecycle instrumentation and troubleshooting. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for detailed sequencing.
