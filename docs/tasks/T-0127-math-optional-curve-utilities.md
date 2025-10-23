# T-0127: Math Optional Curve/Spline Utilities

## Goal
Evaluate and potentially implement curve/spline mathematics for animation interpolation and tool support, or explicitly document that these belong in the animation module.

## Background
- Common game engine math libraries include Bezier, Catmull-Rom, and Hermite curve utilities.
- These are typically used for animation curves, camera paths, and procedural generation.
- Currently unclear if animation module has these or if they should be in math module.
- Need architectural decision: low-level math primitives vs. high-level animation features.
- Related to `RT-001` (animation deformation pipeline) and potential future curve editor tools.

## Decision Required
**Where should curve mathematics live?**

### Option A: Math Module (Pure Mathematics)
- Bezier curve evaluation (de Casteljau algorithm)
- Hermite interpolation
- Catmull-Rom splines
- B-splines basis functions
- Pure math functions, no animation-specific logic

### Option B: Animation Module (Application-Specific)
- Animation curves with keyframes
- Tangent computation
- Curve editor data structures
- Evaluation caching
- Time-domain specific features

### Option C: Both (Layered Approach)
- Math module: Pure evaluation functions
- Animation module: Keyframe management, tangent handles, editor integration

## Investigation Required
1. **Survey animation module** - Does it already have curve support?
2. **Survey geometry module** - Are splines needed for procedural mesh generation?
3. **Survey tools** - Do we need curve editing capabilities?
4. **Check physics module** - Are splines needed for motion paths?

## Potential Deliverables (if implemented in math)
- Bezier curve evaluation templates for cubic/quadratic curves
- Catmull-Rom interpolation functions
- Hermite spline utilities
- Arc-length parameterization helpers
- Curve derivative and tangent computation

## Checklist (Investigation Phase)
- [ ] Survey `engine/animation/` for existing curve support
- [ ] Check `engine/geometry/` for spline usage
- [ ] Review `engine/tools/` for curve editor requirements
- [ ] Consult with animation module owners on needs
- [ ] Document architectural decision in ADR

## Checklist (Implementation Phase - if approved)
- [ ] Implement Bezier evaluation (quadratic and cubic)
- [ ] Implement Catmull-Rom interpolation
- [ ] Implement Hermite interpolation
- [ ] Add arc-length parameterization utilities
- [ ] Write comprehensive unit tests
- [ ] Add usage examples to documentation
- [ ] Benchmark evaluation performance

## Work Breakdown (if implemented)
1. **Investigation** (2-3 hours)
   - Survey existing modules
   - Identify actual requirements
   - Draft architectural decision

2. **Core Implementation** (8-12 hours)
   - Bezier curves (cubic and quadratic)
   - Catmull-Rom splines
   - Hermite interpolation
   - Template design for generic scalar types

3. **Advanced Features** (6-8 hours, optional)
   - Arc-length parameterization
   - Derivative computation
   - Adaptive subdivision
   - Closest point queries

4. **Testing** (4-6 hours)
   - Unit tests for each curve type
   - Numerical accuracy tests
   - Edge case handling (collinear points, etc.)

5. **Documentation** (2-3 hours)
   - API reference
   - Usage examples
   - Performance characteristics

## Acceptance Criteria (if implemented)
- [ ] Can evaluate Bezier curves at arbitrary parameter t
- [ ] Catmull-Rom interpolation produces smooth curves through control points
- [ ] Hermite interpolation respects tangent constraints
- [ ] Tests verify mathematical correctness
- [ ] Documentation explains when to use each curve type
- [ ] Performance is acceptable for real-time use

## Priority
**Optional/Future** - This is not blocking any current work. Should only be implemented if there's clear demand from animation, tools, or geometry modules.

## Estimated Effort
**20-30 hours** for full implementation, or **2-3 hours** for investigation and documentation decision.

## Recommendation
**Start with investigation phase only.** If animation module already has sufficient curve support, document that clearly and close this task. Only proceed with math module implementation if there's demonstrated need for low-level curve primitives used by multiple modules.

## Related Tasks
- `T-0113`: Animation runtime skinning (may reveal curve requirements)
- `RT-001`: Animation deformation pipeline (check for curve interpolation needs)

## Notes
- Unity and Unreal keep curve utilities in animation/editor systems, not core math
- GLM and Eigen don't include curve mathematics - typically separate libraries
- Consider whether curves belong in math, animation, or a future "curves" module
# T-0125: Math Convenience Rotation Matrix Builders

## Goal
Add convenience functions for single-axis rotation matrices to improve API ergonomics and reduce boilerplate when constructing common transformations.

## Background
- Current API requires using quaternions or angle-axis for rotations, which is verbose for simple axis-aligned rotations.
- Documentation examples reference `rotate_x`, `rotate_y`, `rotate_z` functions that don't exist in the implementation.
- Many graphics APIs and competing math libraries provide these as standard utilities.
- Roadmap alignment: Part of `MA-130` (API ergonomics improvements).

## Inputs
- Code: `engine/math/include/engine/math/matrix.hpp`, `engine/math/include/engine/math/utils/utils_rotation.hpp`
- Tests: `engine/math/tests/test_math.cpp`
- Docs: [`docs/modules/math/README.md`](../modules/math/README.md)

## Constraints
- Must maintain consistency with existing rotation conventions (right-handed, column-major).
- Should use the same numerical precision as existing rotation utilities.
- No performance regression vs. current quaternion-based approach.
- Header-only implementation to match existing math module design.

## Deliverables
- `rotate_x(T angle)`, `rotate_y(T angle)`, `rotate_z(T angle)` functions returning `Matrix<T, 4, 4>`.
- Optional 3x3 variants: `rotate_x_3(T angle)`, etc.
- Unit tests covering basic rotations (90°, 180°, 270°) and composition.
- Documentation updates removing aspirational examples and adding actual API.

## Checklist
- [ ] Implement `rotate_x`, `rotate_y`, `rotate_z` in `utils/utils_rotation.hpp`.
- [ ] Add 3x3 variants if needed by consumers.
- [ ] Write unit tests verifying rotation correctness and composition order.
- [ ] Update `docs/modules/math/README.md` to reflect actual API.
- [ ] Add usage examples in documentation.
- [ ] Verify no performance regression with benchmark comparisons.

## Work Breakdown
1. **Implementation** (2-3 hours)
   - Add functions to `utils_rotation.hpp` using sin/cos directly.
   - Follow existing `to_rotation_matrix(angle, axis)` pattern.
   - Ensure proper matrix element ordering for column-major layout.

2. **Testing** (1-2 hours)
   - Test identity cases (angle = 0).
   - Test basic angles (90°, 180°, 270°).
   - Test composition: `rotate_x * rotate_y` matches combined quaternion.
   - Test equivalence: `rotate_x(θ)` matches `to_rotation_matrix(θ, {1,0,0})`.

3. **Documentation** (1 hour)
   - Update README with correct function signatures.
   - Add code examples showing simple rotations.
   - Remove aspirational examples that don't match implementation.

## Acceptance Criteria
- [ ] Can construct X/Y/Z rotation matrices with single function call.
- [ ] Tests verify mathematical correctness against quaternion path.
- [ ] Documentation accurately reflects implemented API.
- [ ] No breaking changes to existing code.

## Priority
**Low** - This is a convenience feature that improves ergonomics but doesn't block any critical functionality. The current quaternion-based API is fully functional.

## Estimated Effort
**4-6 hours** total across implementation, testing, and documentation.

## Related Tasks
- `MA-131`: Documentation accuracy audit (align docs with implementation)

