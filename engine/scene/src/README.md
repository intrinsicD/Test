# Engine Scene Sources

## Current State

- Implements the module logic for the public API entry points and scene lifecycle.
- Registers transform systems on construction and pipes update calls through to the system layer.
- Hosts serialization helpers that marshal components to/from the streaming API.

## Usage

- Source files compile into `engine_scene`; ensure the build target stays warning-clean.
- Mirror API additions with implementation updates in this directory.

## TODO / Next Steps

- Build scenario-driven examples and profiling entry points to exercise large hierarchies.
- Layer in instrumentation that measures dirty-propagation and serialization performance.
- Keep implementation notes in sync with documentation as new component families land.
