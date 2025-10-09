# Rendering Module

## Current State

- Implements a prototype frame graph and forward pipeline within the native renderer.
- Structures backend, resource, material, and pass directories for platform-specific integrations.

## Usage

- Link against `engine_rendering` to access render pass orchestration and frame graph primitives; the target inherits `engine::project_options` and shares headers via `engine::headers`.
- Add backend-specific code under `backend/` and keep resource and material definitions in sync.
- Exercise the renderer with the standard presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`) to ensure dependencies resolve consistently across toolchains.

## TODO / Next Steps

- Connect the frame graph to concrete GPU backends and resource providers.
