# Engine Physics Sources

## Current State

- Implements force integration, rigid-body storage, and collider management inside `api.cpp`.
- Reuses `engine::geometry` intersection predicates for collision detection.

## Usage

- Source files compile into `engine_physics`; ensure the build target stays warning-clean.
- Mirror API additions with implementation updates in this directory.

## TODO / Next Steps

- Add scenario-driven examples and profiling to exercise the implementation.
- Separate collision detection into its own translation unit as the feature set grows.
