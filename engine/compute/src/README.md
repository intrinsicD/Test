# Engine Compute Sources

## Current State

- Implements the module logic across Api.

## Usage

- Source files compile into `engine_compute`; ensure the build target stays warning-clean.
- Mirror API additions with implementation updates in this directory.

## TODO / Next Steps

- Prototype CPU vs. GPU parity tests that submit identical graphs to each backend
  and compare execution traces.
- Layer opt-in instrumentation hooks so that downstream tooling can emit timing
  markers without recompiling the module.
- Publish example dispatch graphs (smoke, stress, failure-injection) to guide
  integration and debugging.
