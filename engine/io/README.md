# I/O Module

## Current State

- Establishes the directory layout for importers, exporters, caching, and runtime I/O infrastructure.
- Provides geometry I/O entry points that wrap format-specific helpers for OBJ/OFF/PLY meshes, PLY/XYZ/PCD point clouds, and
  edgelist/PLY graphs.
- Exposes a registry facade so formats can be supplied via plugins while the global accessor installs built-in ASCII handlers.
- Lacks persistence and residency policies; caching directories only contain scaffolding.

## Usage

- Implement specific format handlers under `importers/` and `exporters/` as pipelines solidify.
- Keep cache policies and runtime integration code in `src/` aligned with subsystem expectations.
- Link against `engine_io`; consumers inherit `engine::project_options` and obtain the exported headers via `engine::headers`.
- Leverage the shared presets to configure and test (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

These milestones coordinate with the content-pipeline initiatives outlined in the
[global roadmap](../../docs/global_roadmap.md).

1. **Harden geometry detection and parsing.**
   - Extend `detect_geometry_file` beyond extension heuristics by inspecting headers for ambiguous formats (binary PLY, STL
     signatures, compressed payloads).
   - Add binary decoding paths (binary PLY/PCD/STL) and richer attribute support (normals, colours, UVs) while keeping failure
     diagnostics precise.
   - Introduce fuzz and regression samples that exercise malformed files, large indices, and non-manifold elements.
2. **Modularise plugin registration.**
   - Break the current monolithic default registration into per-format units that can be compiled conditionally and reloaded at
     runtime.
   - Publish a small plugin authoring guide and validate hot-registration through unit tests that swap import/export handlers on
     the fly.
3. **Implement cache and streaming services.**
   - Design a content-addressable cache layered over the filesystem provider, including eviction policies and dependency
     tracking for derived assets.
   - Provide asynchronous streaming hooks that hydrate geometry on worker threads and surface progress callbacks to consumers.
4. **Integrate with the broader runtime.**
   - Thread the geometry registry through the asset pipeline so animation, rendering, and physics subsystems can request data in
     their native representations.
   - Record the build/runtime configuration knobs (search paths, cache roots, supported formats) in module documentation and
     expose coverage checks inside `engine/io/tests`.
