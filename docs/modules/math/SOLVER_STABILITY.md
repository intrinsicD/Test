# Solver Stability Guidance

This note documents the numerical envelopes for the math module's solvers so
consumers (physics, animation, geometry) can size inputs appropriately and
select the right precision tier. All recommendations below assume IEEE-754
behaviour. Values refer to the magnitude of the determinant, singular values, or
vector norms before any internal normalisation.

## Summary Table

| Solver | Stable domain (float) | Stable domain (double) | Notes |
| --- | --- | --- | --- |
| `try_inverse(Matrix<T,2,2/3,3>)` | `|det| ≥ 1e-6` | `|det| ≥ 1e-12` | Direct adjugate / determinant paths reject zero determinants without tolerance; treat near-singular matrices as unstable. |
| `try_inverse(Matrix<T,4,4>)` | Pivot magnitudes `≥ 1e-6` | Pivot magnitudes `≥ 1e-12` | Gaussian elimination with partial pivoting aborts if a pivot is numerically zero; rescale transforms before inversion. |
| `utils::pseudo_inverse` | Largest singular value `σ_max`, discard `σ < 1e-10 · σ_max` | `σ < 1e-12 · σ_max` | Default tolerance clips small singular values; pass custom tolerance for tightly-conditioned problems. |
| `Transform::from_matrix` | Column lengths `≥ 1e-6`, `w ≈ 1 ± 1e-6` | Column lengths `≥ 1e-12`, `w ≈ 1 ± 1e-12` | Degenerate scales fall back to identity rotation; ensure affine inputs. |
| `normalize`, projection helpers | Vector norm `≥ 1e-6` | Vector norm `≥ 1e-12` | Zero-length vectors return the original input; guard against division by small magnitudes. |

Use double precision whenever condition numbers exceed `1e6` for floats or when
physics constraints accumulate error across frames. Promote intermediate work to
double even if results are stored in float, matching the transform inversion
path.

## Matrix Inversion Families

The 2×2 and 3×3 helpers perform closed-form adjugate division and return
`std::nullopt` when the determinant is exactly zero, with no epsilon-based guard
for near singularities.【F:engine/math/include/engine/math/matrix.hpp†L318-L366】
For floats this means determinants below roughly `1e-6` (matching the default
`nearly_equal` epsilon) can amplify noise; callers should rescale the matrix or
upgrade to doubles before inverting.

The 4×4 path performs Gaussian elimination with partial pivoting and aborts if a
pivot magnitude underflows to zero.【F:engine/math/include/engine/math/matrix.hpp†L369-L418】
Because pivots are compared against exact zero, ensure each pivot stays above
`1e-6` (float) or `1e-12` (double) after any application-specific scaling.
Physics uses this routine when inverting transforms, so pre-normalise column
vectors and avoid baking extremely skewed scales.

Regression coverage exercises representative inputs and rejects singular cases,
showing where the helpers succeed today.【F:engine/math/tests/test_math.cpp†L665-L768】
Add new fixtures when introducing matrices with higher condition numbers.

## Pseudoinverse and SVD Tolerance

`utils::pseudo_inverse` cascades through direct normal-equation inversions
before falling back to the SVD-based implementation, so the same determinant
thresholds apply to the tall/wide paths.【F:engine/math/include/engine/math/utils/utils_matrix.hpp†L45-L87】
The SVD path clips singular values below `tolerance · max(σ)` where the default
`tolerance` is `1e-10`, scaled by the larger matrix dimension.【F:engine/math/include/engine/math/utils/utils_matrix.hpp†L10-L42】
Override the tolerance when working with float data featuring tight aspect
ratios; retain the default for double-precision workflows.

The Jacobi SVD uses a fixed `1e-10` orthogonality tolerance and `1e-15` epsilon
when detecting zero singular values.【F:engine/math/include/engine/math/utils/svd_jacobi.hpp†L17-L124】
If convergence stalls for stiff problems, increase `max_iterations` or switch to
an analytic solver with preconditioning.

Existing tests cover full-rank, overdetermined, underdetermined, and rank-
deficient cases, providing baselines for the documented tolerances.【F:engine/math/tests/test_math.cpp†L770-L869】

## Transform Decomposition and Inversion

`Transform::from_matrix` relies on `utils::nearly_equal` (epsilon `1e-6`) to
validate the affine `w` component, clear perspective terms, and detect degenerate
scale columns.【F:engine/math/include/engine/math/transform.hpp†L56-L147】【F:engine/math/include/engine/math/utils/utils.hpp†L7-L51】
When any scale axis collapses below that epsilon the routine keeps the identity
rotation, so upstream systems should enforce minimum scale magnitudes before
conversion.

`Transform::inverse` always promotes to double precision before calling
`try_inverse`, guaranteeing the more robust 4×4 path even for float transforms.【F:engine/math/include/engine/math/transform.hpp†L180-L185】
Maintain this behaviour when extending the API so precision guidance remains
valid.

Round-trip tests confirm the documented tolerances for typical animation and
scene workloads and should be extended if new edge cases are supported.【F:engine/math/tests/test_math.cpp†L904-L948】

## Vector Normalisation and Projection

The vector helpers compute lengths via `sqrt` and bail out when the norm is zero
before normalising or projecting.【F:engine/math/include/engine/math/vector.hpp†L202-L274】
Guard against inputs whose magnitude drifts below `1e-6` in float space (or
`1e-12` in double space) to avoid returning the original vector unchanged.
When in doubt, pre-normalise using double precision and cast back to float.

## Operational Recommendations

- **Precision selection.** Run constraint solvers in double precision when
  condition numbers exceed `1e6` or when the solver accumulates corrections over
  many frames; store results in float only after normalisation.
- **Preconditioning.** Rescale rows/columns so the largest entry is close to one
  before invoking `try_inverse` or `pseudo_inverse` to keep pivot magnitudes
  inside the stable ranges.
- **Instrumentation.** Log determinant magnitudes and singular value spectra in
  diagnostics to catch scenes drifting outside the documented domains. Update the
  telemetry viewers if new metrics are added.
- **Testing.** Extend `engine_math_tests` with adversarial matrices whenever new
  solver modes are introduced to keep this guidance accurate.

Pair these recommendations with the published
[`FORMAT_CONVERSIONS.md`](FORMAT_CONVERSIONS.md) cheatsheet when shuttling data
between external formats and engine types; revisit both documents if new
formats introduce tighter precision constraints.
