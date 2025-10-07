# Math

_Path: `engine/math`_

_Last updated: 2025-02-14_


## Overview

The math module defines the numeric building blocks used throughout the engine. It currently ships a header-only linear
algebra layer with:

- Fixed-size `Vector`/`Matrix` templates with SIMD-friendly layouts and aliases (`vec3`, `mat4`, ...).
- Quaternion primitives with conversions to and from angle-axis and Cayley parameterisations.
- Affine `Transform` structures combining scale, rotation, and translation along with helpers for composition and
  inversion.
- Camera utilities (projection matrices, view transforms) and rotation helpers shared with the rendering stack.

All public headers live under `include/engine/math/`. The accompanying GoogleTest suite (`tests/test_math.cpp`)
validates vector algebra, quaternion conversions, and transform matrix round-trips.

## Usage

Add the `engine_math` target to your CMake project and include the headers directly:

```cmake
target_link_libraries(my_app PRIVATE engine_math)
```

```cpp
#include <engine/math/vector.hpp>
#include <engine/math/transform.hpp>

engine::math::vec3 velocity{1.0f, 0.0f, 0.0f};
auto position = engine::math::transform_point(transform, velocity);
```
