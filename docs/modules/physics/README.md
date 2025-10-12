# Physics Module

## Current State
- Defines rigid-body representations (`PhysicsWorld`, `RigidBody`) with configurable gravity, damping, and substepping controls, plus collider authoring helpers for spheres, AABBs, and capsules.
- Implements force integration, collider assignment, and collision detection leveraging geometry intersection utilities.
- Exposes module metadata via `module_name()` and integrates with runtime orchestration.
- Smoke tests in `engine/physics/tests/` validate module registration; collision algorithms are exercised indirectly through runtime coverage.

## Usage
- Build using `cmake --build --preset <preset> --target engine_physics`; this links against `engine_math` and `engine_geometry`.
- Include `<engine/physics/api.hpp>` to construct worlds, bodies, apply forces, and query collision pairs.
- Run `ctest --preset <preset> --tests-regex engine_physics` with testing enabled to guard entry points; supplement with scenario tests in runtime when modifying collision code.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
