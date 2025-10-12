# Engine Modules

## Purpose
Aggregate the engine subsystems into a cohesive runtime capable of driving games and simulations.

## Module Structure

```
engine/
├── math/        # Core math primitives (Layer 0)
├── platform/    # OS/window abstraction (Layer 1)
├── memory/      # Memory management (Layer 1)
├── geometry/    # Shapes, meshes, spatial queries (Layer 2)
├── animation/   # Skeletal animation, blend trees (Layer 2)
├── physics/     # Dynamics simulation (Layer 3)
└── rendering/   # Graphics abstraction, scene graph (Layer 4)
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for dependency rules and layering guidance.

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

## API Stability Roadmap

- **Stable (1.0+)**: `engine::math`, `engine::platform`
- **Beta (0.9+)**: `engine::geometry`, `engine::physics`
- **Alpha (0.5+)**: `engine::rendering`, `engine::animation`

Consult individual module READMEs for finer-grained status.

## Versioning Strategy

Modules version independently but must interoperate within a given engine major version.
- **Engine 1.x**: All modules maintain compatibility; breaking changes require migration guides.
- **Engine 2.x**: Breaking changes permitted after deprecation cycle completes.

## TODO / Next Steps

### Pre-1.0 Release (Target: Q2 2026)
- [ ] @alice — Finalise geometry API (shapes, intersection tests). Due: 2026-01-31.
- [ ] @bob — Complete physics↔geometry integration tests. Due: 2026-02-15.
- [ ] @carol — Author rendering↔animation binding design doc. Due: 2026-02-28.
- [ ] @dave — Produce performance benchmarks across all modules. Due: 2026-03-15.
- [ ] @team — Lock module APIs and begin 90-day stability period. Due: 2026-04-01.

### Post-1.0
- [ ] Stand up plugin system for extending modules without recompilation.
- [ ] Generate scripting bindings from C++ headers.
- [ ] Enable hot-reload workflows for development builds.

## Contributing

Refer to `../CONTRIBUTING.md` for coding standards and review policy. Module-specific guidance resides in each module README.
