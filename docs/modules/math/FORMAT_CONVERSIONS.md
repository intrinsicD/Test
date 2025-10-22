# Math Module External Format Conversions

_Updated: 2025-05-20_

## Overview

Many asset formats and runtime integrations store math primitives in tightly
packed arrays whose layout differs from the engine's native types. The
conversion helpers introduced in `MA-125` provide deterministic round-trips
between `engine::math` vectors/matrices and the layouts expected by common
pipelines. All helpers live in
`engine/math/conversions.hpp` under the `engine::math::conversions` namespace
and operate purely on value copies—no global state or allocator hooks are used.

### Quick Reference

| External format | Layout | Helper(s) | Notes |
| --- | --- | --- | --- |
| **glTF 2.0** (JSON + binary buffers) | Column-major 4×4 affine matrices, translation in last column. | `to_column_major_array`, `matrix_from_column_major_span` | Cast to `float` before export; preserves row vectors used by animation + geometry IO. |
| **USD / OpenEXR** attribute payloads | Row-major matrices serialized into contiguous arrays. | `to_row_major_array`, `matrix_from_row_major_span` | Supports `float` and `double`; use spans when consuming Hydra-style buffers. |
| **DirectX constant buffers** | Row-major matrices with HLSL `row_major` semantics. | `to_row_major_array`, `matrix_from_row_major_array` | Emit via `std::array<float, 16>` to avoid padding before upload. |
| **Quaternion/vector blobs** in legacy authoring tools | Packed XYZ(W) arrays. | `to_array`, `vector_from_span` | Accepts `std::span<const T>` for interoperability with binary readers. |

## Usage Examples

```cpp
#include <engine/math/conversions.hpp>

using namespace engine::math;

void ExportNodeTransform(const Transform<float>& transform,
                         std::array<float, 16>& out_gltf_matrix)
{
    const Matrix<float, 4, 4> local = to_matrix(transform);
    out_gltf_matrix = conversions::to_column_major_array(local);
}

Matrix<double, 4, 4> LoadUsdMatrix(std::span<const double> payload)
{
    return conversions::matrix_from_row_major_span<double, 4, 4>(payload);
}
```

Vectors, quaternions, and colors follow the same pattern: use
`conversions::to_array` when serialising to flat buffers and
`conversions::vector_from_span` when reconstructing the engine types from
external memory.

## Precision & Validation Guidance

- **Type promotion.** Helpers perform `static_cast` assignments so callers can
  mix `float` payloads with `double` math types when higher-precision processing
  is required. Normalise values before down-casting when exporting to `float`.
- **Bounds checking.** Span-based loaders assert on size mismatches to surface
  malformed input during testing. Release builds keep the copies deterministic
  and rely on upstream loaders to validate buffer lengths.
- **Determinism.** Array order is fixed; the first element written by
  `to_column_major_array` is `[0][0]`, mirroring GPU API expectations. Pair the
  helpers with `math::try_inverse`/`math::pseudo_inverse` guidance documented in
  [`SOLVER_STABILITY.md`](SOLVER_STABILITY.md) when ingesting untrusted data.

## Regression Coverage

`engine/math/tests/test_math.cpp` exercises round-trips for vectors and both
row-major/column-major matrix layouts to prevent accidental regressions in
memory ordering. Extend these tests when introducing new helper variants.

## Related Materials

- [`docs/modules/math/SOLVER_STABILITY.md`](SOLVER_STABILITY.md) — numerical
  tolerances and preconditioning tips when operating on external data.
- [`docs/modules/runtime/ASYNC_STREAMING_INTEGRATION.md`](../runtime/ASYNC_STREAMING_INTEGRATION.md)
  — how conversion helpers feed into runtime asset streaming once data is
  decoded from disk.
