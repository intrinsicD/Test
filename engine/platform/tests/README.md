# Engine Platform Tests

## Current State

- Contains automated tests covering module exports, window console behaviour, and
  the input state tracker.

## Usage

- Build the module with testing enabled and execute `ctest --test-dir build` to
  run this suite.
- Extend the cases alongside new runtime features to maintain behavioural coverage.

## TODO / Next Steps

- Expand regression coverage beyond the current smoke checks.
