# Math Module Roadmap

## Near Term
- Document invariants and numerical expectations for vector, matrix, quaternion, and transform helpers; augment unit tests with corner cases (degenerate transforms, precision thresholds).
- Add decomposition utilities (polar, QR) required by animation and physics subsystems, along with benchmarks.

## Mid Term
- Provide SIMD-specialised paths for hot operations (dot, cross, matrix multiply) with graceful fallbacks on scalar builds.
- Introduce fixed-size linear algebra solvers and factorizations to support simulation and rendering workloads.

## Long Term
- Build conversion utilities that translate math primitives to/from external formats (GLM, Eigen) and surface them through Python bindings for tooling integration.
