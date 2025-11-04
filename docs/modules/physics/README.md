# Physics Module

## Overview

The physics module provides rigid-body simulation with collision detection, constraint solving, and telemetry. It features a sweep-and-prune broad phase, persistent contact manifolds, configurable substepping, and support for multiple collider primitives (sphere, capsule, AABB).

## Core Concepts

### Physics World

`PhysicsWorld` manages all rigid bodies and simulates their interactions:

```cpp
#include "engine/physics/api.hpp"

engine::physics::PhysicsWorld world;
engine::physics::set_substepping(world, /*max_step=*/0.01, /*max_substeps=*/8);
engine::physics::set_linear_damping(world, /*damping=*/0.98f);
world.gravity = {0.0f, -9.81f, 0.0f};

// Step simulation (integrates + updates contacts + solver telemetry)
engine::physics::step(world, /*dt=*/0.0166667);
```

### Rigid Bodies

Create and configure rigid bodies:

```cpp
engine::physics::RigidBody body;
body.position = {0.0f, 10.0f, 0.0f};
body.velocity = {0.0f, 0.0f, 0.0f};
body.mass = 1.0f;

// Add to world
const std::size_t body_id = engine::physics::add_body(world, body);

// Update body state via helpers
engine::physics::apply_force(world, body_id, {0.0f, 100.0f, 0.0f});
engine::physics::integrate(world, 0.0166667); // or use engine::physics::step(world, dt)
```

### Colliders

Attach collision geometry to bodies:

```cpp
// Sphere collider
engine::physics::set_collider(world, body_id, engine::physics::Collider::make_sphere(/*radius=*/1.0f));

// AABB collider (local, offset is optional)
const engine::geometry::Aabb local_box = engine::geometry::MakeAabbFromCenterExtent({0,0,0}, {1,1,1});
engine::physics::set_collider(world, body_id, engine::physics::Collider::make_aabb(local_box));

// Capsule collider
engine::physics::Collider::Capsule capsule{.point_a = {0,-1,0}, .point_b = {0,1,0}, .radius = 0.5f};
engine::physics::set_collider(world, body_id, engine::physics::Collider::make_capsule(capsule));
```

### Mass Properties

Mass is clamped to avoid instability. Static bodies are expressed with zero or near-zero mass (inverse mass becomes 0):

- Minimum mass clamp to treat as static
- Infinite mass (static bodies): `inverse_mass = 0`

## Collision Detection

### Broad Phase: Sweep and Prune

Broad phase identifies potentially colliding pairs by sorting AABB bounds:

- O(n log n) insertion/removal
- O(n + k) update, where k is overlapping pairs
- Cache-coherent sorted arrays along one axis

Pairs then proceed to the narrow phase.

### Narrow Phase

Precise collision detection between collider pairs is built into `update_contact_manifolds` and exposed by free functions for pairwise queries in geometry (e.g., `engine::geometry::Intersects`). Supported pairs include Sphere–Sphere, Sphere–AABB, Capsule–Capsule, Capsule–AABB, and AABB–AABB.

### Persistent Contact Manifolds (`RT-002`)

Contacts are cached across frames for improved stability:

```cpp
// Update manifolds after bodies moved
engine::physics::update_contact_manifolds(world);

for (const auto& manifold : engine::physics::contact_manifolds(world)) {
    // manifold.first/second: body indices
    // manifold.lifetime: frames the contact persisted
}
```

Benefits:
- Reduced jitter
- Better stacking stability
- Fewer solver iterations due to warm starts

## Constraint Solving

Configure the sequential impulse solver via `ConstraintSolverConfig`:

```cpp
engine::physics::ConstraintSolverConfig cfg{};
cfg.iterations = 10;          // iterations per update_contact_manifolds
cfg.restitution = 0.0f;       // [0,1]
cfg.baumgarte = 0.2f;         // positional correction factor
cfg.penetration_slop = 1e-4f; // tolerance
engine::physics::set_constraint_solver_config(world, cfg);
```

## Substepping and Damping

```cpp
engine::physics::set_substepping(world, /*max_step=*/0.01f, /*max_substeps=*/8);
engine::physics::set_linear_damping(world, /*damping=*/2.0f);
```

- Substeps improve stability by splitting dt into smaller integration steps.
- Linear damping is applied exponentially per substep.

## Telemetry & Diagnostics

Access physics telemetry from the world:

```cpp
const auto& t = engine::physics::collision_telemetry(world);
// t.manifold_count, t.contact_count, t.max_penetration, t.solver_iterations
```

## Integration with Runtime

Typical runtime order:

1. Advance animation (if any)
2. Write kinematic targets
3. Physics: `engine::physics::step(world, dt)`
4. Extract body positions for rendering
5. Publish telemetry

## Current State

- Rigid bodies with mass clamping and linear damping.
- Colliders: sphere, AABB, capsule; broad-phase sweep-and-prune; narrow-phase contact generation.
- Persistent contact manifolds with lifetime and sampling utilities.
- Sequential impulse solver with configurable iterations and parameters, telemetry exposure.

## Usage

- Run physics tests:
  - `ctest --preset linux-gcc-debug -R physics`
- Run the collision throughput benchmark:
  - `ctest --preset linux-gcc-release -R physics_collision_benchmark`
- Minimal integration tick:
  - Use `engine::physics::step(world, dt)` to integrate and update contacts in one call.

## Dependencies

- Math (vectors/matrices)
- Geometry (shapes and intersections)
- Core/Runtime (integration points)

## Related Documentation

- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md)
- [`../../archive/backlog/legacy/tasks/T_0117_PHYSICS_CONTACT_MANIFOLDS.md`](../../archive/backlog/legacy/tasks/T_0117_PHYSICS_CONTACT_MANIFOLDS.md)

## TODO / Next Steps

- Scope automation for long-term collision telemetry trends (post-`PH-430`) (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Evaluate and design joint constraints (hinge/spherical) ADRs (see [docs/ROADMAP.md](../../ROADMAP.md)).
- Expand manifold contact reduction (clipping, multi-point generation) and friction modeling.
