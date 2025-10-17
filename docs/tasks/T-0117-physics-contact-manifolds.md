# T-0117: Physics Contact Manifolds Persistence & Telemetry

## Goal
Complete roadmap item `RT-002` by introducing persistent contact manifolds, solver
callback plumbing, and collision telemetry for the physics module.

## Inputs
- Code: `engine/physics/include/engine/physics/api.hpp`,
  `engine/physics/src/collisions.cpp`, `engine/physics/tests/test_module.cpp`.
- Docs: [`docs/modules/physics/README.md`](../modules/physics/README.md),
  [`docs/modules/physics/ROADMAP.md`](../modules/physics/ROADMAP.md),
  [`docs/ROADMAP.md`](../ROADMAP.md#rt-002-physics-contact-manifolds).

## Constraints
- Preserve deterministic ordering of manifolds frame-to-frame.
- Avoid introducing additional per-contact heap allocations in hot paths.
- Keep collision detection compatibility with existing sphere/AABB/capsule support.
- Surface diagnostics without requiring optional GPU backends or platform features.

## Deliverables
1. Persistent `ContactManifold` data stored on `PhysicsWorld` with lifetime tracking.
2. Constraint solver callback registration and invocation hooks.
3. Collision telemetry exposing manifold/contact counts and peak penetration.
4. Regression tests covering manifold persistence, telemetry, and callback execution.
5. Documentation updates aligning module and central roadmaps with the new behaviour.

## Checklist
- [x] Persistent manifolds maintain lifetime counters across consecutive frames.
- [x] Telemetry reports manifold/contact counts and maximum penetration depth.
- [x] Solver callbacks trigger deterministically for each active manifold.
- [x] Physics module README and roadmap updated; central roadmap reflects completion of `RT-002`.
- [x] Regression tests added in `engine/physics/tests/test_module.cpp`.

## Metrics & Benchmarks
- `ctest --preset linux-gcc-debug --tests-regex engine_physics` validates the
  updated physics suite, including the new manifold persistence and callback tests.

## Follow-Up Tasks
- [ ] Implement impulse-based constraint resolution using the registered
      callbacks and extend telemetry with solver iteration counts (`RT-002-FU1`).
- [ ] Expose manifold sampling utilities for editor visualisation workflows.
