# Engine Architecture Overview

This document sketches the top-level organisation of the engine, focusing on module boundaries, entity-
component layering, and runtime discovery.

## Module boundaries

Each functional domain (math, geometry, scene, rendering, physics, etc.) resides in its own top-level
subdirectory under [`engine/`](../..). Modules publish a minimal ABI surface through `include/engine/<name>`
headers, while corresponding `src/` directories (where present) contain shared-library entry points. Shared
infrastructure is imported explicitly; no module relies on transitive include paths.

Math sits at the base of the dependency graph, providing numerics for geometry, physics, and rendering.
Geometry builds on math to supply shapes, spatial queries, and property registries. The scene module wraps
the ECS, depending on math for transforms and exposing entity handles consumed by animation, physics, and
rendering.

## ECS layering

The [`Scene` façade](../../engine/scene/include/engine/scene/scene.hpp) wraps an `entt::registry` and owns
entity creation, destruction, and component storage. High-level systems interact exclusively through the
`Scene` interface—either by iterating `Scene::view<Components...>()` or by caching `Entity` handles that
validate themselves before acting. This design enforces a clear ownership model: only the scene owns the
registry, while other modules supply components and systems that operate on it. Future subsystem initializers
can extend `Scene::initialize_systems()` without leaking `entt` internals across module boundaries.

## Runtime subsystem discovery

The runtime module is responsible for surfacing all loadable subsystems. Its
[`api.cpp`](../../engine/runtime/src/api.cpp) composes a static array of module identifiers by invoking
`module_name()` on each dependency. The exported C ABI (`engine_runtime_module_*`) allows host applications
to enumerate available modules at startup, enabling dynamic linking or scripting layers to request only the
services they need. When new modules are added, extending this array is enough for them to appear in the
discovery list, keeping runtime configuration declarative and centralized.
