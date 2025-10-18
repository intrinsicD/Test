# IO Module

## Current State
- Detects geometry file formats and routes loading/saving through mesh, point cloud, and graph interfaces (`detect_geometry_file`, `load_geometry`, etc.), returning `GeometryIoResult<T>` with structured error codes instead of throwing exceptions.
- Exposes specialised read/write helpers per format (OBJ, PLY, OFF, STL, XYZ, PCD) and registers them through the geometry IO registry for lookup.
- Provides animation import scaffolding so clips can be loaded alongside geometry assets.
- Tests under `engine/io/tests/` validate detection, registry configuration, and
  animation import hooks, while the integration harness in
  [`engine/tests/integration`](../../../engine/tests/integration/README.md) verifies geometry
  assets survive round-tripping through the runtime cache (`TI-001`) now that the
  Googletest fixture upgrade in `T-0118` has landed.

## Usage
- Build with `cmake --build --preset <preset> --target engine_io`; this links against `engine_geometry` for the core data structures.
- Include `<engine/io/geometry_io.hpp>` for direct read/write helpers or `<engine/io/geometry_io_registry.hpp>` to inspect registered codecs. Recoverable failures are reported via `GeometryIoResult<T>` and `GeometryIoErrorCode`.
- Run `ctest --preset <preset> --tests-regex engine_io` to ensure format handlers remain stable.

## Dependencies
- Requires the geometry module for canonical mesh, point cloud, and graph interfaces plus validation helpers; the IO registry marshals those types through the codec adapters.
- Relies on the C++17 `<filesystem>` library for staging temporary assets and integrates with `engine::platform::filesystem` utilities when generating transient working directories.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
