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

The current implementation provides in-memory caches with hot-reload support, but the
module still lacks the surrounding content pipeline needed for real projects. The next
milestones should be tackled in the following order:

1. **Establish authoritative asset metadata.** Define per-asset descriptors that capture
   source provenance, transformation settings, dependency graphs, and runtime usage
   flags. The schema should integrate with the IO importers/exporters so that metadata
   remains round-trippable alongside raw files.
2. **Introduce staged import pipelines.** Build task graphs that invoke geometry, texture,
   and material importers, feed processed data into the existing caches, and emit build
   artifacts (e.g., CPU-friendly meshes, GPU-ready textures). Reuse the job system in
   `engine/core` and coordinate filesystem interactions through `engine/platform`.
3. **Persist and version cache artifacts.** Extend the caches with serialization hooks so
   that expensive imports can be reused across runs. Establish a versioning strategy that
   invalidates stale artifacts when source data or processing parameters change.
4. **Automate validation and coverage.** Author representative sample assets plus unit
   and integration tests that exercise the new pipeline stages, ensuring deterministic
   outputs and robust error handling.

Keep the root backlog entry in sync as these phases evolve.
