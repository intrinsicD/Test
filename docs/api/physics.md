# Physics Module

The physics API provides a compact `PhysicsWorld`/`RigidBody` pair for deterministic, step-based simulation. Public
interfaces are defined in [`engine/physics/api.hpp`](../../engine/physics/include/engine/physics/api.hpp).

## Rigid bodies

A `RigidBody` stores mass, inverse mass, position, velocity, and an accumulated force. `add_body` normalises the mass
field and appends the body to the world. Callers may query bodies via `body_at(world, index)` which throws
`std::out_of_range` when the index is invalid to surface configuration mistakes early.

## Stepping the world

```cpp
#include <engine/physics/api.hpp>

engine::physics::PhysicsWorld world;
world.gravity = {0.0F, -9.81F, 0.0F};
engine::physics::RigidBody body;
body.mass = 2.0F;
const auto handle = engine::physics::add_body(world, body);
engine::physics::apply_force(world, handle, {0.0F, 20.0F, 0.0F});
engine::physics::integrate(world, 0.016);
```

`clear_forces` zeroes the force accumulators when callers need a clean slate before the next update. The runtime uses
these helpers to turn animation-derived translations into forces applied to the simulated body.

_Last updated: 2025-03-15_
