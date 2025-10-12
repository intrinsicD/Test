# Engine Modules

## Purpose
Aggregate the engine subsystems into a cohesive runtime capable of driving games and simulations.

## Module Structure

```
engine/
├── animation/   # Clip sampling, controllers, and blend-tree evaluation
├── assets/      # Runtime caches and hot-reload hooks for imported content
├── compute/     # Kernel dispatcher and execution profiling utilities
├── core/        # Shared identifiers and module discovery helpers
├── geometry/    # Meshes, graphs, spatial queries, property registries
├── io/          # Geometry/animation importers, exporters, and detection
├── math/        # Vector, matrix, quaternion, and transform primitives
├── physics/     # Rigid-body world, colliders, and collision detection
├── platform/    # Filesystem, windowing, and input abstractions
├── rendering/   # Frame graph prototype and forward pipeline scaffolding
├── runtime/     # `RuntimeHost` orchestration over animation/physics/geometry
├── scene/       # EnTT façade, hierarchy systems, serialization helpers
└── tools/       # Staging area for editor/profiling utilities
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for dependency rules and layering guidance. Each
module ships a local README that records its current behaviour and TODO items.

## Usage

### Adding a New Module

1. Create the directory `engine/<module>/` with `include/`, `src/`, and `tests/` subdirectories.
2. Copy [MODULE_README_TEMPLATE.md](MODULE_README_TEMPLATE.md) into the module root and populate it.
3. Declare the module target in `engine/<module>/CMakeLists.txt`.
4. Add `add_subdirectory(<module>)` to `engine/CMakeLists.txt`.
5. Export public headers under `engine/<module>/include/engine/<module>/` and define `ENGINE_<MODULE>_API` visibility macros.
6. Update [ARCHITECTURE.md](ARCHITECTURE.md) with the new module's layer and dependencies.

### Using Engine Modules in Client Code

```cmake
add_subdirectory(engine)

target_link_libraries(my_game
    PRIVATE
        engine::rendering
        engine::physics
        engine::geometry
)
```

All modules inherit shared compile options via `engine::project_options` and publish headers through `engine::headers`.

## Build System Conventions
- Headers live in `engine/<module>/include/engine/<module>/`.
- Sources live in `engine/<module>/src/`.
- Tests live in `engine/<module>/tests/` and are wired into CTest.
- Each module exports exactly one `engine::<module>` target and matching `ENGINE_<MODULE>_API` macro.
- Static and shared builds must respect the dependency layers documented in [ARCHITECTURE.md](ARCHITECTURE.md).

## API Stability Snapshot

All modules are currently **pre-1.0 prototypes**. Public headers are evolving while the
engine solidifies core behaviours. Refer to individual module READMEs and the
[global roadmap](../docs/global_roadmap.md) for the most recent stability notes.

## Versioning Strategy

Modules version independently but must interoperate within a given engine major version.
- **Engine 1.x**: All modules maintain compatibility; breaking changes require migration guides.
- **Engine 2.x**: Breaking changes permitted after deprecation cycle completes.

## TODO / Next Steps

Module-level backlogs are tracked in their respective READMEs. The highest-priority
milestones that cut across modules are summarised below; keep this list synchronised with
the workspace root `README.md` and the detailed roadmaps under `docs/`.

- **Geometry & IO** – Finish round-tripping meshes/graphs/point clouds through the
  import/export interfaces and extend the property-backed acceleration structures with
  regression coverage.
- **Physics** – Build on the existing sweep-and-prune broad phase by introducing
  contact-manifold generation and the first constraint solver hooks.
- **Rendering** – Enrich frame-graph resource descriptions and prototype the reference
  GPU scheduler before wiring backend integrations.
- **Runtime** – Expand `RuntimeHost` diagnostics, lifecycle checks, and scene mirroring to
  cope with dynamically streamed rigs and meshes.
- **Platform & Tooling** – Replace mock window/input providers with GLFW/SDL backends and
  surface filesystem write/watch utilities to unblock hot-reload workflows.

## Contributing

Refer to `../CONTRIBUTING.md` for coding standards and review policy. Module-specific guidance resides in each module README.
