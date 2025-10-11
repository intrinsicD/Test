# Core Module

## Current State

- Implements foundational services such as the entity registry wrapper and module naming API.
- Provides scaffolding for application lifecycle, configuration, diagnostics, and plugin subsystems.

## Usage

- Include `<engine/core/api.hpp>` and link against `engine_core`; the target inherits `engine::project_options` and exposes headers through `engine::headers` for downstream modules.
- Extend ECS features or add new core services in the dedicated subdirectories.
- Build and validate changes with the repository presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, etc.).

## TODO / Next Steps

The sequencing below underpins the runtime and tooling plans captured in the
[global roadmap](../../docs/global_roadmap.md).

1. **Establish the runtime nucleus.**
   - Define the application lifecycle abstractions (bootstrap, tick, shutdown) and surface them through `engine::core::application`.
   - Provide a bootstrap harness that wires the EnTT registry wrapper into the lifecycle so higher-level modules can register systems without bespoke glue.
2. **Author configuration and diagnostics services.**
   - Add a layered configuration source (defaults, file overrides, command line) with serialization hooks that can later be shared with tooling.
   - Stand up logging, metrics, and tracing endpoints under `engine::core::diagnostics` and document how other modules register providers.
3. **Plugin and runtime integration.**
   - Implement plugin discovery/loading that honours dependency ordering and exposes safe shutdown hooks.
   - Thread the application/configuration/diagnostics APIs into the runtime facade so scenarios in `engine/runtime` can exercise them end-to-end.
4. **Harden with coverage and examples.**
   - Expand the `engine/core/tests` suite to cover lifecycle error cases and configuration fallbacks.
   - Create documentation examples and profiling hooks demonstrating how downstream modules consume the new services.
