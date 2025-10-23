# T-0126: Math Module Documentation Alignment

## Goal
Align math module documentation with actual implementation by removing aspirational API examples and ensuring all documented functions exist in the codebase.

## Background
- `docs/modules/math/README.md` contains examples of functions that don't exist in the implementation.
- Users following documentation will encounter compilation errors.
- Documentation was likely written ahead of implementation or copied from other libraries.
- This creates confusion and reduces trust in documentation accuracy.
- Part of broader documentation quality initiative (`MA-131`).

## Inputs
- Docs: [`docs/modules/math/README.md`](../modules/math/README.md)
- Code: `engine/math/include/engine/math/*.hpp`
- Reference: Existing test suite showing actual API usage

## Issues Found

### Functions Referenced but Not Implemented
1. `rotate_x()`, `rotate_y()`, `rotate_z()` - See T-0125 for implementation
2. `transform_point()`, `transform_direction()` - Use Transform API or matrix multiply directly
3. `nlerp()` for quaternions - Have `slerp()` and `squad()` but not normalized lerp
4. Math constants as variables (`math::pi`, `math::two_pi`) - Use `std::numbers::pi_v<T>`
5. Component-wise comparison functions (`all()`, `any()`, `greater_than()`)
6. Geometric primitives (AABB, Sphere, Ray, Plane) - These belong in geometry module

### Actual API That Should Be Documented Better
1. Camera utilities (`perspective`, `orthographic`, `look_at`) exist but examples are incomplete
2. Transform API (`combine`, `inverse`, `transform_point`, `transform_vector`) exists but not well documented
3. Solver functions exist but examples are minimal
4. Sparse matrix functionality is fully implemented but barely mentioned

## Deliverables
- Accurate documentation reflecting actual implementation
- Clear notes about where to find features (geometry module for shapes, etc.)
- Examples using actual function signatures and namespaces
- Updated quick reference table showing what's in math vs. other modules

## Checklist
- [ ] Audit all code examples in README for compilation
- [ ] Replace non-existent functions with actual equivalents
- [ ] Document existing Transform API properly
- [ ] Add section clarifying module boundaries (math vs. geometry vs. animation)
- [ ] Update camera utility examples to show actual usage
- [ ] Document sparse matrix capabilities
- [ ] Add note about using `std::numbers` for constants
- [ ] Cross-reference geometry module for shape types

## Work Breakdown
1. **Audit** (2 hours)
   - Extract all function references from documentation
   - Compile list of what exists vs. what doesn't
   - Identify actual API that's underdocumented

2. **Rewrite Examples** (3-4 hours)
   - Replace non-existent functions with working equivalents
   - Test all code examples for compilation
   - Add clarifying comments about module boundaries

3. **Expansion** (2-3 hours)
   - Document Transform API thoroughly
   - Add sparse matrix usage examples
   - Document camera utilities with complete signatures

4. **Review** (1 hour)
   - Verify all examples compile
   - Check cross-references to other modules
   - Ensure consistency with SOLVER_STABILITY.md and FORMAT_CONVERSIONS.md

## Acceptance Criteria
- [ ] All code examples in documentation compile without modification
- [ ] No references to non-existent functions without clear notes
- [ ] Users can distinguish what's in math module vs. other modules
- [ ] Transform API is clearly documented with examples
- [ ] Camera utilities and sparse matrices have usage examples

## Priority
**Medium** - Documentation accuracy is important for usability, but the implementation itself is solid and functional.

## Estimated Effort
**8-10 hours** total for thorough audit and rewrite.

## Related Tasks
- `T-0125`: Implement missing convenience functions (optional - can document workarounds instead)
- `MA-130`: Conversion drift diagnostics (ensure docs stay in sync with telemetry work)

## Notes
- Should coordinate with geometry module documentation to clarify boundaries
- Consider adding a "Common Pitfalls" or "Migration from Other Libraries" section
- The existing SOLVER_STABILITY.md and FORMAT_CONVERSIONS.md are excellent - use as quality bar

