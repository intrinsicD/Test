# Geometry Module

## Current State

- Supplies a `SurfaceMesh` abstraction with helpers for bounds updates, centroid evaluation, and normals.
- Provides procedural generation for a unit quad to seed rendering and physics smoke tests.
- Implements spatial acceleration structures such as octrees and k-d trees for geometric queries.

## Usage

- Link against `engine_geometry` and include `<engine/geometry/api.hpp>` to manipulate meshes; the target inherits `engine::project_options` and participates in the shared `engine::headers` interface.
- Extend mesh processing algorithms under `src/` and pair them with tests in `tests/`.
- Exercise the module via the standard presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## TODO / Next Steps

- Introduce advanced mesh processing (remeshing, parameterization, collision prep).
