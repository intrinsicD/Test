# Runtime Module

## Current State
- Aggregates all engine subsystems through `RuntimeHost`, orchestrating animation playback, physics simulation, compute dispatching, and scene graph mirroring each tick.
- Exposes both C++ and C-compatible entry points for lifecycle control (`initialize`, `tick`, `shutdown`) and telemetry queries (joint names, body positions, dispatch timings).
- Maintains a default dependency bundle (`RuntimeHostDependencies`) that seeds sample controllers, meshes, and physics worlds for quick-start scenarios.
- Discovers subsystem plugins through a `SubsystemRegistry`, loading enabled modules (and their dependencies) during runtime initialization.
- Accepts subsystem plugins through `RuntimeHostDependencies::subsystem_plugins`, invoking their lifecycle hooks during initialization, shutdown, and tick to support dependency-injected extensions.
- Exposes helper APIs (`configure_with_default_subsystems`, `default_subsystem_names`) and C bindings (`engine_runtime_configure_with_modules`) so hosts can choose which subsystems to load without manually constructing plugin instances.
- Smoke tests in `engine/runtime/tests/` validate module registration; full integration coverage lives in higher-level scenarios.

## Usage
- Build via `cmake --build --preset <preset> --target engine_runtime`; this pulls every other module as a dependency.
- Include `<engine/runtime/api.hpp>` when hosting the runtime loop or embedding the engine; call `RuntimeHost::initialize()`/`tick()`/`shutdown()` to drive simulation.
- Run `ctest --preset <preset> --tests-regex engine_runtime` to verify module plumbing before running broader integration suites.
- Toggle subsystem availability at configure time via the `ENGINE_ENABLE_<MODULE>` CMake options (for example `-DENGINE_ENABLE_RENDERING=OFF`). Disabled subsystems are omitted from the default registry but can still be re-enabled explicitly through the helper APIs.

## Subsystem Plugin Contract

Subsystems implement `engine::core::plugin::ISubsystemInterface`. Each plugin exposes its
identity, declares dependencies, and reacts to lifecycle callbacks:

- `initialize(const SubsystemLifecycleContext&)` – invoked when the runtime boots.
- `shutdown(const SubsystemLifecycleContext&)` – invoked in reverse order during teardown.
- `tick(const SubsystemUpdateContext&)` – invoked once per frame with the timestep.

Dependencies are expressed by name via `dependencies()`. The `SubsystemRegistry` resolves the
transitive closure so dependent services are constructed in a deterministic order before they are
provided to the `RuntimeHost`.

`engine::runtime::make_default_subsystem_registry()` registers descriptors for every module enabled
via the `ENGINE_ENABLE_<MODULE>` CMake options (for example `ENGINE_ENABLE_RENDERING`). Hosts can
extend or replace the registry by registering additional descriptors prior to runtime
initialization.

Helper APIs defined in `engine/runtime/api.hpp` simplify configuration:

- `configure_with_default_subsystems()` resets the global `RuntimeHost` to the default registry.
- `configure_with_default_subsystems(std::span<const std::string_view>)` loads a subset of modules
  by name while automatically resolving dependencies.
- `default_subsystem_names()` enumerates the descriptors registered by default so hosts can present
  discovery UIs or validation diagnostics.
- The C bindings `engine_runtime_configure_with_default_modules()` and
  `engine_runtime_configure_with_modules()` mirror the C++ helpers for embedders that use the C ABI.

Projects with bespoke requirements can still assemble `RuntimeHostDependencies` manually, but the
helper APIs cover common cases such as disabling expensive subsystems or bootstrapping from a list
of module names.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
