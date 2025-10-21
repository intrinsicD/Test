# Animation Module

## Current State
- Provides skeletal animation primitives (`JointPose`, `AnimationClip`,
  `AnimationController`) and utilities for keyframe sampling, controller
  advancement, and blend-tree evaluation exposed in
  `<engine/animation/api.hpp>`.
- `RigBinding`, `RigJoint`, and `VertexBinding` define skeleton ↔ mesh binding
  contracts consumed by deformation pipelines.
- Skinning helpers (`skinning::build_global_joint_transforms`,
  `skinning::build_skinning_transforms`) evaluate rig poses into per-joint linear
  blend skinning transforms for downstream systems.
- JSON import/export helpers round-trip clips for offline tools and automated
  validation flows.
- `validate_clip` emits structured `ClipValidationErrorCode` values with
  descriptive messages enabling tooling to react precisely to failures.
- Blend-tree authoring covers clip/controller nodes, linear/additive blending,
  and parameter management (float, bool, event) so higher-level systems can
  express complex graphs.
- Module unit tests cover loading, serialization, blend tree behaviour, and feed
  into the integration harness at
  [`engine/tests/integration`](../../../engine/tests/integration/README.md).
- Regression suites exercise clip validation failure codes and controller
  playback invariants, guarding against regressions in authoring and runtime
  pipelines.

### Linear Blend Skinning Pipeline

- Rig bindings must populate `RigJoint::inverse_bind_pose` for every joint and
  keep vertex weights normalised (use `RigBinding::normalized` or
  `skinning::validate_binding` to confirm authoring tools exported consistent
  data). Runtime validation rejects bindings with missing joints or
  unnormalised weights before deformation begins.【F:engine/animation/include/engine/animation/rigging/rig_binding.hpp†L17-L116】【F:engine/runtime/src/api.cpp†L66-L118】
- Runtime evaluation follows [ADR-0006](../../specs/ADR-0006-animation-deformation.md):
  1. `skinning::build_global_joint_transforms` composes local joint poses with
     optional root translations from physics so the skeleton follows dynamic
     bodies deterministically.【F:engine/animation/src/deformation/linear_blend_skinning.cpp†L9-L65】
  2. `skinning::build_skinning_transforms` multiplies the global poses by the
     stored inverse bind matrices to yield per-joint deformation transforms.【F:engine/animation/src/deformation/linear_blend_skinning.cpp†L67-L96】
  3. Geometry consumes these transforms via
     `geometry::deform::apply_linear_blend_skinning` to update mesh positions
     and normals in-place every tick.【F:engine/geometry/src/deform/linear_blend_skinning.cpp†L11-L73】
- Author rig bindings with deterministic joint ordering and consistent naming;
  `RuntimeHostDependencies` expects the binding vertex count to match mesh rest
  positions and will emit `RuntimeError::dependency_invalid_binding` when the
  contract is violated.【F:engine/runtime/src/api.cpp†L85-L134】
- Use `python scripts/diagnostics/runtime_frame_telemetry.py` with the
  `geometry.deform` variance check to track per-frame skinning costs; baseline
  timings for the default cloth mesh average ~22 ms on the Linux GCC debug preset
  (32-frame sample, trimmed 10 % tails).【ba1696†L1-L38】

## Usage
- Build with `cmake --build --preset <preset> --target engine_animation` to
  expose headers under `engine/animation`.
- Link `engine_animation` (and `engine_math`) in downstream targets and include
  `<engine/animation/api.hpp>` to access sampling, validation, and blend-tree
  helpers.
- Execute `ctest --preset <preset> --tests-regex engine_animation` after
  configuring with `BUILD_TESTING=ON`.
- Use `<engine/animation/additive.hpp>` helpers to author additive blend nodes
  and parameter bindings. The additive input interpolates between identity and
  the additive pose before composing onto the base.

## TODO / Next Steps

- Track `AN-230` and `AN-240` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — aligns with `RT-001` milestones.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `AN-201` | Extend validation regression coverage (`RT-001`). | Add negative-path fixtures for `validate_clip`, extend controller regression tests, document results in module roadmap. | ✅ Done |
| `AN-230` | Prototype GPU/parallel sampling plan. | Document benchmarking methodology and publish roadmap references before executing harness work. | 🟡 In Progress |
| `AN-240` | Draft state-machine authoring spec. | Deliver authoring specification and update roadmap links before runtime implementation tasks begin. | 🟡 In Progress |

See [ROADMAP.md](ROADMAP.md) for phased delivery details and dependency
tracking.
