# Scene Module

## Current State

- Provides runtime entity management with helpers for parenting, safe destruction, and transform propagation backed by EnTT.
- Ships a minimal component set (Name, Hierarchy, Local/World/Dirty transform tags) that keeps structural metadata and spatial state in sync.
- Includes deterministic serialization for the built-in components alongside loader logic that rebuilds hierarchy relationships.
- Integrates transform systems that register themselves with a scene at construction and propagate dirty flags through the graph on `Scene::update()`.

## Usage

- Link against `engine_scene` to host world state; the target inherits `engine::project_options` and publishes headers through `engine::headers` for dependent modules.
- Use `Scene::create_entity(std::string)` to attach a `Name` component at creation time or `Scene::create_entity()` for anonymous entities.
- Manage hierarchy relationships via the `Entity::set_parent`/`Entity::detach_from_parent` helpers which validate cross-scene usage.
  - Pass `preserve_world_transform = true` when re-parenting or detaching to keep an entity's world transform constant while its local transform is recomputed relative to the new parent.
- Call `Scene::update()` each frame (or whenever systems should run) to propagate dirty transforms throughout the scene graph.
- Build and test via the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to track integration regressions.

## Roadmap

| Phase | Goals | Notes |
| --- | --- | --- |
| **Immediate** | Define schemas for first-class runtime components (cameras, lights, visibility volumes) and register their systems. | Unlocks per-frame updates for non-transform data and lays the groundwork for renderer/runtime integration. |
| **Short Term** | Expand traversal helpers beyond raw registry views (hierarchical iterators, dependency queries, world/local transform sampling). | Builds on the existing hierarchy metadata and prepares the module for scene-graph driven scheduling. |
| **Medium Term** | Broaden serialization to cover the new component families, add version tags, and ensure forward/backward compatibility tests. | Requires updating the loader to remap legacy data and documenting the format expectations. |
| **Ongoing** | Add scenario-driven tests and profiling harnesses that stress entity lifecycle, hierarchy manipulation, and serialization throughput. | Keeps regressions visible as the component matrix and system count grow. |

Keep this plan in sync with the aggregated backlog in the repository root when milestones shift.
