# Engine Geometry Public Headers — Shapes

## Overview
Primitive volume types (e.g., `Aabb`, `Sphere`, `Obb`, `Cylinder`) backed by free-function utilities for intersection tests,
bounds construction, and sampling. These headers surface the stable ABI for geometry consumers; see the parent
[geometry module README](../../../README.md) for the broader roadmap and stability criteria.

## Current State (v0.9-beta)

**Stable primitives**
- `Aabb` — Axis-aligned bounding box
- `Sphere` — Spherical volume
- `Segment` — Line segment
- `Ray` — Half-infinite ray
- `Plane` — Infinite plane
- `Triangle` — 3D triangle

**Experimental primitives**
- `Obb` — Orientation representation being finalised (quaternion vs. matrix)
- `Cylinder` — End-cap handling under evaluation
- `Ellipsoid` — Orientation may switch to matrix form for consistency

Stability of these headers follows the geometry module milestones targeting v2.0. Experimental types may change layout until the
`MeshBounds` → `Aabb` consolidation and naming audit complete.

## Why a Dedicated Subdirectory?

- Keeps compile-time dependencies minimal: `shapes/` depends only on `engine::math`.
- Enables incremental ABI guarantees so consumers can rely on stable primitive layouts before heavier mesh utilities freeze.
- Supports selective inclusion for downstream modules that only require bounds/intersection helpers.

## Usage

```cpp
#include <engine/geometry/shapes/aabb.hpp>
#include <engine/geometry/shapes/sphere.hpp>

using namespace engine::geometry;

Sphere sphere{{0.0f, 0.0f, 0.0f}, 5.0f};
Aabb bounds = BoundingAabb(sphere);

auto centre = Center(bounds);
auto volume = Volume(bounds);
auto nearest = ClosestPoint(bounds, {10.0f, 0.0f, 0.0f});
```

## Design Principles
1. Prefer free functions over member methods for extensibility.
2. Maintain consistent naming across shapes (`ClosestPoint`, `SquaredDistance`, etc.).
3. Preserve value semantics (POD types) for ABI transparency.
4. Avoid virtual dispatch in performance-critical code paths.

## Relationship to the Geometry Module

```
engine/geometry/
├── include/engine/geometry/
│   ├── api.hpp                 # Mesh types, I/O helpers
│   ├── shapes/                 # This directory
│   ├── intersection/           # Shape-shape tests (planned)
│   └── spatial/                # Acceleration structures (planned)
└── src/
    ├── shapes/                 # Implementations
    ├── mesh/
    └── ...
```

Dependency flow:
- `shapes/` depends only on `engine::math`.
- Mesh APIs depend on `shapes/` for bounds definitions.
- Intersection and spatial modules consume `shapes/` types for queries and acceleration structures.

## Testing

```bash
ctest -R engine_geometry_shapes_tests
ctest -R engine_geometry_shapes_tests -V
```

Tests live under `engine/geometry/tests/shapes/`.

## ABI Compatibility
- Standard-layout types with explicit padding for alignment.
- No virtual functions or hidden allocations.
- Layout will freeze with the geometry v2.0 release; follow the [geometry README](../../../README.md) for status.

## TODO / Next Steps

Stay aligned with the geometry module backlog to keep documentation consistent.

### Immediate (v2.0 readiness)
- [ ] Finalise `Obb` orientation representation in tandem with the module-wide naming audit (#124).
- [ ] Document complexity/precondition notes for every public function once the API freezes.

### Short-term (post-freeze polish)
- [ ] Add per-shape usage examples covering collision, rendering culling, and deformation workflows.
- [ ] Integrate shape benchmarks into the automated performance harness alongside mesh metrics.

### Long-term (roadmap alignment)
- [ ] Introduce additional primitives (convex hull, frustum, capsule) as downstream modules require them.
- [ ] Cross-reference shapes documentation with collision detection algorithms and spatial structures once those READMEs land.
