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
  which is blocked on Googletest fixture support (`T-0118`).

## Usage
- Build via `cmake --build --preset <preset> --target engine_geometry`; this links against `engine_math`.
- Include `<engine/geometry/mesh/halfedge_mesh.hpp>`, `<engine/geometry/point_cloud/point_cloud.hpp>`, or related headers to manipulate geometry assets; IO helpers live in `<engine/geometry/export.hpp>` and `<engine/geometry/utils/...>`.
- Run `ctest --preset <preset> --tests-regex engine_geometry` with testing enabled to validate algorithms and conversions.

## Dependencies
- Consumes codecs and format detection from the IO module when persisting meshes, graphs, or point clouds; round-trip tests exercise the IO registry directly.
- Uses C++17 `<filesystem>` together with `engine::platform::filesystem::generate_random_suffix` to stage deterministic temporary paths for disk-based validation.

## Roadmap
- See [ROADMAP.md](ROADMAP.md) for upcoming work.
