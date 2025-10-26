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
- [x] AABB-AABB
- [x] AABB-Sphere
- [x] AABB-Ray (needed for picking)
- [x] AABB-Plane (needed for frustum culling)
- [x] Sphere-Sphere
- [x] Sphere-Ray (needed for picking)
- [x] Triangle-Ray (critical for mesh picking)
- [x] OBB-OBB (complex, needed for accurate collision)

### High Priority (Tools & Physics)
- [x] Cylinder-Cylinder
- [x] Cylinder-Sphere
- [ ] Capsule-Capsule (if adding capsule shape)
- [x] Triangle-Triangle
- [x] Segment-Sphere
- [x] Segment-AABB

### Medium Priority (Specialized Cases)
- [x] Ellipsoid-Ellipsoid
- [x] Ellipsoid-Sphere
- [x] OBB-Sphere
- [x] Plane-Plane intersection (line result)

### Low Priority (Rare Cases)
- [x] Line-Line (3D skew lines)
- [ ] Complex polyhedron tests

## Audit Summary (2025-11-30)

| Pair | Implementation | Regression Coverage | Notes |
| --- | --- | --- | --- |
| AABB ↔ Cylinder | ✅ | `AabbIntersection.CylinderSymmetricParity` | Locks boolean symmetry across overload order. |
| AABB ↔ Ellipsoid | ✅ | `AabbIntersection.EllipsoidSymmetricParity` | Ensures both overloads agree on inside/outside classification. |
| AABB ↔ OBB | ✅ | `AabbIntersection.ObbSymmetricParity` | Guards broad-phase classification parity for oriented boxes. |
| AABB ↔ Plane | ✅ | `AabbIntersection.PlaneSymmetricParity` | Verifies plane clipping matches from either direction. |
| AABB ↔ Sphere | ✅ | `AabbIntersection.SphereSymmetricParity` | Prevents regressions in bounding-sphere culling helpers. |
| AABB ↔ Triangle | ✅ | `AabbIntersection.TriangleSymmetricParity` | Protects mesh picking routines relying on either call order. |

All of the above pairs previously lacked explicit regression tests for the reversed overload. The new parity checks accompany the existing interval comparisons for line, ray, and segment queries to complete AABB coverage for critical physics/rendering paths.

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

## Progress
- 2025-11-20: Added regression tests covering symmetric cylinder intersection overloads (line, ray, segment, OBB, ellipsoid, sphere, triangle) and verified `Result` interval parity, ensuring both argument orders remain equivalent.
- 2025-11-21: Extended parity regression coverage to ellipsoid line, ray, and segment intersections to protect `Result` interval symmetry in both invocation orders.
- 2025-11-22: Added AABB line, ray, and segment symmetry tests to confirm shared interval computations remain identical regardless of argument order, preventing regressions in broad-phase culling routines.
- 2025-11-23: Added OBB line, ray, and segment symmetry regression tests to lock identical interval outputs across both argument orders and guard oriented-box culling routines against parity regressions.
- 2025-11-27: Added plane and sphere intersection symmetry regression tests covering line, ray, and segment overloads to ensure `Result` intervals remain consistent across argument orderings.
- 2025-11-28: Added triangle line, ray, and segment symmetry regression tests to lock `Result` parameter parity across both invocation orders and extend coverage to the remaining ray/segment triangle overloads.
- 2025-11-29: Added line-ray, line-segment, and ray-segment symmetric regression tests comparing intersection points to ensure argument-order conversions remain consistent for mixed parameterizations.
- 2025-11-30: Added boolean symmetry regression tests for AABB pairings (cylinder, ellipsoid, OBB, plane, sphere, triangle) to guarantee consistent broad-phase classifications regardless of overload order and updated the coverage matrix to reflect the completed audit.
- 2025-12-02: Added plane intersection symmetry tests for cylinder, ellipsoid, OBB, sphere, and triangle pairings to lock boolean parity across overload orders and extend coverage to planform classifiers used by rendering and physics culling paths.
- 2025-12-03: Upgraded frustum–OBB intersection to use oriented extent projections and added dedicated tests (inside/outside/intersecting + symmetric parity) to remove the bounding-sphere approximation and tighten rendering culling results.
