# Geometry Module

## Current State
- Implements half-edge and surface mesh data structures with conversion helpers, property registries, and IO pipelines for meshes, point clouds, and graphs.
- Provides spatial utilities including kd-trees, octrees, and intersection tests across a breadth of analytic shapes (`Sphere`, `Aabb`, `Capsule`, etc.).
- Ships procedural shape generators and sampling routines used by physics and runtime initialisation.
- Offers deformation helpers under `engine/geometry/deform/` that consume animation rig bindings and per-joint transforms to apply linear blend skinning to `SurfaceMesh` instances.
- Comprehensive unit tests in `engine/geometry/tests/` cover graph/mesh conversions,
  property storage, kd-tree behaviour, and shape interactions, with additional
  asset-driven round-trip coverage exercised by
  [`engine/tests/integration`](../../../engine/tests/integration/README.md) (`TI-001`),
  now running against the fixture-enabled Googletest toolchain introduced in
  `T-0118`.

## Usage
- Build via `cmake --build --preset <preset> --target engine_geometry`; this links against `engine_math`.
- Include `<engine/geometry/mesh/halfedge_mesh.hpp>`, `<engine/geometry/point_cloud/point_cloud.hpp>`, or related headers to manipulate geometry assets; IO helpers live in `<engine/geometry/export.hpp>` and `<engine/geometry/utils/...>`.
- Run `ctest --preset <preset> --tests-regex engine_geometry` with testing enabled to validate algorithms and conversions.

## Dependencies
- Consumes codecs and format detection from the IO module when persisting meshes, graphs, or point clouds; round-trip tests exercise the IO registry directly.
- Uses C++17 `<filesystem>` together with `engine::platform::filesystem::generate_random_suffix` to stage deterministic temporary paths for disk-based validation.

## TODO / Next Steps

- **RT-006:** Coordinate with IO to extend signature databases, fuzzing, and
  diagnostics described in the [IO format detection hardening plan](../../ROADMAP.md#rt-006-io-format-detection-hardening)
  so geometry round-trips stay lossless.
- **TI-002:** Add geometry-focused benchmarks and regression tracking for
  deformation, spatial queries, and property updates under the
  [performance benchmarking initiative](../../ROADMAP.md#ti-002-performance-benchmarks),
  mirroring details in [ROADMAP.md](ROADMAP.md).
