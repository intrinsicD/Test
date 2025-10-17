# ADR-0006: Runtime Linear Blend Skinning Pipeline

- **Status:** Accepted
- **Drivers:** `RT-001`
- **Authors:** Runtime & Animation Working Group
- **Last Updated:** 2025-02-20

## Context

The runtime previously translated animated rigs into renderable meshes by
applying a uniform translation derived from the root joint and the physics
proxy. This shortcut ignored vertex-level skinning weights, preventing the
runtime from honouring deformation authored for skinned meshes. The roadmap
item `RT-001` requires deterministic linear blend skinning (LBS) coverage
with documented workflows and regression tests spanning animation,
geometry, and runtime.

## Decision

1. **Rig Bindings Own Inverse Bind Poses:** `RigJoint` records the inverse
   bind transform for each joint. Bindings are validated to ensure vertex
   influences reference known joints with normalised weights before they
   are consumed by the runtime.
2. **Animation Supplies Skinning Transforms:** The animation module exposes
   `skinning::build_global_joint_transforms` and
   `skinning::build_skinning_transforms` helpers that evaluate a rig pose
   into global joint transforms and the corresponding skinning matrices,
   optionally offset by external translations such as physics-driven body
   motion.
3. **Geometry Applies LBS:** A new helper under
   `engine/geometry/deform/linear_blend_skinning.hpp` consumes the rig
   binding and per-joint skinning transforms to deform `SurfaceMesh`
   vertices, updating normals and bounds in-place while gracefully falling
   back to rest positions when influences are missing.
4. **Runtime Integrates the Pipeline:** `RuntimeHost` keeps the rig binding
   and reusable transform scratch buffers alongside its mesh. During the
   `geometry.deform` kernel it validates the binding, evaluates the
   skinning transforms, and delegates deformation to the geometry helper.
   When bindings are absent it falls back to the previous uniform
   translation path.

## Consequences

- Runtime ticks now publish skinned vertex positions that match the active
  animation pose, enabling rendering and downstream systems to operate on
  deformed geometry.
- Validation hooks detect malformed bindings early, avoiding undefined
  behaviour when skinning data is incomplete.
- Geometry assumes a dependency on the animation module to access binding
  structures, and runtime allocations include persistent scratch buffers
  for joint transforms.

## Follow-Up Work

- Extend telemetry scripts to surface per-frame skinning timings in
  automated dashboards.
- Document authoring workflows for blend shapes and GPU skinning variants
  as future roadmap items build on top of the CPU pipeline.
