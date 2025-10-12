# Assets Module

## Current State
- Exposes strongly typed `AssetHandle` wrappers for meshes, graphs, point clouds, textures, shaders, and materials to avoid accidental identifier mixing and provide filesystem-aware constructors.
- Provides caches for meshes, point clouds, graphs, textures, and shaders that track descriptors, last-write timestamps, and hot-reload callbacks while delegating format-aware loading to `engine::io` utilities.
- Defines asset descriptors that capture provenance, format hints, and binding metadata shared between caches and runtime consumers.
- Stores material assets as descriptor bindings (shader + texture handles); material authoring, serialization, and hot-reload remain TODO.
- Unit tests under `engine/assets/tests/` validate module registration, cache reload behaviour, and descriptor plumbing.

## Usage
- Build the target with `cmake --build --preset <preset> --target engine_assets`; this links against `engine_io` and transitively pulls in geometry readers.
- Include `<engine/assets/handles.hpp>` plus the relevant cache headers (`mesh_asset.hpp`, `point_cloud_asset.hpp`, `graph_asset.hpp`, `texture_asset.hpp`, `shader_asset.hpp`) to request loads; configure `engine_io` importers for the formats you rely on.
- Populate `MaterialCache` entries by constructing descriptors that bind preloaded shader/texture handles until material serialization lands.
- Execute `ctest --preset <preset> --tests-regex engine_assets` with testing enabled to validate cache behaviour after modifications.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
