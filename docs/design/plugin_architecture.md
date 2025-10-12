# Runtime Plugin Architecture

The runtime module discovers engine subsystems through the `SubsystemRegistry` and
loads them into the `RuntimeHost` at initialization. This document summarises the
contract between hosts, subsystem providers, and the build system.

## Subsystem Interfaces

Subsystems implement `engine::core::plugin::ISubsystemInterface`. Implementations
report their name, dependency list, and respond to lifecycle callbacks:

- `initialize(const SubsystemLifecycleContext&)` – invoked when the runtime boots.
- `shutdown(const SubsystemLifecycleContext&)` – invoked in reverse order at teardown.
- `tick(const SubsystemUpdateContext&)` – invoked once per frame with the timestep.

A subsystem may depend on other subsystems by returning their names from
`dependencies()`. The registry resolves the transitive closure before instantiating
plugins so that dependent services are constructed in a consistent order.

## Registry Population

`engine::runtime::make_default_subsystem_registry()` populates the registry with
static descriptors for each engine module that is enabled at configure time. Each
descriptor records the module name and the factory used to instantiate the
subsystem. Hosts can extend or replace the registry before configuring the runtime
by registering additional descriptors.

Subsystem availability is gated by the `ENGINE_ENABLE_<MODULE>` CMake options (for
example `ENGINE_ENABLE_RENDERING`). When an option is set to `OFF`, the
corresponding descriptor is not registered in the default registry. Projects can
override the options to shrink the runtime footprint while keeping optional
modules available for specialised builds.

## Host Configuration APIs

`engine/runtime/api.hpp` exposes helpers that simplify plugin configuration:

- `configure_with_default_subsystems()` resets the global `RuntimeHost` to the
  default registry with all enabled modules.
- `configure_with_default_subsystems(std::span<const std::string_view>)` selects a
  subset of modules by name, resolving dependencies automatically.
- `default_subsystem_names()` enumerates the modules registered in the default
  registry so hosts can present discovery UI or validation diagnostics.
- The C bindings `engine_runtime_configure_with_default_modules()` and
  `engine_runtime_configure_with_modules()` mirror the C++ helpers for embedders
  that interact with the runtime via the C ABI.

Hosts that need full control can still build a `RuntimeHostDependencies` instance
manually by supplying a custom `SubsystemRegistry` and plugin vector. The helper
APIs ensure common scenarios—such as disabling heavy subsystems or bootstrapping
from configuration files—require only a list of module names.

## Documentation Responsibilities

When the plugin inventory changes (for example because a new module is added or a
subsystem becomes optional), update the following artefacts together:

- `docs/design/architecture_improvement_plan.md` – record backlog status.
- Module READMEs – describe new dependencies or lifecycle guarantees.
- `README.md` – summarise the available build-time flags so integrators can
  discover them without opening the design notes.
