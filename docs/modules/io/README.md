# IO Module

## Current State
- Detects geometry file formats via signature inspection and extension hints, routing loading/saving through mesh, point cloud, and graph interfaces (`detect_geometry_file`, `load_geometry`, etc.) while returning `GeometryIoResult<T>` with structured error codes instead of throwing exceptions.
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
- Enable fuzzing harnesses with `-DENGINE_ENABLE_FUZZING=ON` to build `engine_io_geometry_fuzz`. When compiled with `-fsanitize=fuzzer,address`, the harness exercises signature detection and all geometry loaders; without libFuzzer it can replay individual corpora by invoking the executable with a file path argument.

## Dependencies
- Requires the geometry module for canonical mesh, point cloud, and graph interfaces plus validation helpers; the IO registry marshals those types through the codec adapters.
- Relies on the C++17 `<filesystem>` library for staging temporary assets and integrates with `engine::platform::filesystem` utilities when generating transient working directories.

## TODO / Next Steps

- **RT-006:** Continue expanding detection heuristics, fuzzing coverage, and
  structured diagnostics under the [IO format detection hardening
  initiative](../../ROADMAP.md#rt-006-io-format-detection-hardening) while
  tracking progress in [ROADMAP.md](ROADMAP.md).
- **AI-002:** Provide streaming-friendly loaders and cancellation hooks that
  integrate with the asset caches per the [async asset streaming
  architecture](../../ROADMAP.md#ai-002-async-asset-streaming-architecture).
