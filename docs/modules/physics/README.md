# Physics Module

## Current State
- Defines rigid-body representations (`PhysicsWorld`, `RigidBody`) with configurable gravity, damping, and substepping controls, plus collider authoring helpers for spheres, AABBs, and capsules.
- Implements force integration, collider assignment, and collision detection leveraging geometry intersection utilities.
- Maintains persistent contact manifolds with per-frame telemetry and optional solver callbacks to drive downstream constraint
  integration.
- Exposes module metadata via `module_name()` and integrates with runtime orchestration.
- Tests in `engine/physics/tests/` cover module registration, force integration,
  damping/substepping, collider management, and direct collision detection
  scenarios across sphere/AABB/capsule pairs, while the runtime integration
  suite in [`engine/tests/integration`](../../../engine/tests/integration/README.md)
  ensures physics forces couple correctly with animation-driven inputs (`TI-001`),
  now backed by the Googletest fixture upgrade delivered in `T-0118`.

## Usage
- Build using `cmake --build --preset <preset> --target engine_physics`; this links against `engine_math` and `engine_geometry`.
- Include `<engine/physics/api.hpp>` to construct worlds, bodies, apply forces, and query collision pairs.
- Run `ctest --preset <preset> --tests-regex engine_physics` with testing enabled to guard entry points; supplement with scenario tests in runtime when modifying collision code.
- `PhysicsWorld` defaults to single-step integration; call `set_substepping()` to enable fixed-step refinement when stability demands it.
- After advancing simulation, invoke `update_contact_manifolds()` to refresh persistent contacts, inspect `contact_manifolds()` or
  `collision_telemetry()` for diagnostics, and register solver callbacks via `set_constraint_callbacks()` to integrate custom
  constraint resolvers.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
