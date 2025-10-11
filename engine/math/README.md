# Math Module

## Current State

- Wraps vector, matrix, and quaternion helpers that support animation, geometry, and physics subsystems.
- Acts as the shared numerical foundation for both native and Python bindings.
- Provides orthonormal basis construction via `engine::math::utils::orthonormal_basis` for tangent-frame derivation.

## Usage

- Include `<engine/math/math.hpp>` from consumers to access the linear algebra primitives.
- Link against `engine_math`; being an interface target it forwards `engine::project_options` and participates in `engine::headers` so downstream modules inherit the same compile features.
- Extend the math toolkit in tandem with new subsystem requirements and corresponding tests, validating with `ctest --preset <preset>`.

## TODO / Next Steps

1. **Stabilise the public surface.**
   - Catalogue the current vector, matrix, quaternion, transform, and utility facilities and capture invariants (storage order, handedness conventions, implicit normalisation rules) directly in the header `README.md` files.
   - Add Doxygen summaries for every public struct/function and wire those headers into the docs build so downstream teams can reason about ABI expectations before we introduce breaking changes.

2. **Raise coverage for the existing primitives.**
   - Expand the math test suite to cover the reflection/refraction helpers, matrix inversion edge cases (singular/pivoting scenarios), and transform decomposition/combination pipelines.
   - Integrate property-based checks (e.g. random orthonormal frames) to catch numerical drift across compilers and floating-point widths.

3. **Broaden the linear algebra toolset.**
   - Implement decomposition kernels frequently consumed by animation/physics (polar decomposition, QR/SVD sketches for $3\times3$ blocks, eigen-solvers for symmetric matrices) using numerically stable algorithms.
   - Introduce small, fixed-size LU/Cholesky routines to support constraint solvers in the physics module, together with benchmarks that compare them against the sparse backends.

4. **Improve numeric robustness and performance.**
   - Audit normalisation paths to eliminate silent divide-by-zero fallbacks, and surface explicit status channels (return `std::optional` or error enums) when operations fail.
   - Provide SIMD-specialised kernels (SSE4/AVX2/NEON) gated behind compile-time flags, along with golden-reference tests to keep the scalar implementations as baselines.

5. **Extend interoperability.**
   - Add conversion layers to and from the geometry module (bounding volumes, barycentric utilities) and ensure the Python bindings expose the same semantics.
   - Document the dependency graph in `docs/` and update the root backlog once milestones above graduate from planning to execution.
