# Engine Compute Tests

## Current State

- Contains automated tests covering Module.

## Usage

- Build the module with testing enabled and execute `ctest --test-dir build` to run this suite.
- Extend the cases alongside new runtime features to maintain behavioural coverage.

## TODO / Next Steps

- Capture fixtures that model heterogeneous dependency chains (CPU-only,
  GPU-only, and mixed) and assert deterministic execution ordering.
- Add failure-path regression tests that exercise invalid dependency graphs and
  resource lifetime violations once the API surfaces them.
- Integrate micro-benchmarks to watch for scheduler regressions as GPU paths
  come online.
