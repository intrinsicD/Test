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

## Module Stability Status

| Module    | Status | Version | ABI Stable | Docs Complete |
|-----------|--------|---------|------------|---------------|
| math      | ✅ Stable | 1.0.0 | Yes | Yes |
| geometry  | 🚧 Beta | 0.9.0 | No | Partial |
| physics   | 🚧 Beta | 0.8.0 | No | No |
| rendering | ⏳ Alpha | 0.5.0 | No | No |
| animation | ⏳ Alpha | 0.3.0 | No | Partial |

**Legend** – ✅ Stable: API frozen with ABI guarantees. 🚧 Beta: API mostly stable but breaking changes possible. ⏳ Alpha: active development; expect rapid iteration.

Refer to individual module READMEs and the
[global roadmap](../docs/global_roadmap.md) for the most recent stability notes and
release criteria.

## Versioning Strategy

Modules version independently but must interoperate within a given engine major version.
- **Engine 1.x**: All modules maintain compatibility; breaking changes require migration guides.
- **Engine 2.x**: Breaking changes permitted after deprecation cycle completes.

## TODO / Next Steps

Keep these milestones synchronised with the workspace root `README.md` and the detailed
roadmaps under `docs/`.

### Immediate (v2.0 readiness)
- [ ] Stabilise geometry and physics integration ahead of the v2.0 release target (Q2 2025).
- [ ] Establish ABI compatibility testing across supported toolchains (#234).
- [ ] Document cross-module integration patterns so contributors can align module roadmaps (#235).

### Short-term (post-freeze polish)
- [ ] Produce a module dependency visualisation to surface layering rules (#236).
- [ ] Expand build presets and CI coverage for standalone module builds.
- [ ] Ensure per-module README TODO sections mirror the aggregate backlog entries.

### Long-term (roadmap alignment)
- [ ] Maintain semantic versioning policy updates as modules transition between stability tiers.
- [ ] Grow contributor onboarding material, including review checklists and workflow guides.
- [ ] Track performance/benchmark coverage across modules to uphold zero-cost abstractions goals.

## Contributing

Refer to `../CONTRIBUTING.md` for coding standards and review policy. Module-specific guidance resides in each module README.
