# Scene Module

The scene module adapts the [`entt`](https://github.com/skypjack/entt) entity-component-system into an
engine-friendly façade.

## Scene graph façade
- [`engine/scene/scene.hpp`](../../engine/scene/include/engine/scene/scene.hpp) exposes the `Scene` and
  `Entity` wrappers that manage lifetime, component insertion, and registry access. Inline implementations
  forward to the underlying `entt::registry`, providing ergonomic helpers such as `Entity::emplace`,
  `Entity::destroy`, and templated `Scene::view` iterators.
- [`engine/scene/api.hpp`](../../engine/scene/include/engine/scene/api.hpp) and
  [`engine/scene/src/api.cpp`](../../engine/scene/src/api.cpp) publish the module identifier used by the
  runtime loader to discover the ECS layer.

## Key behaviours
- `Scene` instances own an `entt::registry` and control system initialization via an internal
  `initialize_systems` hook.
- Entities are lightweight handles that validate against their backing scene before performing registry
  operations, preventing accidental use-after-destroy patterns.
- The module is header-only aside from the ABI surface, enabling inline component access in performance-
  critical loops.
