# Physics Module

## Current State

- Offers a simple rigid-body world with force accumulation, Euler integration, configurable damping/substepping, and mass management.
- Clamps non-positive masses to produce static bodies that ignore forces and gravity, ensuring stable placeholders.
- Associates rigid bodies with optional sphere, capsule, or axis-aligned bounding-box colliders.
- Performs broad-phase sweep-and-prune pruning before delegating narrow-phase collision queries to `engine::geometry` intersection routines.
- Exposes safe accessors that validate indices and ensure deterministic updates.

## Usage

- Link against `engine_physics` and include `<engine/physics/api.hpp>` to simulate bodies; the target inherits `engine::project_options` and is exposed via `engine::headers` for downstream consumers.
- Extend dynamics, collision, and constraints logic within `src/` and cover it via tests.
- Compile and test with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

Roadmap items are prioritised in [docs/roadmaps/physics.md](../../docs/roadmaps/physics.md) and reflected in the
[global alignment overview](../../docs/global_roadmap.md). The near-term focus is summarised below:

- Prioritise contact-manifold generation and constraint solving now that broad-phase pruning and capsule support are available.
- Document damping/substepping guidance and expand regression scenarios that stress-test high-velocity and stacked-body cases.
- Extend collider coverage beyond capsules (OBB, triangle meshes) and integrate constraint resolution per the shared roadmap.
