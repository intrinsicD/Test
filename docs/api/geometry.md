# Geometry Module

The geometry module layers geometric primitives, spatial queries, and property management utilities on top
of the core math types. Public headers live under [`engine/geometry/include`](../../engine/geometry/include).

## Shape primitives
- [`engine/geometry/shapes/aabb.hpp`](../../engine/geometry/include/engine/geometry/shapes/aabb.hpp)
  introduces axis-aligned bounding boxes with helpers for bounding-volume hierarchy construction and
  intersection queries.
- [`engine/geometry/shapes.hpp`](../../engine/geometry/include/engine/geometry/shapes.hpp) aggregates the
  canonical shape definitions (spheres, cylinders, planes, triangles, lines, rays, and more) so call sites
  can pull the entire catalog with a single include.

## Interaction utilities
- [`engine/geometry/utils/shape_interactions.hpp`](../../engine/geometry/include/engine/geometry/utils/shape_interactions.hpp)
  enumerates robust intersection tests for every pair of supported shapes and exposes configurable epsilon
  tolerances for edge cases.
- [`engine/geometry/utils/connectivity.hpp`](../../engine/geometry/include/engine/geometry/utils/connectivity.hpp)
  and its companions provide mesh-centric traversal helpers, circulators, and range adaptors for
  half-edge based data structures.
- [`engine/geometry/random.hpp`](../../engine/geometry/include/engine/geometry/random.hpp) defines
  deterministic sampling APIs, making it trivial to scatter points or randomize bounding volumes.

## Property system
- [`engine/geometry/properties/property_set.hpp`](../../engine/geometry/include/engine/geometry/properties/property_set.hpp)
  and [`property_registry.hpp`](../../engine/geometry/include/engine/geometry/properties/property_registry.hpp)
  manage per-element attributes (vertices, edges, faces) with type-erased buffers and handle-based access.
  The registry underpins flexible mesh annotations used by topology and UV pipelines.

## Usage notes
- Each header follows a zero-allocation philosophy and interoperates directly with the math module’s
  fixed-size vectors and matrices.
- Intersection routines share result structures and tolerances, ensuring predictable behaviour for physics
  and collision detection systems.
- Property registries decouple mesh storage from user-defined attributes, so downstream algorithms can add
  or remove annotations without recompiling core containers.

## Surface mesh runtime helpers

`engine/geometry/api.hpp` now exposes a compact `SurfaceMesh` struct geared towards runtime deformation rather than
authoring. It stores both rest-state and deformed vertex positions, indices, normals, and cached bounds. Key helpers:

- `make_unit_quad()` – Generates a planar quad used by the runtime smoke test.
- `apply_uniform_translation(SurfaceMesh&, vec3)` – Applies rigid offsets while keeping the rest positions intact.
- `recompute_vertex_normals(SurfaceMesh&)` – Rebuilds area-weighted vertex normals after deformation.
- `update_bounds(SurfaceMesh&)` / `centroid(const SurfaceMesh&)` – Provide inexpensive spatial queries.

These functions allow the runtime to preview animation- and physics-driven deformation without touching the heavier
half-edge mesh infrastructure. They are intentionally header-only so lightweight tools (and the Python bindings) can
consume them with minimal dependencies.
