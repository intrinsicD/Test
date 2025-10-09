# Runtime Module

## Current State

- Provides the aggregation layer that will stitch subsystems into an executable runtime.
- Includes scaffolding for lifecycle management and module loading.

## Usage

- Link against `engine_runtime` to bootstrap the engine once subsystems are wired together; the target inherits `engine::project_options` and relies on the header aggregator `engine::headers` to surface subsystem APIs.
- Expand runtime orchestration logic under `src/` and keep tests aligned with subsystem growth.
- Validate integrations with the shared presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) and extend coverage as new subsystems land.

## TODO / Next Steps

- Drive subsystem lifecycles and state management for end-to-end scenarios.
