# Rendering Module

## Current State

- Implements a prototype frame graph and forward pipeline within the native renderer.
- Structures backend, resource, material, and pass directories for platform-specific integrations.

## Usage

- Link against `engine_rendering` to access render pass orchestration and frame graph primitives.
- Add backend-specific code under `backend/` and keep resource and material definitions in sync.

## TODO / Next Steps

- Connect the frame graph to concrete GPU backends and resource providers.
