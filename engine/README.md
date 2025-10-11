# Engine Modules

## Current State

- Aggregates all native subsystems including animation, rendering, physics, geometry, and platform glue.
- Each subdirectory builds as an independent static/shared library that contributes to the runtime.

## Usage

- Add new subsystems as sibling directories with their own CMake targets and README following the template.
- Include this directory from the root `CMakeLists.txt` to expose all engine libraries.
- Link against the generated `engine_<subsystem>` target; the root build injects `engine::project_options` (compile features) and publishes the headers through `engine::headers`.

## TODO / Next Steps

- Stabilise cross-module integration and build targets for contributors while keeping the
  per-module plans aligned with the [global roadmap](../docs/global_roadmap.md).
