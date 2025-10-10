# Scene Module

## Current State

- Provides runtime entity management with helpers for parenting and transform propagation.
- Supports serialization of core components (Name, Hierarchy, Transform) with deterministic entity identifiers.
- Integrates systems that mark and propagate transform updates through the hierarchy.

## Usage

- Link against `engine_scene` to host world state; the target inherits `engine::project_options` and publishes headers through `engine::headers` for dependent modules.
- Use `Scene::create_entity(std::string)` to attach a `Name` component at creation time or `Scene::create_entity()` for anonymous entities.
- Manage hierarchy relationships via the `Entity::set_parent`/`Entity::detach_from_parent` helpers which validate cross-scene usage.
  - Pass `preserve_world_transform = true` when re-parenting or detaching to keep an entity's world transform constant while its local transform is recomputed relative to the new parent.
- Call `Scene::update()` each frame (or whenever systems should run) to propagate dirty transforms throughout the scene graph.
- Build and test via the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to track integration regressions.

## TODO / Next Steps

- Extend system registration to cover additional component types (lights, cameras, custom gameplay state).
- Surface higher-level traversal utilities for queries beyond simple registry views.
