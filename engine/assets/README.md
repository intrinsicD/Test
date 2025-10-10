# Asset Module

## Current State

- Defines the staging area for runtime asset management across samples, shaders, and packaging.
- Provides runtime caches for geometry (meshes, point clouds, graphs) and rendering assets with hot-reload support.

## Usage

- Organise engine-owned assets here so runtime modules can locate them consistently.
- Keep asset metadata in sync with runtime expectations while pipelines mature.
- Link against `engine_assets`; it inherits `engine::project_options` usage requirements and exposes its headers through `engine::headers`.
- Configure and validate with the shared presets (for example `cmake --preset linux-gcc-debug` + `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Design asset pipelines that feed runtime modules beyond sample data.
