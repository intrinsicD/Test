# Engine Physics Sources

## Current State

- Implements force integration, rigid-body storage, and collider management inside `api.cpp`.
- Reuses `engine::geometry` intersection predicates for collision detection.
- Isolates collision detection routines in `collisions.cpp` so narrow-phase logic evolves independently of integration code.

## Usage

- Source files compile into `engine_physics`; ensure the build target stays warning-clean.
- Mirror API additions with implementation updates in this directory.

## TODO / Next Steps

- Add scenario-driven examples and profiling to exercise the implementation.
