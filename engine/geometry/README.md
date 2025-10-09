# Geometry Module

## Current State

- Supplies a `SurfaceMesh` abstraction with helpers for bounds updates, centroid evaluation, and normals.
- Provides procedural generation for a unit quad to seed rendering and physics smoke tests.

## Usage

- Link against `engine_geometry` and include `<engine/geometry/api.hpp>` to manipulate meshes.
- Extend mesh processing algorithms under `src/` and pair them with tests in `tests/`.

## TODO / Next Steps

- Introduce advanced mesh processing (remeshing, parameterization, collision prep).
