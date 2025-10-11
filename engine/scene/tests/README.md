# Engine Scene Tests

## Current State

- Contains automated tests covering component plumbing, module lifecycle, transform systems, and serialization round-trips.

## Usage

- Build the module with testing enabled and execute `ctest --test-dir build` to run this suite.
- Extend the cases alongside new runtime features to maintain behavioural coverage.

## TODO / Next Steps

- Grow scenario-based fixtures that validate complex hierarchy reparenting, bulk entity churn, and cross-component interactions.
- Add golden serialization assets to detect backward-compatibility breaks.
- Integrate performance-sensitive tests once traversal utilities land.
