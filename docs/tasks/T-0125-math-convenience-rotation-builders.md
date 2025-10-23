# T-0125: Math Convenience Rotation Matrix Builders

## Goal
Add convenience functions for single-axis rotation matrices to improve API ergonomics and reduce boilerplate when constructing common transformations.

## Background
- Current API requires using quaternions or angle-axis for rotations, which is verbose for simple axis-aligned rotations.
- Documentation examples reference `rotate_x`, `rotate_y`, `rotate_z` functions that don't exist in the implementation.
- Many graphics APIs and competing math libraries provide these as standard utilities.
- Roadmap alignment: Part of `MA-132` (API ergonomics improvements).

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
- Optional 3x3 variants if needed by consumers.
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
- `T-0126`: Documentation accuracy audit (align docs with implementation)

