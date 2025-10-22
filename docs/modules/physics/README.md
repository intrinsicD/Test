# Physics Module

## Overview

The physics module provides rigid-body simulation with collision detection, constraint solving, and comprehensive telemetry. It features a sweep-and-prune broad phase, persistent contact manifolds, configurable substepping, and support for multiple collider primitives (sphere, capsule, AABB, convex hulls).

## Core Concepts

### Physics World

`PhysicsWorld` manages all rigid bodies and simulates their interactions:

```cpp
#include "engine/physics/api.hpp"

physics::PhysicsWorld world;
world.gravity = {0.0f, -9.81f, 0.0f};
world.substeps = 4;  // 4 substeps per tick for stability
world.damping = 0.98f;  // Velocity damping per substep

// Step simulation
world.step(delta_time);
```

### Rigid Bodies

Create and configure rigid bodies:

```cpp
physics::RigidBody body;
body.position = {0.0f, 10.0f, 0.0f};
body.velocity = {0.0f, 0.0f, 0.0f};
body.mass = 1.0f;
body.restitution = 0.5f;  // Bounciness
body.friction = 0.3f;

// Add to world
auto body_id = world.add_body(body);

// Update body properties
world.set_velocity(body_id, {1.0f, 0.0f, 0.0f});
world.apply_force(body_id, {0.0f, 100.0f, 0.0f});
world.apply_impulse(body_id, {0.0f, 50.0f, 0.0f});
```

### Colliders

Attach collision geometry to bodies:

```cpp
// Sphere collider
physics::SphereCollider sphere{.radius = 1.0f};
world.add_collider(body_id, sphere);

// Capsule collider
physics::CapsuleCollider capsule{
    .radius = 0.5f,
    .height = 2.0f
};
world.add_collider(body_id, capsule);

// AABB collider
physics::AabbCollider box{
    .half_extents = {1.0f, 1.0f, 1.0f}
};
world.add_collider(body_id, box);

// Convex hull (mesh-based)
physics::ConvexHullCollider hull{
    .vertices = mesh_vertices
};
world.add_collider(body_id, hull);
```

### Mass Properties

Configure inertia and center of mass:

```cpp
body.mass = 10.0f;
body.inverse_mass = 1.0f / body.mass;
body.inertia_tensor = physics::compute_sphere_inertia(body.mass, radius);
body.center_of_mass = {0.0f, 0.0f, 0.0f};

// Automatic computation from collider
auto mass_props = physics::compute_mass_properties(sphere, density);
body.mass = mass_props.mass;
body.inertia_tensor = mass_props.inertia;
```

Mass is automatically clamped to prevent numerical instability:
- Minimum mass: `1e-6`
- Maximum mass: `1e6`
- Infinite mass (static bodies): `inverse_mass = 0`

## Collision Detection

### Broad Phase: Sweep and Prune

The physics world uses sweep-and-prune for efficient broad-phase collision detection:

```cpp
// Automatically maintained during simulation
// Detects potentially colliding pairs
auto potential_pairs = world.broad_phase_pairs();

fmt::print("Checking {} potential collision pairs\n", potential_pairs.size());
```

Characteristics:
- O(n log n) insertion/removal
- O(n + k) update, where k is overlapping pairs
- Cache-coherent sorted arrays along each axis
- Handles dynamic objects efficiently

### Narrow Phase

Precise collision detection between collider pairs:

```cpp
// Per-pair collision detection
physics::Contact contact = physics::detect_collision(
    sphere_collider_a, transform_a,
    capsule_collider_b, transform_b
);

if (contact.is_colliding) {
    fmt::print("Penetration depth: {}\n", contact.penetration_depth);
    fmt::print("Normal: ({}, {}, {})\n", 
        contact.normal.x, contact.normal.y, contact.normal.z);
}
```

Supported collision pairs:
- Sphere-Sphere
- Sphere-Capsule
- Sphere-AABB
- Capsule-Capsule
- Capsule-AABB
- AABB-AABB
- Convex-Convex (GJK/EPA)

### Persistent Contact Manifolds (`RT-002`)

Contacts are cached across frames for improved stability:

```cpp
// Manifolds maintained automatically
const auto& manifolds = world.contact_manifolds();

for (const auto& manifold : manifolds) {
    fmt::print("Contact between {} and {}\n", 
        manifold.body_a_id, manifold.body_b_id);
    fmt::print("  Points: {}\n", manifold.contact_points.size());
    fmt::print("  Lifetime: {} frames\n", manifold.frame_count);
}
```

Benefits:
- Reduced jitter from frame-to-frame contact changes
- Better stacking stability
- Lower solver iteration counts
- Warm-starting for faster convergence

## Constraint Solving

### Contact Constraints

Resolve collisions through impulse-based constraints:

```cpp
world.solver_iterations = 10;  // Iterations per substep
world.positional_correction = 0.2f;  // Baumgarte stabilization
world.slop = 0.01f;  // Penetration tolerance
```

### Joint Constraints (Planned)

Future support for:
- Distance joints (spring-damper)
- Hinge joints (revolute)
- Ball-and-socket joints (spherical)
- Fixed joints (weld)

## Substepping

Improve simulation stability with fixed substeps:

```cpp
world.substeps = 4;  // 4 substeps per world.step() call

// Example: 60 FPS with dt=0.016s
// Each substep processes dt/4 = 0.004s
// Smaller timesteps = more stable simulation
```

Substep configuration:
- Recommended range: 1-10 substeps
- Higher values = more stable but slower
- Each substep performs full collision detection + solving

## Telemetry & Diagnostics

Access physics telemetry through runtime diagnostics:

```cpp
const auto& diag = runtime::diagnostics();
const auto& collision_telemetry = diag.physics_collision;

fmt::print("Broad phase pairs: {}\n", collision_telemetry.broad_phase_pair_count);
fmt::print("Narrow phase tests: {}\n", collision_telemetry.narrow_phase_test_count);
fmt::print("Active contacts: {}\n", collision_telemetry.active_contact_count);
fmt::print("Manifold cache hits: {}\n", collision_telemetry.manifold_cache_hit_count);
```

Available metrics:
- `broad_phase_pair_count`: Potential collision pairs from sweep-and-prune
- `narrow_phase_test_count`: Precise collision tests performed
- `active_contact_count`: Current frame's active contacts
- `manifold_cache_hit_count`: Contacts reused from previous frame
- `solver_iteration_count`: Total constraint solver iterations
- `substep_count`: Substeps executed this frame

## Integration with Runtime

The runtime orchestrates physics simulation:

```cpp
// In RuntimeHost::tick()
1. Advance animation (update joint transforms)
2. Apply animation to rigid bodies (kinematic control)
3. Step physics world (collision + solving)
4. Extract body positions for rendering
5. Update telemetry
```

Body positions are exposed via `runtime::body_positions()` for rendering and debugging.

## Performance Considerations

Benchmarks from `RT-002` (physics manifold milestone):
- Broad phase: ~0.15ms for 1000 bodies
- Narrow phase: ~0.8ms for 100 active contacts
- Solver: ~1.2ms for 100 contacts with 10 iterations
- Total step: ~2.5ms for moderately complex scene

Optimization tips:
- Use sweep-and-prune for dynamic objects (automatic)
- Increase substeps for stability, not solver iterations
- Static bodies have `inverse_mass = 0` (no integration cost)
- Convex hulls are more expensive than primitives

## Testing

Physics tests validate:
- Rigid body integration (`test_rigid_body.cpp`)
- Collision detection accuracy (`test_collision.cpp`)
- Manifold persistence (`test_manifolds.cpp`)
- Constraint solving convergence (`test_solver.cpp`)
- Mass clamping and stability (`test_stability.cpp`)

Run tests:
```bash
ctest --preset clang-debug -R physics
```

Benchmarks:
```bash
ctest --preset clang-release -R physics_benchmark
```

## Dependencies

- **Math**: Vector, matrix, quaternion types for transforms
- **Core**: Telemetry schema, ECS integration
- **Runtime** (integration): Lifecycle orchestration and diagnostics

## Related Documentation

- [`ROADMAP.md`](ROADMAP.md): Module milestones including `RT-002` manifold work
- [`../../architecture.md`](../../architecture.md): Physics integration in data flow
- [`../../tasks/T-0117-physics-contact-manifolds.md`](../../tasks/T-0117-physics-contact-manifolds.md): Manifold implementation milestone
- Benchmark results: `engine/physics/benchmarks/`


