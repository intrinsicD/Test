# Math Module

## Overview

The math module provides header-only vector, matrix, quaternion, and transform types optimized for graphics and physics applications. It serves as the foundation for all geometric computations across animation, physics, geometry, and rendering modules.

## Core Types

### Vectors

```cpp
#include "engine/math/math.hpp"

// 2D vectors
math::vec2 v2{1.0f, 2.0f};
math::ivec2 iv2{10, 20};  // Integer vector

// 3D vectors
math::vec3 v3{1.0f, 2.0f, 3.0f};
math::vec3 forward{0.0f, 0.0f, 1.0f};

// 4D vectors (homogeneous coordinates)
math::vec4 v4{1.0f, 2.0f, 3.0f, 1.0f};

// Component access
float x = v3.x;  // or v3[0]
float y = v3.y;  // or v3[1]
float z = v3.z;  // or v3[2]
```

### Matrices

```cpp
// 4x4 matrices (column-major)
math::mat4 identity = math::mat4::identity();
math::mat4 m = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

// 3x3 matrices (often for rotation/normal transforms)
math::mat3 rotation = math::mat3::identity();

// Matrix multiplication
math::mat4 result = m1 * m2;

// Access elements
float m00 = m[0][0];  // Column 0, row 0
```

### Quaternions

```cpp
// Quaternions for rotations (w, x, y, z)
math::quat identity{1.0f, 0.0f, 0.0f, 0.0f};

// From axis-angle
math::vec3 axis{0.0f, 1.0f, 0.0f};
float angle = math::radians(90.0f);
math::quat rotation = math::quat::from_axis_angle(axis, angle);

// From euler angles
math::quat euler_rotation = math::quat::from_euler(
    math::radians(45.0f),  // pitch
    math::radians(90.0f),  // yaw
    math::radians(0.0f)    // roll
);

// Quaternion operations
math::quat q1, q2;
math::quat combined = q1 * q2;  // Composition
math::quat inverse = math::inverse(q1);
math::quat normalized = math::normalize(q1);

// Spherical linear interpolation
math::quat interpolated = math::slerp(q1, q2, 0.5f);
```

### Transforms

```cpp
// Transform with translation, rotation, scale
math::Transform<float> transform;
transform.translation = {1.0f, 2.0f, 3.0f};
transform.rotation = math::quat::from_euler(0.0f, math::radians(45.0f), 0.0f);
transform.scale = {1.0f, 1.0f, 1.0f};

// Convert to matrix
math::mat4 matrix = transform.to_matrix();

// Combine transforms (child relative to parent)
math::Transform<float> world_transform = parent * child;

// Inverse transform
math::Transform<float> inv = math::inverse(transform);
```

## Vector Operations

### Basic Operations

```cpp
math::vec3 a{1.0f, 2.0f, 3.0f};
math::vec3 b{4.0f, 5.0f, 6.0f};

// Arithmetic
math::vec3 sum = a + b;
math::vec3 diff = a - b;
math::vec3 scaled = a * 2.0f;
math::vec3 component_mul = a * b;  // Component-wise

// Dot product
float dot = math::dot(a, b);

// Cross product (3D only)
math::vec3 cross = math::cross(a, b);

// Length
float len = math::length(a);
float len_squared = math::length_squared(a);  // Faster (no sqrt)

// Normalization
math::vec3 normalized = math::normalize(a);

// Distance
float dist = math::distance(a, b);

// Lerp
math::vec3 interpolated = math::lerp(a, b, 0.5f);
```

### Advanced Operations

```cpp
// Reflection
math::vec3 normal{0.0f, 1.0f, 0.0f};
math::vec3 incident{1.0f, -1.0f, 0.0f};
math::vec3 reflected = math::reflect(incident, normal);

// Clamp
math::vec3 clamped = math::clamp(v, min_vec, max_vec);

// Min/max
math::vec3 minimum = math::min(a, b);
math::vec3 maximum = math::max(a, b);

// Component-wise operations
math::vec3 absolute = math::abs(v);
math::vec3 floored = math::floor(v);
math::vec3 ceiled = math::ceil(v);
```

## Matrix Operations

### Construction

```cpp
// Translation matrix
math::mat4 translation = math::translate(math::vec3{1.0f, 2.0f, 3.0f});

// Rotation matrices
math::mat4 rotate_x = math::rotate_x(math::radians(45.0f));
math::mat4 rotate_y = math::rotate_y(math::radians(90.0f));
math::mat4 rotate_z = math::rotate_z(math::radians(30.0f));

// Scale matrix
math::mat4 scale = math::scale(math::vec3{2.0f, 2.0f, 2.0f});

// Look-at matrix (view matrix)
math::mat4 view = math::look_at(
    eye_position,
    target_position,
    up_vector
);

// Perspective projection
math::mat4 proj = math::perspective(
    math::radians(60.0f),  // FOV
    aspect_ratio,
    near_plane,
    far_plane
);

// Orthographic projection
math::mat4 ortho = math::ortho(left, right, bottom, top, near, far);
```

### Operations

```cpp
// Transpose
math::mat4 transposed = math::transpose(m);

// Inverse
math::mat4 inverted = math::inverse(m);

// Determinant
float det = math::determinant(m);

// Transform point (w=1)
math::vec3 point{1.0f, 2.0f, 3.0f};
math::vec3 transformed = math::transform_point(m, point);

// Transform direction (w=0)
math::vec3 direction{0.0f, 1.0f, 0.0f};
math::vec3 transformed_dir = math::transform_direction(m, direction);
```

## Quaternion Operations

### Conversion

```cpp
// Quaternion to matrix
math::mat4 matrix = math::to_matrix(quat);
math::mat3 matrix3 = math::to_matrix3(quat);

// Matrix to quaternion
math::quat q = math::to_quaternion(matrix);

// To axis-angle
auto [axis, angle] = math::to_axis_angle(quat);

// To euler angles
auto [pitch, yaw, roll] = math::to_euler(quat);
```

### Interpolation

```cpp
// Linear interpolation (not recommended for rotations)
math::quat lerp_result = math::lerp(q1, q2, t);

// Spherical linear interpolation (preferred)
math::quat slerp_result = math::slerp(q1, q2, t);

// Normalized lerp (faster approximation)
math::quat nlerp_result = math::nlerp(q1, q2, t);
```

## Utility Functions

### Angles

```cpp
// Degree/radian conversion
float radians = math::radians(180.0f);  // π
float degrees = math::degrees(math::pi);  // 180.0f

// Constants
constexpr float pi = math::pi;
constexpr float two_pi = math::two_pi;
constexpr float half_pi = math::half_pi;
```

### Comparison

```cpp
// Epsilon comparison
bool equal = math::equal(a, b, epsilon);
bool nearly_zero = math::is_zero(v, epsilon);

// Component-wise comparison
bool all_greater = math::all(math::greater_than(a, b));
bool any_greater = math::any(math::greater_than(a, b));
```

### Bounds & Geometry

```cpp
// AABB
struct Aabb {
    math::vec3 min;
    math::vec3 max;
};

// Sphere
struct Sphere {
    math::vec3 center;
    float radius;
};

// Ray
struct Ray {
    math::vec3 origin;
    math::vec3 direction;
};

// Plane
struct Plane {
    math::vec3 normal;
    float distance;
};
```

## Solver Utilities

The solver helpers cover small linear systems and low-degree polynomials that
appear in physics constraints, animation blending, and geometry intersection
tests.

```cpp
#include "engine/math/solvers.hpp"

// Solve Ax = b for a 3x3 matrix. Returns std::optional<vec3>.
math::mat3 A{
    3.0, 2.0, -1.0,
    2.0, -2.0, 4.0,
    -1.0, 0.5, -1.0};
math::vec3 b{1.0F, -2.0F, 0.0F};

auto x = math::solvers::try_solve_linear_system(A, b);
if (x)
{
    // -> {1, -2, -2}
}

// Quadratic roots (coefficients ax^2 + bx + c = 0)
std::array<float, 2> roots{};
const std::size_t root_count = math::solvers::solve_quadratic(1.0F, -5.0F, 6.0F, roots);
// root_count == 2, roots == {2, 3}

// Cubic roots. Returns 1, 2, or 3 real solutions sorted ascending.
std::array<double, 3> cubic_roots{};
const std::size_t cubic_count = math::solvers::solve_cubic(1.0, -6.0, 11.0, -6.0, cubic_roots);
// cubic_roots == {1, 2, 3}
```

### Numerical Domains

- `try_solve_linear_system` uses partial pivoting with tolerances matching the
  matrix inversion helpers: pivots smaller than `1e-6` (float) or `1e-12`
  (double) trigger a `std::nullopt` result; pass an explicit tolerance for
  custom scaling regimes.
- The polynomial solvers internally promote to long double, clamp discriminants
  with the same `1e-6`/`1e-12` thresholds, and gracefully fall back to lower-
  degree problems when the leading coefficient vanishes.

## Orthonormal Basis

Create coordinate frames:

```cpp
// From single vector (generate perpendicular vectors)
math::vec3 normal{0.0f, 1.0f, 0.0f};
auto [tangent, bitangent] = math::build_orthonormal_basis(normal);

// From forward direction
math::vec3 forward{0.0f, 0.0f, 1.0f};
math::vec3 up{0.0f, 1.0f, 0.0f};
math::vec3 right = math::normalize(math::cross(up, forward));
```

## Format Conversions

The module provides conversion utilities for external formats. See [`FORMAT_CONVERSIONS.md`](FORMAT_CONVERSIONS.md) for:
- OpenGL ↔ Vulkan ↔ DirectX coordinate system conversions
- Row-major ↔ column-major matrix layouts
- Left-handed ↔ right-handed coordinate systems
- Clip space transformations

## Performance Characteristics

- **Header-only**: No runtime overhead, enables inlining
- **SIMD-friendly**: Aligned types compatible with vectorization
- **Cache-coherent**: Compact memory layout for batch operations
- **Deterministic**: Consistent results across platforms (IEEE 754)

Typical operation costs:
- Vector operations: ~1-5 cycles
- Matrix multiplication: ~50-100 cycles
- Quaternion slerp: ~30-50 cycles
- Matrix inverse: ~200-300 cycles

## Testing

Math tests validate:
- Vector and matrix operations (`test_math.cpp`)
- Quaternion conversions and interpolation (`test_quaternion.cpp`)
- Transform composition (`test_transform.cpp`)
- Numerical precision and stability (`test_precision.cpp`)
- SIMD optimizations where available (`test_math_simd.cpp`)

Run tests:
```bash
ctest --preset linux-gcc-debug -R math
```

## Dependencies

- **None**: The math module is self-contained and header-only

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones and planned improvements
- [`FORMAT_CONVERSIONS.md`](FORMAT_CONVERSIONS.md): Coordinate system conversion reference
- [`SOLVER_STABILITY.md`](SOLVER_STABILITY.md): Numerical stability considerations for physics/animation
- Used by: Animation, Physics, Geometry, Rendering, Scene modules

## Current State

- Vector/matrix/quaternion primitives, transform utilities, and projection helpers used across modules.
- Header-only design with SIMD-friendly operations; format conversion cheatsheet published.

## Usage

- Include math headers directly from `engine/math/`.
- Run math tests:
  - `ctest --preset linux-gcc-debug -R math`

## TODO / Next Steps

- Track external-format conversion metrics and drift monitoring (`MA-130`); see ../../ROADMAP.md
- Extend numeric stability guidance in solver docs and wire unit drift telemetry; see ../../ROADMAP.md
