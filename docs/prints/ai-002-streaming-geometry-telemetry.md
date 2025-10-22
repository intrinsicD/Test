## PRIORITY_DECISION
- Selected Task: AI-002 streaming geometry failure telemetry
- Score Table:
  | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
  | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
  | AI-002 streaming geometry failure telemetry | 3 | 4 | 4 | 4 | 2 | 5 | 22 |
  | RT-005 hierarchy alert thresholds | 2 | 3 | 2 | 3 | 3 | 4 | 17 |
  | TL-110 integration smoke test | 2 | 3 | 2 | 2 | 2 | 3 | 14 |
- Tie-break Rationale: AI-002 closes an in-progress observability gap that blocks diagnosing async streaming regressions. Remaining candidates deliver incremental documentation but do not unblock roadmap execution.
- Decision Rationale
  - Async streaming failures currently collapse into generic counters, preventing operators from attributing outages to IO defects.
  - Attaching `GeometryIoErrorCode` identifiers unblocks telemetry consumers and satisfies the IO-230 follow-up.
  - The work reuses existing IO telemetry primitives and surfaces value across runtime tooling with moderate effort.
  - Downstream scripts/viewers rely on structured failure metadata to guide remediation; leaving the gap risks silent regressions.
  - Aligns directly with AI-002 acceptance criteria and feeds diagnostics runbooks referenced in docs/modules/runtime/diagnostics.md.

## DESIGN_BRIEF
- Problem Statement: Runtime streaming telemetry only exposed aggregate failure counts. Operators cannot correlate async load failures with the underlying `GeometryIoErrorCode`, stalling AI-002 observability goals and IO-230 follow-up guidance.
- Acceptance Criteria:
  1. `AssetLoadError` preserves optional `GeometryIoErrorCode` metadata for geometry-driven failures.
  2. `AssetStreamingTelemetry` tracks per-error counters and snapshots surface them to runtime diagnostics.
  3. Runtime telemetry exports `runtime.streaming.failures{error=...}` metrics plus structured C/Python bindings.
  4. Diagnostics tooling (`runtime_frame_telemetry.py`, `telemetry_viewer.py`, `streaming_report.py`) renders per-error breakdowns.
  5. Asset async tests and runtime tests cover the new telemetry surfaces; docs updated to describe usage.
- Interfaces & Data Flow: geometry caches map IO errors to `AssetLoadError` with attached geometry codes → async queue notifies `AssetStreamingTelemetry` on failure → runtime snapshots copy counts into `StreamingMetrics` → telemetry metrics gain labeled counters → C ABI + Python bindings extend structs → CLI scripts serialise/visualise failure histograms.
- Invariants: preserve existing async queue semantics, avoid duplicate total counters, keep telemetry snapshots deterministic for tests, maintain compatibility with existing struct layouts via additive fields.
- Compatibility/Migrations: C ABI adds array fields; Python ctypes structs updated accordingly. Runtime/asset binaries must be rebuilt together. Existing consumers without updates will ignore new data safely.
- Security/Performance/Edge Cases: Additional atomics per geometry error are negligible; detection preflight is bounded and reuses IO utilities. No secrets introduced. Ensure failure recording remains lock-free.
- Test Plan: extend C++ unit tests (`engine_assets_tests`, `engine_runtime_tests`) and Python unit tests for telemetry scripts; run targeted CTest suites and pytest. Validate docs with `scripts/validate_docs.py`.

## PATCH
```diff
+++ engine/assets/include/engine/assets/async.hpp
@@
+#include "engine/io/errors.hpp"
+#include "engine/io/telemetry.hpp"
+#include <array>
@@
-    class AssetLoadError final : public engine::EnumeratedErrorCode<AssetLoadErrorCategory>
+    class AssetLoadError final : public engine::EnumeratedErrorCode<AssetLoadErrorCategory>
     {
     public:
         using EnumeratedErrorCode::EnumeratedErrorCode;
@@
-        [[nodiscard]] AssetLoadError with_message(std::string message) const
+        [[nodiscard]] AssetLoadError with_message(std::string message) const
         {
             AssetLoadError copy{*this};
             copy.assign_message(std::move(message));
             return copy;
         }
+
+        [[nodiscard]] AssetLoadError with_geometry_error(io::GeometryIoErrorCode error) const;
+        [[nodiscard]] const std::optional<io::GeometryIoErrorCode>& geometry_error() const noexcept;
@@
-    struct AssetStreamingSnapshot
+    struct AssetStreamingSnapshot
     {
         std::uint64_t pending{0};
         std::uint64_t loading{0};
         std::uint64_t total_requests{0};
         std::uint64_t total_completed{0};
         std::uint64_t total_failed{0};
         std::uint64_t total_cancelled{0};
         std::uint64_t total_rejected{0};
+        std::array<std::uint64_t, io::geometry_io_error_count()> geometry_failures{};
     };
@@
-        void on_transition(AssetLoadState from, AssetLoadState to);
+        void on_transition(AssetLoadState from, AssetLoadState to);
+        void on_failure(const AssetLoadError& error);
@@
-        std::atomic<std::uint64_t> total_rejected_{0};
+        std::atomic<std::uint64_t> total_rejected_{0};
+        std::array<std::atomic<std::uint64_t>, io::geometry_io_error_count()> geometry_failures_{};
```
```diff
+++ engine/runtime/include/engine/runtime/api.hpp
@@
-    std::uint64_t streaming_total_rejected{0};
+    std::uint64_t streaming_total_rejected{0};
+    std::array<std::uint64_t, io::geometry_io_error_count()> geometry_failures_by_error{};
@@
-struct engine_runtime_streaming_metrics
+struct engine_runtime_streaming_metrics
 {
@@
-    std::uint64_t streaming_total_rejected;
+    std::uint64_t streaming_total_rejected;
+    std::uint64_t geometry_failures_by_error[engine::io::geometry_io_error_count()];
 };
```
```diff
+++ scripts/diagnostics/runtime_frame_telemetry.py
@@
+GEOMETRY_IO_ERROR_CODES: Tuple[str, ...] = (
+    "file_not_found",
+    "io_failure",
+    "invalid_argument",
+    "unsupported_format",
+    "plugin_missing",
+)
+GEOMETRY_IO_ERROR_COUNT = len(GEOMETRY_IO_ERROR_CODES)
@@
 class _CStreamingMetrics(ctypes.Structure):
     _fields_ = [
@@
         ("streaming_total_rejected", ctypes.c_uint64),
+        ("geometry_failure_counts", ctypes.c_uint64 * GEOMETRY_IO_ERROR_COUNT),
     ]
@@
 class RuntimeStreamingMetrics:
@@
     streaming_total_rejected: int
+    geometry_failures_by_error: Dict[str, int]
@@
             streaming_total_rejected=int(data.streaming_total_rejected),
+            geometry_failures_by_error={
+                name: int(data.geometry_failure_counts[index])
+                for index, name in enumerate(GEOMETRY_IO_ERROR_CODES)
+            },
         )
```
```diff
+++ scripts/diagnostics/telemetry_viewer.py
@@
+GEOMETRY_IO_ERROR_CODES: Sequence[str] = (
+    "file_not_found",
+    "io_failure",
+    "invalid_argument",
+    "unsupported_format",
+    "plugin_missing",
+)
@@
-    lines = [f"{key.replace('_', ' ').title()}: {int(streaming.get(key, 0))}" for key in keys]
+    lines = [f"{key.replace('_', ' ').title()}: {int(streaming.get(key, 0))}" for key in keys]
+    failure_lines: List[str] = []
+    for error in GEOMETRY_IO_ERROR_CODES:
+        key = f"geometry_failure:{error}"
+        value = streaming.get(key)
+        if isinstance(value, (int, float)) and int(value) > 0:
+            failure_lines.append(f"  • {error.replace('_', ' ')}: {int(value)}")
+    if failure_lines:
+        lines.append("Geometry IO failures:")
+        lines.extend(failure_lines)
```
(Additional diffs cover runtime metric emission, async cache detection prechecks, C API struct copies, Python CLI output, and documentation updates.)

## TESTS
- Strengthened C++ coverage in `engine/assets/tests/test_async.cpp` for geometry failure counts and cancellation/rejection zero checks.
- Updated runtime diagnostics tests in `engine/runtime/tests/test_module.cpp` to assert per-error counters and telemetry samples.
- Extended Python unit tests in `scripts/tests/test_runtime_frame_telemetry.py` and `scripts/tests/test_telemetry_viewer.py` for new schema fields and console output.

## DOCS
- Documented geometry failure counters in `docs/modules/runtime/README.md` and `docs/modules/runtime/diagnostics.md`.
- Marked IO-230 follow-up complete in `docs/prints/io-230-implementation.md`.

## VERIFY
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target engine_assets_tests engine_runtime_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_assets_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_runtime_tests`
- `pytest scripts/tests/test_runtime_frame_telemetry.py scripts/tests/test_telemetry_viewer.py`
- `python scripts/validate_docs.py`

## REVIEW_FINDINGS
- Correctness: Verified geometry error propagation for async loads and runtime metrics; noted decode-path errors remain aggregated (acceptable for follow-up).
- Security: No new external inputs; telemetry additions are internal.
- Compatibility: C ABI change is additive; Python bindings updated to match.
- Quality & Style: Naming and formatting follow existing conventions; comments unnecessary for straightforward data plumbing.
- Performance: Preflight detection only runs for async geometry loads and short-circuits on success.
- Tests: C++ and Python unit suites cover new behaviour; docs validation executed.
- Observability: Labeled counters integrate with telemetry schema and viewer output.
- Docs: Runtime telemetry documentation updated alongside implementation.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Unchanged from ## PATCH.

## PRIORITY_DECISION (2025-05-07)
- Selected Task: AI-002 follow-up – propagate geometry errors from decode path
- Score Table:
  | Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
  | ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
  | AI-002 follow-up – propagate geometry errors from decode path | 3 | 4 | 4 | 4 | 3 | 5 | 23 |
  | AI-002 follow-up – extend telemetry to non-geometry error categories | 2 | 3 | 2 | 2 | 3 | 3 | 15 |
- Decision Rationale
  - Decode-path failures were collapsing into generic IO/validation errors, hiding the `GeometryIoErrorCode` attribution promised by AI-002.
  - Operators rely on per-error counters to differentiate corrupt assets from missing files; leaving the gap risks false negatives.
  - Implementation reuses existing async cache plumbing with moderate effort and minimal blast radius.
  - Completing this follow-up unblocks the remaining telemetry parity work and keeps roadmap alignment intact.
  - The deferred task is observability-nice-to-have with lower urgency and unblock value.

## DESIGN_BRIEF
- Problem Statement: When geometry detection succeeds but subsequent decode or validation fails, async caches throw `std::runtime_error`, erasing the underlying `GeometryIoErrorCode`. Telemetry therefore records counts without per-error attribution for corrupt assets.
- Acceptance Criteria:
  1. Synchronous cache reloads surface an exception retaining the originating `AssetLoadError` (with optional geometry metadata).
  2. Async cache catch blocks propagate the preserved error so telemetry increments the appropriate geometry counter.
  3. Regression coverage exercises decode-path failures, asserting both future results and telemetry snapshots expose `GeometryIoErrorCode::invalid_argument`.
- Interfaces & Data Flow: `reload_asset` returns `AssetLoadError` → `load` throws `AssetLoadException` carrying the error → async lambda catches `AssetLoadException` and forwards the stored error to `AssetStreamingTelemetry` and the future → telemetry snapshot reflects per-error increments.
- Invariants: Maintain existing async state transitions, preserve ABI of `AssetLoadError`, and continue recording hot-reload telemetry exactly once per failure.
- Compatibility/Migrations: New exception type inherits `std::runtime_error`; existing synchronous callers catching `std::exception` continue to function. No ABI changes beyond inline header updates.
- Security/Performance/Edge Cases: No new IO surfaces; additional copy of `AssetLoadError` is small. Ensure corrupted asset fixtures are ephemeral to avoid leaking to repo.
- Test Plan: Extend `engine/assets/tests/test_async.cpp` with decode-failure scenarios covering futures, telemetry counters, and geometry error metadata.

## PATCH
```diff
+++ engine/assets/include/engine/assets/async.hpp
@@
-#include <string>
+#include <stdexcept>
+#include <string>
@@
     private:
         std::optional<io::GeometryIoErrorCode> geometry_error_{};
     };
+
+    class AssetLoadException final : public std::runtime_error
+    {
+    public:
+        explicit AssetLoadException(AssetLoadError error)
+            : std::runtime_error([&error]() {
+                  const auto message = error.message();
+                  if (!message.empty())
+                  {
+                      return std::string{message};
+                  }
+                  return std::string{to_string(error.code())};
+              }()),
+              error_{std::move(error)}
+        {
+        }
+
+        [[nodiscard]] const AssetLoadError& error() const noexcept
+        {
+            return error_;
+        }
+
+    private:
+        AssetLoadError error_;
+    };
```
```diff
+++ engine/assets/src/mesh_asset.cpp
@@
-        if (auto reload = reload_asset(handle, *asset, !inserted); !reload.has_value())
-        {
-            const auto message = reload.error().message();
-            throw std::runtime_error(message.empty() ? std::string{to_string(reload.error().code())}
-                                                     : std::string{message});
-        }
+        if (auto reload = reload_asset(handle, *asset, !inserted); !reload.has_value())
+        {
+            throw AssetLoadException(reload.error());
+        }
@@
             try
             {
                 const auto& asset = this->load(descriptor);
                 return AssetLoadResult<MeshHandle>{asset.descriptor.handle};
             }
+            catch (const AssetLoadException& ex)
+            {
+                auto error = ex.error();
+                AssetHotReloadTelemetry::instance().record_failure(error, descriptor.handle.id());
+                return AssetLoadResult<MeshHandle>{error};
+            }
             catch (const std::invalid_argument& ex)
             {
                 auto error = make_error(AssetLoadErrorCategory::ValidationError, ex.what());
                 AssetHotReloadTelemetry::instance().record_failure(error, descriptor.handle.id());
```
```diff
+++ engine/assets/tests/test_async.cpp
@@
     std::filesystem::path write_temporary_obj()
@@
         return path;
     }
+
+    std::filesystem::path write_corrupted_obj()
+    {
+        auto path = std::filesystem::temp_directory_path() / "engine_async_mesh_corrupted.obj";
+        std::ofstream stream{path};
+        stream << "o mesh\n";
+        stream << "v 0 0 0\n";
+        stream << "v 1 0 0\n";
+        stream << "f 1 3 2\n";
+        stream.close();
+        return path;
+    }
@@
 TEST_F(MeshCacheAsyncTest, LoadAsyncPropagatesGeometryErrorOnDecodeFailure)
@@
     ASSERT_FALSE(result.has_value());
     ASSERT_TRUE(result.error().geometry_error().has_value());
     EXPECT_EQ(result.error().geometry_error()->code(), engine::io::GeometryIoError::invalid_argument);
@@
 TEST(AssetStreamingTelemetry, RecordsDecodeFailureTransition)
@@
     ASSERT_FALSE(result.has_value());
     ASSERT_TRUE(result.error().geometry_error().has_value());
     EXPECT_EQ(result.error().geometry_error()->code(), engine::io::GeometryIoError::invalid_argument);
```

## TESTS
- Added decode-failure coverage in `engine/assets/tests/test_async.cpp` verifying futures propagate `GeometryIoErrorCode::invalid_argument` and telemetry snapshots record the corresponding counter.

## DOCS
- Logged the follow-up execution and completion state in `docs/prints/ai-002-streaming-geometry-telemetry.md`.

## VERIFY
- `cmake --build --preset linux-gcc-debug --target engine_assets_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_assets_tests`

## REVIEW_FINDINGS
- Correctness: Exception plumbing now preserves geometry metadata across async boundaries; tests cover both future results and telemetry snapshots.
- Security: No new IO surfaces; corrupted fixtures live under temp directories.
- Compatibility: `AssetLoadException` derives from `std::runtime_error`, so existing catch sites continue functioning.
- Quality & Style: Exception and helper naming align with assets module conventions; tests clean up temporary files.
- Performance: Added error-copying path runs only on failures; steady-state unaffected.
- Tests: New cases validate decode-failure reporting and telemetry integration.
- Observability: Telemetry counters now differentiate corrupt inputs without manual inspection.
- Docs: Follow-up log updated for traceability.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Unchanged from ## PATCH.

## FOLLOW_UP_TODOS
- [x] Propagate `GeometryIoErrorCode` through decode/validation failure paths where detection succeeds but reads fail (owner: Assets, priority: medium, rationale: complete per-error coverage). Resolved by capturing `AssetLoadException` instances in async caches to preserve geometry metadata and exercising decode-failure telemetry in unit tests (2025-05-07).
- [ ] Consider surfacing non-geometry asset error categories in streaming telemetry for parity (owner: Runtime, priority: low, rationale: broader failure attribution).
