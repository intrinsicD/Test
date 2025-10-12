# Runtime Module

## Current State
- Aggregates all engine subsystems through `RuntimeHost`, orchestrating animation playback, physics simulation, compute dispatching, and scene graph mirroring each tick.
- Exposes both C++ and C-compatible entry points for lifecycle control (`initialize`, `tick`, `shutdown`) and telemetry queries (joint names, body positions, dispatch timings).
- Maintains a default dependency bundle (`RuntimeHostDependencies`) that seeds sample controllers, meshes, and physics worlds for quick-start scenarios.
- Discovers subsystem plugins through a `SubsystemRegistry`, loading enabled modules (and their dependencies) during runtime initialization.
- Accepts subsystem plugins through `RuntimeHostDependencies::subsystem_plugins`, invoking their lifecycle hooks during initialization, shutdown, and tick to support dependency-injected extensions.
- Smoke tests in `engine/runtime/tests/` validate module registration; full integration coverage lives in higher-level scenarios.

## Usage
- Build via `cmake --build --preset <preset> --target engine_runtime`; this pulls every other module as a dependency.
- Include `<engine/runtime/api.hpp>` when hosting the runtime loop or embedding the engine; call `RuntimeHost::initialize()`/`tick()`/`shutdown()` to drive simulation.
- Run `ctest --preset <preset> --tests-regex engine_runtime` to verify module plumbing before running broader integration suites.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
