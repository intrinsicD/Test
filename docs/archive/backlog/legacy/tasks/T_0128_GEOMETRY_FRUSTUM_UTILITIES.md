# T-0128: Geometry Frustum Utilities

## Goal
Add frustum extraction and frustum-shape intersection utilities to the geometry module to support the rendering visibility/culling system (T-0122).

## Background
- The geometry module already has shape primitives (AABB, OBB, Sphere, Plane, Ray, etc.) with intersection tests.
- The rendering visibility system (T-0122) needs frustum culling but frustum utilities don't exist yet.
- Frustum is a geometric primitive that should live in the geometry module, not rendering.
- Roadmap alignment: Required for `RT-002` rendering performance optimization.

## Current State
- ✅ Geometry module has: AABB, OBB, Sphere, Plane, Ray, Triangle, Cylinder, Ellipsoid
- ✅ Comprehensive pairwise intersection tests in `shape_interactions.hpp`
- ❌ No Frustum primitive
- ❌ No frustum extraction from view-projection matrices
- ❌ No frustum-AABB, frustum-Sphere intersection tests

## Inputs
- Code: `engine/geometry/include/engine/geometry/shapes/`, `engine/geometry/src/shapes/`
- Math: `engine/math/include/engine/math/` (matrix, vector utilities)
- Tests: `engine/geometry/tests/`
- Docs: [`docs/modules/geometry/README.md`](../../../../modules/geometry/README.md)

## Constraints
- Must match existing shape API conventions (see AABB, Plane, etc.)
- Should use engine::math types (mat4, vec3, vec4)
- Efficient intersection tests (will be called thousands of times per frame)
- Header-only where possible, implementation in .cpp for complex functions
- No rendering dependencies (geometry module is lower-level than rendering)

## Deliverables

### 1. Frustum Primitive
**File**: `engine/geometry/include/engine/geometry/shapes/frustum.hpp` + `.cpp`

```cpp
namespace engine::geometry {
    struct ENGINE_GEOMETRY_API Frustum {
        std::array<Plane, 6> planes; // left, right, bottom, top, near, far
    };

    // Extract frustum from view-projection matrix
    [[nodiscard]] ENGINE_GEOMETRY_API Frustum ExtractFrustum(const math::mat4& view_projection) noexcept;
    
    // Get frustum corners in world space
    [[nodiscard]] ENGINE_GEOMETRY_API std::array<math::vec3, 8> GetCorners(const Frustum& frustum) noexcept;
}
```

### 2. Frustum Intersection Tests
**File**: `engine/geometry/include/engine/geometry/utils/shape_interactions.hpp` (extend)

```cpp
// Add to existing shape_interactions.hpp:
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Frustum& frustum, const Aabb& aabb) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Frustum& frustum, const Sphere& sphere) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Frustum& frustum, const Obb& obb) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Frustum& frustum, const math::vec3& point) noexcept;

// Symmetric versions for consistency
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Aabb& aabb, const Frustum& frustum) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Sphere& sphere, const Frustum& frustum) noexcept;
[[nodiscard]] ENGINE_GEOMETRY_API bool Intersects(const Obb& obb, const Frustum& frustum) noexcept;
```

### 3. Update shapes.hpp
**File**: `engine/geometry/include/engine/geometry/shapes.hpp`

Add: `#include "engine/geometry/shapes/frustum.hpp"`

### 4. Unit Tests
**File**: `engine/geometry/tests/test_frustum.cpp` (new)

- Test frustum extraction from identity matrix
- Test frustum extraction from perspective projection
- Test frustum extraction from orthographic projection
- Test frustum-AABB intersection (inside, outside, intersecting)
- Test frustum-Sphere intersection
- Test point containment
- Test frustum corner calculation

### 5. Documentation
**File**: `docs/modules/geometry/README.md`

Add section on frustum utilities with examples.

## Checklist
- [x] Implement `Frustum` struct in `shapes/frustum.hpp`
- [x] Implement `ExtractFrustum()` for view-projection matrix
- [x] Implement `GetCorners()` for frustum
- [x] Add frustum-AABB intersection test (optimized, plane-AABB tests)
- [x] Add frustum-Sphere intersection test
- [x] Add frustum-OBB intersection test
- [x] Add frustum-point containment test
- [x] Add symmetric overloads for consistency
- [x] Include frustum.hpp in shapes.hpp
- [x] Write comprehensive unit tests
- [x] Update geometry module documentation
- [x] Benchmark intersection test performance

## Work Breakdown

### 1. Frustum Extraction (4-6 hours)
- Implement Gribb-Hartmann method for extracting planes from VP matrix
- Handle both row-major and column-major conventions correctly
- Normalize plane equations for signed distance tests
- Test with perspective and orthographic projections
- Verify near/far plane orientations

### 2. Intersection Tests (6-8 hours)
- **Frustum-AABB**: Test AABB against 6 planes (fast reject if outside any plane)
- **Frustum-Sphere**: Test sphere center distance to each plane
- **Frustum-OBB**: Transform frustum to OBB local space or use separating axis theorem
- **Frustum-Point**: Test point against all 6 planes
- Optimize common case (AABB) with SIMD if beneficial
- Handle edge cases (degenerate frustums, zero-size shapes)

### 3. Testing (4-5 hours)
- Unit tests for extraction correctness
- Intersection test correctness (known inside/outside cases)
- Edge cases (objects on frustum boundaries)
- Performance benchmarks (target: <50ns per frustum-AABB test)

### 4. Documentation (2 hours)
- Add frustum section to geometry README
- Code examples showing extraction and culling
- Cross-reference rendering visibility system (T-0122)

## Acceptance Criteria
- [x] Can extract frustum from any 4x4 view-projection matrix
- [x] Frustum-AABB intersection is correct and fast (<100ns)
- [x] Frustum-Sphere intersection is correct
- [x] All intersection tests pass unit tests
- [x] Documentation shows clear usage examples
- [x] No dependencies on rendering module
- [x] Compatible with existing shape API conventions

## Status
**COMPLETED** - 2025-10-24

## Implementation Summary
Successfully implemented frustum extraction and intersection tests:
- Added `Frustum` primitive with 6 planes structure
- Implemented Gribb-Hartmann extraction from view-projection matrices
- Added efficient frustum-AABB intersection (p-vertex/n-vertex test)
- Added frustum-Sphere intersection (signed distance test)
- Added frustum-OBB intersection (conservative bounding sphere test)
- Added frustum-point containment test
- Implemented symmetric overloads for API consistency
- Added GetCorners() utility for frustum visualization
- Created comprehensive test suite (16 tests covering all scenarios)
- Updated geometry module README with usage examples
- Added `geometry_frustum_culling` benchmark capturing 203 ns/test in Debug and 41 ns/test in Release for 200k AABBs over 256
  iterations, validating the <50 ns/test target.

All tests pass in both Debug and Release configurations.

## Priority
**High** - Blocks T-0122 (rendering visibility culling), which is critical for rendering performance.

## Estimated Effort
**16-21 hours** total across implementation, testing, and documentation.

## Dependencies
- Requires math module (already complete)
- Blocks T-0122 (rendering visibility culling system)

## Related Tasks
- `T-0122`: Rendering visibility culling system (depends on this)
- `MA-132`: Math convenience functions (independent)

## Notes
- Frustum extraction algorithms:
  - Gribb-Hartmann method (extract planes directly from matrix rows)
  - Alternative: Calculate from corner points
  - Both should be tested for correctness
- Intersection test optimization:
  - Early exit on first plane rejection for AABB
  - SIMD can process 4 planes simultaneously
  - Consider offering both "accurate" and "fast conservative" versions
- The geometry module is the right place for this - frustum is a geometric primitive like AABB or OBB
- Rendering module will use these utilities but shouldn't implement them

