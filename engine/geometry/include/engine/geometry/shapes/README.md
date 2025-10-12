# Engine Geometry Public Headers — Shapes

## Purpose
Provide primitive geometric volumes for collision detection, bounding operations, and spatial queries.

## Current State (v0.9-beta)

**Stable shapes**
- `Aabb` — Axis-aligned bounding box
- `Sphere` — Spherical volume
- `Segment` — Line segment
- `Ray` — Half-infinite ray
- `Plane` — Infinite plane
- `Triangle` — 3D triangle

**Experimental shapes**
- `Obb` — Oriented bounding box (orientation representation under review)
- `Cylinder` — Cylindrical volume (end-cap handling under evaluation)
- `Ellipsoid` — Ellipsoidal volume (orientation may switch to matrix form)

## Why a Dedicated Subdirectory?

Shapes are the foundation of the geometry module:
- Consumed by mesh processing for bounding volumes.
- Required by intersection testing utilities.
- Power spatial partitioning structures (BVH, octree).
- Depend only on `engine::math`, keeping compile times low and ABI stable.

Separating the headers keeps the API surface explicit, enables selective inclusion, and allows shapes to stabilise ahead of heavier geometry features.

## Usage

```cpp
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"

using namespace engine::geometry;

Sphere s{{0.0f, 0.0f, 0.0f}, 5.0f};
Aabb bounds = BoundingAabb(s);

auto centre = Center(bounds);
auto volume = Volume(bounds);
auto nearest = ClosestPoint(bounds, {10.0f, 0.0f, 0.0f});
```

## Design Principles
1. Prefer free functions over member methods for extensibility.
2. Maintain consistent naming across shapes (`ClosestPoint`, `SquaredDistance`, etc.).
3. Keep value semantics (POD types) for cheap copies and ABI stability.
4. Avoid virtual dispatch in performance-critical code paths.

## Relationship to the Geometry Module

```
engine/geometry/
├── include/engine/geometry/
│   ├── api.hpp                 # Mesh types, I/O helpers
│   ├── shapes/                 # This directory
│   │   ├── aabb.hpp
│   │   ├── sphere.hpp
│   │   └── ...
│   ├── intersection/           # Future: shape-shape tests
│   └── spatial/                # Future: BVH, octree
└── src/
    ├── shapes/                 # Implementations
    ├── mesh/
    └── ...
```

Dependency flow:
- `shapes/` depends only on `engine::math`.
- `api.hpp` (meshes) depends on `shapes/` for bounds.
- `intersection/` and `spatial/` consume `shapes/` types.

## Performance Notes
- Construction: $Ο(1)$
- `ClosestPoint`: $Ο(1)$ for all shapes
- `BoundingAabb`: $Ο(1)$ for primitives, $Ο(n)$ for point clouds
- `Merge`: $Ο(1)$

## Testing

```bash
ctest -R engine_geometry_shapes_tests
ctest -R engine_geometry_shapes_tests -V
```

Test cases live under `engine/geometry/tests/shapes/`.

## ABI Compatibility
- Standard-layout types with explicit padding for alignment.
- No virtual functions or hidden allocations.
- Layout frozen post-1.0 to maintain DLL compatibility via `ENGINE_GEOMETRY_API`.

## TODO / Next Steps

### Towards v1.0 (Stable Release)
- [ ] @alice — Finalise `Obb` orientation representation (quaternion vs. matrix). Due: 2026-01-15.
- [ ] @bob — Document all public functions with complexity and precondition notes. Due: 2026-01-31.
- [ ] @carol — Benchmark shape operations and highlight optimisation targets. Due: 2026-02-15.
- [ ] @dave — Write a shapes usage tutorial covering collision and rendering workflows. Due: 2026-02-28.

### Post-v1.0
- [ ] Add convex hull primitive.
- [ ] Introduce frustum shape for view culling.
- [ ] Implement capsule shape (currently approximated with cylinders).
