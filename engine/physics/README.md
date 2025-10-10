# Physics Module

## Current State

- Offers a simple rigid-body world with force accumulation, Euler integration, and mass management.
- Associates rigid bodies with optional sphere or axis-aligned bounding-box colliders.
- Performs narrow-phase collision queries by delegating to `engine::geometry` intersection routines.
- Exposes safe accessors that validate indices and ensure deterministic updates.

## Usage

- Link against `engine_physics` and include `<engine/physics/api.hpp>` to simulate bodies; the target inherits `engine::project_options` and is exposed via `engine::headers` for downstream consumers.
- Extend dynamics, collision, and constraints logic within `src/` and cover it via tests.
- Compile and test with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Extend collider support (OBB, capsules, meshes) and integrate a broad-phase acceleration structure.
- Implement contact manifold generation and constraint-based resolution for persistent collisions.
- Add energy-preserving integration schemes and sleeping/activation rules for performance.
