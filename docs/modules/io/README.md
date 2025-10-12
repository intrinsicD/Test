# IO Module

## Current State
- Detects geometry file formats and routes loading/saving through mesh, point cloud, and graph interfaces (`detect_geometry_file`, `load_geometry`, etc.).
- Exposes specialised read/write helpers per format (OBJ, PLY, OFF, STL, XYZ, PCD) and registers them through the geometry IO registry for lookup.
- Provides animation import scaffolding so clips can be loaded alongside geometry assets.
- Tests under `engine/io/tests/` validate detection, registry configuration, and animation import hooks.

## Usage
- Build with `cmake --build --preset <preset> --target engine_io`; this links against `engine_geometry` for the core data structures.
- Include `<engine/io/geometry_io.hpp>` for direct read/write helpers or `<engine/io/geometry_io_registry.hpp>` to inspect registered codecs.
- Run `ctest --preset <preset> --tests-regex engine_io` to ensure format handlers remain stable.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
