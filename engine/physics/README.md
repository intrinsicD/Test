# Physics Module

## Current State

- Offers a simple rigid-body world with force accumulation, Euler integration, and mass management.
- Clamps non-positive masses to produce static bodies that ignore forces and gravity, ensuring stable placeholders.
- Associates rigid bodies with optional sphere or axis-aligned bounding-box colliders.
- Performs narrow-phase collision queries by delegating to `engine::geometry` intersection routines.
- Exposes safe accessors that validate indices and ensure deterministic updates.

## Usage

- Link against `engine_physics` and include `<engine/physics/api.hpp>` to simulate bodies; the target inherits `engine::project_options` and is exposed via `engine::headers` for downstream consumers.
- Extend dynamics, collision, and constraints logic within `src/` and cover it via tests.
- Compile and test with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

Roadmap items are prioritised in [docs/physics/roadmap.md](../../docs/physics/roadmap.md) and reflected in the
[global alignment overview](../../docs/global_roadmap.md). The near-term focus is summarised below:

- Stabilise the rigid-body core by tightening mass/force invariants, documenting API usage, and splitting collision helpers into dedicated units.
- Introduce a scalable collision pipeline (broad-phase acceleration, contact manifolds, and a constraint solver) before widening collider coverage.
- Advance dynamics fidelity with improved integration schemes, sleeping/activation heuristics, and an extensible collider set (OBB, capsules, meshes).
