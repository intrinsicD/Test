# Math Module

The math module provides the foundational numeric types and routines that power higher-level systems. Its
public headers live under [`engine/math/include`](../../engine/math/include) and expose the following
major building blocks.

## Core types
- [`engine/math/common.hpp`](../../engine/math/include/engine/math/common.hpp) defines compile-time
  traits, literal helpers, and inline macros shared across all math primitives.
- [`engine/math/vector.hpp`](../../engine/math/include/engine/math/vector.hpp) declares the templated
  `Vector<T, N>` container along with arithmetic operators, index accessors, and dimension-changing
  constructors. Vectors form the bridge between scalar math and geometric constructs.
- [`engine/math/matrix.hpp`](../../engine/math/include/engine/math/matrix.hpp) offers column-major
  matrices with row and column proxy types, arithmetic composition, and conversion helpers used by the
  rendering and physics stacks.
- [`engine/math/quaternion.hpp`](../../engine/math/include/engine/math/quaternion.hpp) implements
  Hamiltonian quaternions for orientation representation, including scalar/vector constructors, identity
  helpers, and product operators.

## Utility algorithms
- [`engine/math/utils.hpp`](../../engine/math/include/engine/math/utils/utils.hpp) collects inline utilities
  for clamping, extrema queries, elementary functions, and tolerance-based comparisons. These helpers
  preserve type generality while interoperating with the core vector and matrix structures.
- [`engine/math/utils_rotation.hpp`](../../engine/math/include/engine/math/utils/utils_rotation.hpp) (not
  shown) supplements quaternion and matrix math with routines for constructing rotation primitives from
  canonical parameterizations.

## Usage notes
- All math types are header-only templates; include the relevant header where needed and rely on C++
  inline semantics for performance.
- Constructors favor explicit semantics to avoid accidental narrowing; implicit conversions exist only
  when guaranteed to preserve dimension and orientation semantics.
- The module intentionally avoids heap allocation. Fixed-size data structures make it suitable for real-
  time systems.
