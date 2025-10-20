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
| `AN-230` | Prototype GPU/parallel sampling plan. | Bench sampling throughput via `compute::KernelDispatcher` and publish findings in module roadmap. | 🟢 Todo |
| `AN-240` | Draft state-machine authoring spec. | Author proposal covering transition orchestration and event propagation; link in `docs/design/`. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for phased delivery details and dependency
tracking.
