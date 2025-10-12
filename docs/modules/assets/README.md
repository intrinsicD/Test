# Assets Module

## Current State
- Exposes strongly typed `AssetHandle` wrappers for meshes, graphs, point clouds, textures, shaders, and materials to avoid accidental identifier mixing and provide filesystem-aware constructors.
- Ships cache implementations (for example `MeshCache`) that track descriptors, file timestamps, and hot-reload callbacks while leveraging `engine::io` loaders to populate geometry payloads.
- Defines asset descriptors describing provenance and format hints so pipelines can communicate required import behaviour.
- Unit tests under `engine/assets/tests/` validate module registration and cache flows.

## Usage
- Build the target with `cmake --build --preset <preset> --target engine_assets`; this links against `engine_io` and transitively pulls in geometry readers.
- Include `<engine/assets/handles.hpp>` and the relevant asset headers to request cache loads; ensure `engine_io` importers are initialised for the formats you rely on.
- Execute `ctest --preset <preset> --tests-regex engine_assets` with testing enabled to validate cache behaviour after modifications.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
