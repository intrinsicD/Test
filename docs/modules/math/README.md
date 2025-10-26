# Math Module

## Overview

The math module provides header-only vector, matrix, quaternion, and transform types optimized for graphics and physics applications. It serves as the foundation for all geometric computations across animation, physics, geometry, and rendering modules.

## Module Boundaries

- **Math** supplies numeric primitives (vectors, matrices, quaternions, transforms), projection helpers, and solver utilities. Everything is header-only under `engine/math/` so the module can be embedded in host applications and CUDA kernels.
- **Geometry** owns spatial primitives (`Aabb`, `Sphere`, `Ray`, `Plane`, frustums, etc.) and their intersection/containment logic. Import them from the geometry module and see [`docs/modules/geometry/README.md`](../geometry/README.md) for usage examples.
- **Animation/Physics/Rendering** consume the math types; keep documentation for those modules focused on behavioural details and reference this file for shared math helpers.

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
#include "engine/math/transform.hpp"

// Transform with translation, rotation, and scale
math::Transform<float> transform{};
transform.translation = {1.0f, 2.0f, 3.0f};
transform.rotation = math::quat::from_euler(0.0f, math::radians(45.0f), 0.0f);
transform.scale = {1.0f, 1.0f, 1.0f};

// Convert to a 4x4 matrix for pipeline uploads
const math::mat4 matrix = math::to_matrix(transform);

// Apply to points or directions
const math::vec3 point{0.0f, 0.0f, 1.0f};
const math::vec3 transformed_point = math::transform_point(transform, point);

const math::vec3 direction{0.0f, 1.0f, 0.0f};
const math::vec3 transformed_dir = math::transform_vector(transform, direction);

// Combine child relative to parent and compute inverse
const math::Transform<float> parent = math::Transform<float>::Identity();
const math::Transform<float> child = transform;
const math::Transform<float> world_transform = math::combine(parent, child);
const math::Transform<float> inverse_transform = math::inverse(transform);
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
#include "engine/math/utils/utils_rotation.hpp"
#include "engine/math/utils/utils_camera.hpp"

// Rotation matrices (single-axis convenience functions)
math::mat3 rotate_x3_mat = math::utils::rotate_x3(math::radians(45.0f));
math::mat3 rotate_y3_mat = math::utils::rotate_y3(math::radians(90.0f));
math::mat3 rotate_z3_mat = math::utils::rotate_z3(math::radians(30.0f));
math::mat4 rotate_x_mat = math::utils::rotate_x(math::radians(45.0f));
math::mat4 rotate_y_mat = math::utils::rotate_y(math::radians(90.0f));
math::mat4 rotate_z_mat = math::utils::rotate_z(math::radians(30.0f));

// General rotation from angle and axis
const float angle = math::radians(45.0f);
const math::vec3 axis{0.0f, 1.0f, 0.0f};
math::mat4 rotation = math::utils::to_rotation_matrix(angle, axis);

// Rotation from quaternion
const math::quat q = math::quat::from_euler(
    math::radians(15.0f),
    math::radians(30.0f),
    math::radians(0.0f)
);
math::mat4 rotation_from_quat = math::utils::to_rotation_matrix(q);

// Look-at matrix (view matrix)
const math::vec3 eye{0.0f, 0.0f, -5.0f};
const math::vec3 target{0.0f, 0.0f, 0.0f};
const math::vec3 up{0.0f, 1.0f, 0.0f};
math::mat4 view = math::utils::look_at(eye, target, up);

// Perspective projection
const float aspect_ratio = 16.0f / 9.0f;
const float near_plane = 0.1f;
const float far_plane = 1000.0f;
math::mat4 proj = math::utils::perspective(
    math::radians(60.0f),  // FOV
    aspect_ratio,
    near_plane,
    far_plane
);

// Orthographic projection
const float left = -10.0f;
const float right = 10.0f;
const float bottom = -10.0f;
const float top = 10.0f;
math::mat4 ortho = math::utils::orthographic(left, right, bottom, top, near_plane, far_plane);
```

### Operations

```cpp
// Transpose
math::mat4 transposed = math::transpose(m);

// Inverse
math::mat4 inverted = math::inverse(m);

// Determinant
float det = math::determinant(m);

// Transform point (homogeneous w = 1)
const math::vec3 point{1.0f, 2.0f, 3.0f};
const math::vec4 point4 = m * math::vec4{point[0], point[1], point[2], 1.0f};
const math::vec3 transformed{
    point4[0] / point4[3],
    point4[1] / point4[3],
    point4[2] / point4[3]
};

// Transform direction (homogeneous w = 0)
const math::vec3 direction{0.0f, 1.0f, 0.0f};
const math::vec4 dir4 = m * math::vec4{direction[0], direction[1], direction[2], 0.0f};
const math::vec3 transformed_dir{dir4[0], dir4[1], dir4[2]};
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
#include <cmath>

// Spherical linear interpolation (exact great-circle interpolation)
math::quat slerp_result = math::slerp(q1, q2, t);

// Squad for smooth orientation curves across keyframes
math::quat squad_result = math::squad(q_prev, q1, q2, q_next, t);

// Normalised linear interpolation helper (manual)
const math::quat linear{
    std::lerp(q1.w, q2.w, t),
    std::lerp(q1.x, q2.x, t),
    std::lerp(q1.y, q2.y, t),
    std::lerp(q1.z, q2.z, t)
};
const math::quat nlerp_result = math::normalize(linear);
```

## Rotation Utilities

The math module provides several ways to create rotation transformations, each suited for different use cases.

### Single-Axis Rotations

For simple rotations around cardinal axes, use the convenience functions in `utils_rotation.hpp`:

```cpp
#include "engine/math/utils/utils_rotation.hpp"

// Rotate 90 degrees around X-axis (right-handed coordinates)
float angle = math::radians(90.0f);
math::mat4 rx = math::utils::rotate_x(angle);

// Rotate around Y-axis (pitch/yaw)
math::mat4 ry = math::utils::rotate_y(angle);

// Rotate around Z-axis (roll)
math::mat4 rz = math::utils::rotate_z(angle);

// Combine rotations (applied right-to-left)
math::mat4 combined = rz * ry * rx;
```

**When to use**: Simple axis-aligned rotations, camera controls, or building complex rotations step-by-step.

**Advantages**: Direct sin/cos computation, no quaternion overhead, clear intent.

### General Axis-Angle Rotations

For rotations around arbitrary axes:

```cpp
// Rotation around normalized arbitrary axis
math::vec3 axis = math::normalize(math::vec3{1.0f, 1.0f, 0.0f});
float angle = math::radians(45.0f);
math::mat4 rotation = math::utils::to_rotation_matrix(angle, axis);
```

**When to use**: Physics simulations, joint rotations, or any rotation around a known axis.

### Quaternion-Based Rotations

For smooth interpolation and avoiding gimbal lock:

```cpp
// Create from euler angles
math::quat q = math::quat::from_euler(pitch, yaw, roll);
math::mat4 rotation = math::utils::to_rotation_matrix(q);

// Smooth interpolation between orientations
math::quat interpolated = math::slerp(start_quat, end_quat, 0.5f);
```

**When to use**: Animation, smooth camera transitions, or when combining many rotations.

**Advantages**: Avoids gimbal lock, efficient composition, smooth interpolation (slerp).

## Utility Functions

### Angles

```cpp
#include <numbers>

// Degree/radian conversion helpers
float radians = math::radians(180.0f);  // π
float degrees = math::utils::degrees(std::numbers::pi_v<float>);  // 180.0f

// Constants via <numbers>
constexpr float pi = std::numbers::pi_v<float>;
constexpr float two_pi = std::numbers::pi_v<float> * 2.0f;
constexpr float half_pi = std::numbers::pi_v<float> * 0.5f;
```

`math::utils::degrees` and other camera helpers live in `engine/math/utils/utils_camera.hpp`.

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

Shape primitives (AABBs, spheres, rays, planes, frustums, capsules, …) and all
intersection/containment helpers live in the geometry module. Import them from
`engine/geometry/shapes.hpp` and consult
[`docs/modules/geometry/README.md`](../geometry/README.md) for dedicated usage
examples and parity tests.

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

## Sparse Matrices

`SparseMatrix<T>` (column-major CSC) lives in `engine/math/sparse_matrix.hpp`.
Use it when you need dynamic, editable sparse storage without depending on
external libraries.

```cpp
#include "engine/math/sparse_matrix.hpp"

using Sparse = math::SparseMatrix<float>;

// Assemble from triplets (duplicates summed by default)
std::vector<Sparse::Triplet> triplets{
    {0, 0, 2.0f},
    {1, 0, -1.0f},
    {1, 2, 4.0f}
};
Sparse matrix = Sparse::from_triplets(3, 3, std::move(triplets));
matrix.add_to(0, 1, 1.5f);   // Increment existing entry
matrix.set(2, 2, 5.0f);      // Insert or overwrite

// Traverse non-zero entries in deterministic order
matrix.for_each_nz([](Sparse::size_type row, Sparse::size_type col, float value)
{
    // Export to CSR, assemble normal equations, etc.
});

// Query optional entry (binary search per column)
const std::optional<float> maybe = matrix.try_get(1, 2);
```

Call `reserve` before streaming inserts to avoid reallocations and reuse
existing storage via `clear()` when batching solves.

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

## Telemetry

- Include `engine/math/telemetry/conversion_telemetry.hpp` to access the
  recording helpers and snapshot APIs.
- Conversion drift metrics record round-trip error statistics whenever
  `engine::math::telemetry::RecordVectorRoundTrip` or
  `RecordMatrixRoundTrip` is called. Metrics surface through runtime
  diagnostics as `runtime.math.conversions.*` gauges and counters grouped
  by vector dimension or matrix size.
- Use `ConversionTelemetry::snapshot()` in tests or tooling to capture the
  aggregated statistics; see
  `engine/math/tests/test_conversion_telemetry.cpp` for examples.
- Telemetry entries include sample counts plus maximum and mean absolute
  and relative error, enabling dashboards to monitor drift when exporting
  to external formats.

## TODO / Next Steps

- Monitor conversion telemetry adoption across geometry and IO pipelines;
  add dashboards once runtime consumers integrate the new metrics (`MA-130`
  follow-up).
- Extend numeric stability guidance in solver docs and wire unit drift telemetry; see ../../ROADMAP.md
