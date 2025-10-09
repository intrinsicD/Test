# Physics Module

## Current State

- Offers a simple rigid-body world with force accumulation, Euler integration, and mass management.
- Exposes safe accessors that validate indices and ensure deterministic updates.

## Usage

- Link against `engine_physics` and include `<engine/physics/api.hpp>` to simulate bodies; the target inherits `engine::project_options` and is exposed via `engine::headers` for downstream consumers.
- Extend dynamics, collision, and constraints logic within `src/` and cover it via tests.
- Compile and test with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Add collision detection, constraints, and stable integration schemes.
