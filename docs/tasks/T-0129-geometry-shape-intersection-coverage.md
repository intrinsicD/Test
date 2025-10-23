# T-0129: Geometry Shape Intersection Test Coverage

## Goal
Complete the implementation of all declared pairwise shape intersection tests in `shape_interactions.hpp` to ensure full coverage for collision detection and spatial queries.

## Background
- `engine/geometry/include/engine/geometry/utils/shape_interactions.hpp` declares ~100+ pairwise intersection functions
- Not all declared functions are implemented - some may be stubs or missing
- Full intersection coverage is needed for:
  - Physics collision detection
  - Ray picking in tools
  - Visibility culling optimizations
  - Spatial query acceleration

## Current State
Investigation needed to determine:
- ✅ Which intersection tests are fully implemented
- ❌ Which are stubs (declared but return placeholder values)
- ❌ Which are complex and need advanced algorithms (GJK, SAT, etc.)

## Inputs
- Code: `engine/geometry/include/engine/geometry/utils/shape_interactions.hpp`, `engine/geometry/src/utils/shape_interactions.cpp`
- Tests: `engine/geometry/tests/test_shape_interactions.cpp`
- Reference: Real-Time Collision Detection (Christer Ericson), Game Physics Engine Development

## Deliverables
1. **Audit Report**: Document which tests are implemented vs. stubbed
2. **Implementation Priority Matrix**: Which tests are critical vs. optional
3. **Complete Implementations**: Implement missing high-priority tests
4. **Test Coverage**: Unit tests for all implemented intersection tests
5. **Documentation**: Usage examples and performance characteristics

## Intersection Test Matrix

### Critical Priority (Physics & Rendering)
- [x] AABB-AABB (likely implemented)
- [x] AABB-Sphere (likely implemented)
- [ ] AABB-Ray (needed for picking)
- [ ] AABB-Plane (needed for frustum culling)
- [x] Sphere-Sphere (likely implemented)
- [ ] Sphere-Ray (needed for picking)
- [ ] Triangle-Ray (critical for mesh picking)
- [ ] OBB-OBB (complex, needed for accurate collision)

### High Priority (Tools & Physics)
- [ ] Cylinder-Cylinder
- [ ] Cylinder-Sphere
- [ ] Capsule-Capsule (if adding capsule shape)
- [ ] Triangle-Triangle
- [ ] Segment-Sphere
- [ ] Segment-AABB

### Medium Priority (Specialized Cases)
- [ ] Ellipsoid-Ellipsoid
- [ ] Ellipsoid-Sphere
- [ ] OBB-Sphere
- [ ] Plane-Plane intersection (line result)

### Low Priority (Rare Cases)
- [ ] Line-Line (3D skew lines)
- [ ] Complex polyhedron tests

## Work Breakdown

### Phase 1: Audit (4-6 hours)
1. Review all declared functions in `shape_interactions.hpp`
2. Check corresponding implementations in `shape_interactions.cpp`
3. Identify stubs (functions that return false or placeholder values)
4. Classify by priority based on usage in physics/rendering/tools
5. Document findings in a coverage report

### Phase 2: Critical Implementations (20-30 hours)
Implement missing critical tests:

#### Ray-Triangle (Möller-Trumbore algorithm)
- Efficient single-sided test
- Optional backface culling
- Return barycentric coordinates

#### Ray-AABB (Slab method)
- Fast rejection test
- Return near/far intersection distances
- Handle edge cases (ray origin inside box)

#### OBB-OBB (Separating Axis Theorem)
- 15 potential separating axes
- Early exit optimization
- Return collision normal and depth (optional)

#### AABB-Plane
- Classify AABB against plane (in front, behind, intersecting)
- Used for frustum culling

### Phase 3: High Priority (15-20 hours)
- Cylinder-Cylinder: Capsule approximation or analytical test
- Triangle-Triangle: Edge-edge tests + coplanarity check
- Segment intersection tests: Various shapes

### Phase 4: Testing (10-15 hours)
- Unit tests for each implemented intersection
- Known hit/miss cases
- Edge cases (touching, degenerate shapes)
- Performance benchmarks

### Phase 5: Documentation (3-4 hours)
- Document which tests are implemented
- Performance characteristics table
- Usage examples for common cases
- Note which tests use conservative approximations

## Acceptance Criteria
- [ ] All critical-priority tests are implemented and tested
- [ ] At least 80% of high-priority tests are implemented
- [ ] Unit tests cover all implemented intersections
- [ ] Documentation clarifies implementation status
- [ ] Performance benchmarks show acceptable speed (<1μs for simple tests)
- [ ] No false negatives (missed intersections are bugs)
- [ ] Conservative false positives are acceptable for some approximations

## Priority
**Medium** - Not immediately blocking, but needed for:
- Complete physics system
- Tool ray picking
- Advanced culling optimizations

## Estimated Effort
**52-75 hours** total for complete coverage (can be split into phases)

**Phase 1 (Audit): 4-6 hours** - Should be done first to prioritize work

## Dependencies
- None (geometry module is independent)
- Enables full physics collision detection
- Enables complete tool picking system

## Related Tasks
- `T-0128`: Geometry frustum utilities (needs AABB-Plane, Sphere-Plane)
- `T-0117`: Physics contact manifolds (needs accurate collision tests)

## Notes
- Some complex tests (OBB-OBB, convex polyhedra) can use general algorithms:
  - **GJK (Gilbert-Johnson-Keerthi)**: General convex-convex distance
  - **SAT (Separating Axis Theorem)**: Polytope intersection
  - **EPA (Expanding Polytope Algorithm)**: Penetration depth
- Consider using SIMD for batch intersection tests
- Some tests may benefit from early-out spatial hashing
- Document which tests are exact vs. conservative approximations
- May want to provide both "fast" and "accurate" versions for some pairs

## Open Questions
- Should we implement GJK/EPA for general convex-convex tests?
- What's the tolerance for approximation in physics tests?
- Do we need swept/continuous collision detection variants?
- Should intersection tests return contact information (normal, depth)?
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
- Docs: [`docs/modules/geometry/README.md`](../modules/geometry/README.md)

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
- [ ] Implement `Frustum` struct in `shapes/frustum.hpp`
- [ ] Implement `ExtractFrustum()` for view-projection matrix
- [ ] Implement `GetCorners()` for frustum
- [ ] Add frustum-AABB intersection test (optimized, plane-AABB tests)
- [ ] Add frustum-Sphere intersection test
- [ ] Add frustum-OBB intersection test
- [ ] Add frustum-point containment test
- [ ] Add symmetric overloads for consistency
- [ ] Include frustum.hpp in shapes.hpp
- [ ] Write comprehensive unit tests
- [ ] Update geometry module documentation
- [ ] Benchmark intersection test performance

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
- [ ] Can extract frustum from any 4x4 view-projection matrix
- [ ] Frustum-AABB intersection is correct and fast (<100ns)
- [ ] Frustum-Sphere intersection is correct
- [ ] All intersection tests pass unit tests
- [ ] Documentation shows clear usage examples
- [ ] No dependencies on rendering module
- [ ] Compatible with existing shape API conventions

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

