# Core Module

## Current State

- Implements foundational services such as the entity registry wrapper and module naming API.
- Provides scaffolding for application lifecycle, configuration, diagnostics, and plugin subsystems.

## Usage

- Include `<engine/core/api.hpp>` and link against `engine_core`; the target inherits `engine::project_options` and exposes headers through `engine::headers` for downstream modules.
- Extend ECS features or add new core services in the dedicated subdirectories.
- Build and validate changes with the repository presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`, etc.).

## TODO / Next Steps

- Extend engine core services for application control, configuration, and diagnostics.
