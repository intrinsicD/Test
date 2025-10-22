## PRIORITY_DECISION
Selected Task: MA-110 — SIMD validation harness
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| MA-110 SIMD validation harness | 5 | 4 | 3 | 3 | 3 | 5 | 23 |
| MA-118 solver stability documentation | 3 | 3 | 2 | 2 | 3 | 4 | 17 |
| MA-125 external-format conversion cheatsheet | 2 | 2 | 1 | 1 | 3 | 3 | 12 |
Decision Rationale:
- Overdue roadmap commitment threatening math module stability metrics.
- Unlocks MA-118/MA-125 documentation by providing empirical coverage for SIMD paths.
- Keeps TI-003 validation workstream on schedule with deterministic regression data.
- Moderate implementation scope limited to tests and documentation updates.
- Direct alignment with architecture improvement plan expectations for math utilities.

## DESIGN_BRIEF
Problem Statement: Math module lacks deterministic coverage for prospective SIMD implementations, leaving TI-003 validation unsupported and risking silent regressions in vector operations. Acceptance Criteria: (1) Provide reusable harness verifying vector arithmetic, dot/cross, and normalisation across typical SIMD widths (4, 8); (2) Register harness with CTest for CI consumption; (3) Update module/roadmap documentation to mark MA-110 complete and advertise the new suite; (4) Ensure command recipes cover new targets. Interfaces & Data Flow: Typed GoogleTest suite batches deterministic vec3 datasets, converts them into a structure-of-arrays pack, and compares batched results against scalar engine::math reference implementations. CMake registers a dedicated `engine_math_simd_tests` target. Docs propagate completion status. Invariants: Preserve existing vector APIs; tests operate purely on deterministic data without mutating module state. Compatibility: Purely additive test harness; no runtime API changes. Security/Performance: Test-only additions; deterministic RNG seeds guarantee reproducibility. Test Strategy: Typed tests executed for widths 4 and 8 validate arithmetic, dot/cross, and normalisation outputs against scalar baselines.

## PATCH
```diff
+add_executable(engine_math_simd_tests
+    test_math_simd.cpp
+)
+
+target_link_libraries(engine_math_simd_tests
+    PRIVATE
+        engine::project_options
+        engine_math
+        GTest::gtest_main
+)
+
+add_test(NAME engine_math_simd_tests COMMAND engine_math_simd_tests)
```
```cpp
+    template <typename T, std::size_t N, std::size_t Width>
+    SimdVector<T, N, Width> SimdAdd(const SimdVector<T, N, Width>& lhs, const SimdVector<T, N, Width>& rhs) noexcept
+    {
+        SimdVector<T, N, Width> result{};
+        for (std::size_t lane = 0; lane < Width; ++lane)
+        {
+            for (std::size_t component = 0; component < N; ++component)
+            {
+                result[component][lane] = lhs[component][lane] + rhs[component][lane];
+            }
+        }
+        return result;
+    }
```

## TESTS
```cpp
+TYPED_TEST(SimdValidationTest, DotAndCrossMatchScalar)
+{
+    constexpr std::size_t width = TestFixture::kWidth;
+    const auto lhs = TestFixture::MakeVectors(84U);
+    const auto rhs = TestFixture::MakeVectors(2112U);
+
+    TestFixture::ForEachChunk(lhs, rhs,
+        [](const auto& lhs_chunk, const auto& rhs_chunk)
+        {
+            const auto lhs_pack = LoadVectors<float, 3, width>(lhs_chunk);
+            const auto rhs_pack = LoadVectors<float, 3, width>(rhs_chunk);
+
+            const auto dot_values = SimdDot(lhs_pack, rhs_pack);
+            const auto cross_pack = SimdCross(lhs_pack, rhs_pack);
+            const auto cross_vectors = StoreVectors(cross_pack);
+
+            for (std::size_t lane = 0; lane < width; ++lane)
+            {
+                const auto expected_dot = engine::math::dot(lhs_chunk[lane], rhs_chunk[lane]);
+                const auto expected_cross = engine::math::cross(lhs_chunk[lane], rhs_chunk[lane]);
+
+                EXPECT_NEAR(dot_values[lane], expected_dot, 1e-5F);
+                ExpectVec3Near(cross_vectors[lane], expected_cross, 1e-5F);
+            }
+        });
+}
```

## DOCS
```diff
-| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, and transform utilities feeding animation, geometry, and physics. | `MA-110`: add SIMD validation targets aligned with `TI-003`. |
+| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, and transform utilities feeding animation, geometry, and physics. | `MA-118`: document solver stability ranges. `MA-125`: provide external-format conversion cheatsheet. |
```
```diff
-| `MA-110` | Introduce SIMD validation harness (`TI-003`). | Add SIMD regression suite and hook into CI. | 🔄 In Progress |
+| `MA-110` | Introduce SIMD validation harness (`TI-003`). | Add SIMD regression suite and hook into CI. | ✅ Done |
```

## VERIFY
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target engine_math_tests engine_math_simd_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_math`

## REVIEW_FINDINGS
- Review completed (see `ma-110-simd-validation-harness-review.md`) – no
  critical or warning issues identified.
  - Suggestions: extend coverage to quaternion math alongside MA-125; monitor
    SIMD suite runtime once CI integration lands.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
See diff excerpts above; no additional adjustments after review.

## FOLLOW_UP_TODOS
- [ ] Extend SIMD harness to cover quaternion operations once vector coverage stabilises (owner: Math team, medium priority, aligns with MA-125 planning).
- [ ] Wire SIMD test target into CI performance dashboards to monitor execution time drift.
- [ ] Investigate opportunities to share structure-of-arrays helpers with future SIMD runtime paths (tech debt backlog).
- [x] Capture harness usage notes in docs/specs when MA-118 documentation expands solver guidance (covered by `docs/modules/math/solver_stability.md`).
