# Physics

_Path: `engine/physics`_

_Last updated: 2025-03-15_


## Overview

`engine_physics` now exposes a compact `PhysicsWorld` and `RigidBody` abstraction that powers the runtime smoke test.
While the long-term goal remains a feature-complete dynamics stack, the current code focuses on deterministic,
single-body integration with gravity and external forces. The API is intentionally minimal so that higher-level
systems (animation, gameplay scripting) can experiment without pulling the entire dynamics subsystem.

Key entry points live in [`include/engine/physics/api.hpp`](include/engine/physics/api.hpp):

- `add_body` – Registers a new rigid body with correct inverse-mass bookkeeping.
- `apply_force` / `clear_forces` – Manage per-step force accumulation.
- `integrate` – Advances the world using a semi-implicit Euler step.

## TODO

- Introduce simple constraints (springs, dampers) so animation-driven targets can influence physics more expressively.
- Track angular velocity/orientation to extend the runtime demo beyond point-mass motion.
- Feed integration statistics back into the runtime for profiling and regression testing.
