# Engine Modules

## Current State

- Aggregates all native subsystems including animation, rendering, physics, geometry, and platform glue.
- Each subdirectory builds as an independent static/shared library that contributes to the runtime.

## Usage

- Add new subsystems as sibling directories with their own CMake targets and README following the template.
- Include this directory from the root `CMakeLists.txt` to expose all engine libraries.

## TODO / Next Steps

- Stabilise cross-module integration and build targets for contributors.
