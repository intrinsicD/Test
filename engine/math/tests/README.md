# Engine Math Tests

## Current State

- Contains automated tests covering Math.

## Usage

- Build the module with testing enabled and execute `ctest --test-dir build` to run this suite.
- Extend the cases alongside new runtime features to maintain behavioural coverage.

## TODO / Next Steps

1. Build deterministic fixtures that validate reflection/refraction, projection, and transform composition pipelines across both single- and double-precision builds.
2. Add fuzz/property-based suites (e.g. with RapidCheck) that exercise random orthonormal bases and near-singular matrices to ensure inversion and decomposition code reports failure instead of producing NaNs.
3. Integrate performance regression checks that compare SIMD-enabled kernels with their scalar baselines so that math optimisations do not silently regress accuracy.
