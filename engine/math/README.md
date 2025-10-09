# Math Module

## Current State

- Wraps vector, matrix, and quaternion helpers that support animation, geometry, and physics subsystems.
- Acts as the shared numerical foundation for both native and Python bindings.

## Usage

- Include `<engine/math/math.hpp>` from consumers to access the linear algebra primitives.
- Link against `engine_math`; being an interface target it forwards `engine::project_options` and participates in `engine::headers` so downstream modules inherit the same compile features.
- Extend the math toolkit in tandem with new subsystem requirements and corresponding tests, validating with `ctest --preset <preset>`.

## TODO / Next Steps

- Broaden math primitives and numerics beyond the current minimal set.
