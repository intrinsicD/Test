# Engine Rendering Tests

## Current State

- Contains automated tests covering Forward Pipeline, Frame Graph, Module.
- Provides a recording GPU scheduler harness that validates submission ordering and synchronisation.

## Usage

- Build the module with testing enabled and execute `ctest --test-dir build` to run this suite.
- Extend the cases alongside new runtime features to maintain behavioural coverage.

## TODO / Next Steps

- Expand regression coverage beyond the current smoke checks.
- Add backend-specific conformance tests once hardware schedulers replace the stubs.
