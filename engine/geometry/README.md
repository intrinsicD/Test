# Geometry Module

## Current State

- Supplies a `SurfaceMesh` abstraction with helpers for bounds updates, centroid evaluation, and normals.
- Provides procedural generation for a unit quad to seed rendering and physics smoke tests.
- Implements spatial acceleration structures such as octrees and k-d trees for geometric queries.

## Usage

- Link against `engine_geometry` and include `<engine/geometry/api.hpp>` to manipulate meshes; the target inherits `engine::project_options` and participates in the shared `engine::headers` interface.
- Extend mesh processing algorithms under `src/` and pair them with tests in `tests/`.
- Exercise the module via the standard presets (`cmake --preset linux-gcc-debug`, `ctest --preset linux-gcc-debug`).

## Proposed Roadmap

### 1. Stabilise Core Data Structures

- Implement the declared read/write entry points for graphs, point clouds, and halfedge meshes so that the property-backed
  containers can round-trip standard formats (OBJ/PLY) and feed the rest of the engine without bespoke loaders. The
  interfaces already expose `friend` hooks for this work; wiring them up in `src/graph`, `src/mesh`, and `src/point_cloud`
  should be the first milestone.
- Provide conversion utilities that translate between the light-weight `SurfaceMesh` used by the public API and the richer
  halfedge mesh implementation. This enables reuse of topology-aware algorithms while keeping the simple struct convenient
  for tooling.
- Audit the existing acceleration structures (k-d tree, octree) and formalise a shared data layout for spatial indices so
  that they ingest either raw arrays or property handles without duplication.

### 2. Algorithmic Expansion

- Add mesh cleanup passes (duplicate vertex welding, degenerate face pruning) and curvature/feature estimators to feed later
  remeshing and UV workflows.
- Implement adaptive remeshing and parameterisation pipelines (edge collapse/flip/split operations, Loop subdivision,
  harmonic or LSCM UV solves) on top of the halfedge core.
- Extend point-cloud tooling with surface reconstruction (Poisson/ball-pivoting) to bridge scanned data into the mesh
  pipeline.

### 3. Integration, Tooling, and Tests

- Record benchmark scenes and regression datasets that exercise the new import/export and meshing routines, and grow the
  GoogleTest suite accordingly.
- Surface profiling hooks and statistics so the runtime can monitor build times, vertex/face counts, and memory usage when
  geometry assets are loaded.
- Update the module and top-level READMEs whenever features land, keeping the roadmap aligned with the aggregated project
  backlog.
