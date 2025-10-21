# CC-002.3 Shader Hot Reload Failure Diagnostics

## PRIORITY_DECISION
Selected Task: CC-002.3 — Shader cache Result-based reload & telemetry
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| CC-002.3 Shader cache | 4 | 4 | 5 | 4 | 3 | 5 | 25 |
| CC-002.3 Runtime diagnostics | 3 | 4 | 3 | 3 | 2 | 5 | 20 |
| CC-002.3 Python telemetry viewer | 3 | 3 | 2 | 3 | 3 | 4 | 18 |
Tie-break Rationale: Highest total score with maximal unblocking value for downstream diagnostics work.
Decision Rationale
- Shader cache is the last asset cache lacking telemetry-aware reload semantics, blocking accurate diagnostics for shader failures.
- Runtime API and Python bindings depend on the shader cache emitting structured telemetry before snapshot wiring delivers meaningful data.
- Risk of inconsistent asset state during shader reload failures remains until this refactor lands, threatening runtime stability.
- Effort is moderate compared with multi-module runtime updates, fitting within current window without delaying follow-up tasks.
- Directly aligns with roadmap item `CC-002.3` and keeps module behaviour consistent with other caches that already migrated.

## DESIGN_BRIEF
Problem Statement
- Shader cache still throws exceptions during reload, skipping telemetry updates and risking partial state updates on failure.
- Watcher callbacks and polling ignore error codes, so runtime will not observe shader reload failures, undermining diagnostics goals.

Acceptance Criteria
- `ShaderCache::reload_asset` returns `engine::Result<void, AssetLoadError>` and records telemetry attempts/failures mirroring other caches.
- `ShaderCache::load`, watcher callbacks, and `poll()` propagate `Result` outcomes, preserving previous shader state when reload fails.
- Telemetry records include helpful remediation hints for IO failures during shader reload.
- Unit tests continue to compile; existing async tests remain valid without modification.

Interfaces & Data Flow
- `ShaderCache::reload_asset` loads source into temporary buffers, compiles to SPIR-V, checks timestamps, then swaps into live asset on success.
- Telemetry hooks via `detail::record_hot_reload_attempt/failure` using shader identifier; no public API signature changes besides error handling semantics.

Invariants & Compatibility
- Preserve deterministic cache binding semantics: handles remain valid across reload attempts.
- Maintain `ShaderCache::load` throwing behaviour for callers expecting exceptions on failure.
- Ensure no partial state commits when reload fails (source, binary, last_write remain unchanged).

Security & Performance Considerations
- Avoid holding mutex across filesystem operations longer than necessary; operations remain inside lock but mimic existing caches—acceptable for now.
- File reading uses streaming I/O; no new allocations beyond temporary buffers already required.

Test Strategy
- Rely on existing shader cache usage tests (compilation ensures coverage); run `ctest --preset linux-gcc-debug --tests-regex engine_assets` after build.
- No new Python/tests needed for this step; verify compilation only.

## PATCH
```diff
diff --git a/engine/assets/include/engine/assets/shader_asset.hpp b/engine/assets/include/engine/assets/shader_asset.hpp
index e9453e8..f5ed976 100644
--- a/engine/assets/include/engine/assets/shader_asset.hpp
+++ b/engine/assets/include/engine/assets/shader_asset.hpp
@@
-#include "engine/assets/handles.hpp"
+#include "engine/assets/async.hpp"
+#include "engine/assets/handles.hpp"
@@
-    void reload_asset(const RawHandle& handle, ShaderAsset& asset, bool notify);
+    engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, ShaderAsset& asset, bool notify);

diff --git a/engine/assets/src/shader_asset.cpp b/engine/assets/src/shader_asset.cpp
index 6bf5224..e6ae91d 100644
--- a/engine/assets/src/shader_asset.cpp
+++ b/engine/assets/src/shader_asset.cpp
@@
-[[nodiscard]] std::string read_text(const std::filesystem::path& path)
+[[nodiscard]] engine::Result<std::string, AssetLoadError> read_text(const std::filesystem::path& path)
@@
-    std::string contents(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
-    if (stream.bad()) {
-        throw std::runtime_error("Failed to read shader file: " + path.generic_string());
-    }
-
-    return contents;
+    std::string contents{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
+    if (stream.bad())
+    {
+        return make_asset_load_error(AssetLoadErrorCategory::IoFailure,
+                                     "Failed to read shader file: " + path.generic_string());
+    }
+
+    return contents;
@@
-        reload_asset(handle, *asset, !inserted);
+        if (auto reload = reload_asset(handle, *asset, !inserted); !reload.has_value())
+        {
+            const auto message = reload.error().message();
+            throw std::runtime_error(message.empty() ? std::string{to_string(reload.error().code())}
+                                                     : std::string{message});
+        }
@@
-            reload_asset(handle, asset, true);
+            if (auto reload = reload_asset(handle, asset, true); !reload.has_value())
+            {
+                return;
+            }
@@
-void ShaderCache::reload_asset(const RawHandle& handle, ShaderAsset& asset, bool notify)
+engine::Result<void, AssetLoadError> ShaderCache::reload_asset(const RawHandle& handle,
+                                                             ShaderAsset& asset,
+                                                             bool notify)
@@
-    asset.source = read_text(asset.descriptor.source);
-    asset.binary = ShaderCompiler::compile_glsl_to_spirv(asset.source, asset.descriptor.options);
-    asset.last_write = detail::checked_last_write_time(asset.descriptor.source, "shader");
+    const std::string identifier = asset.descriptor.handle.id();
+    detail::record_hot_reload_attempt(notify, identifier);
+
+    auto source = read_text(asset.descriptor.source);
+    if (!source)
+    {
+        auto error = source.error();
+        detail::record_hot_reload_failure(notify, identifier, error,
+                                          "Verify the shader file exists and is readable by the runtime process.");
+        return error;
+    }
+
+    ShaderBinary compiled = ShaderCompiler::compile_glsl_to_spirv(source.value(), asset.descriptor.options);
+
+    std::filesystem::file_time_type last_write{};
+    try
+    {
+        last_write = detail::checked_last_write_time(asset.descriptor.source, "shader");
+    }
+    catch (const std::runtime_error& ex)
+    {
+        auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
+        detail::record_hot_reload_failure(
+            notify, identifier, error,
+            "Ensure the shader file remains on disk and accessible during reload attempts.");
+        return error;
+    }
+
+    asset.source = std::move(source.value());
+    asset.binary = std::move(compiled);
+    asset.last_write = last_write;
@@
-        auto& tracked = assets_.get(handle);
-        reload_asset(handle, tracked, true);
+        auto& tracked = assets_.get(handle);
+        if (auto reload = reload_asset(handle, tracked, true); !reload.has_value())
+        {
+            return;
+        }

diff --git a/engine/assets/include/engine/assets/graph_asset.hpp b/engine/assets/include/engine/assets/graph_asset.hpp
index 0cc8af9..40854cb 100644
--- a/engine/assets/include/engine/assets/graph_asset.hpp
+++ b/engine/assets/include/engine/assets/graph_asset.hpp
@@
-#include "engine/assets/handles.hpp"
+#include "engine/assets/async.hpp"
+#include "engine/assets/handles.hpp"

diff --git a/engine/assets/include/engine/assets/texture_asset.hpp b/engine/assets/include/engine/assets/texture_asset.hpp
index f884d63..7897c0b 100644
--- a/engine/assets/include/engine/assets/texture_asset.hpp
+++ b/engine/assets/include/engine/assets/texture_asset.hpp
@@
-#include "engine/assets/handles.hpp"
+#include "engine/assets/async.hpp"
+#include "engine/assets/handles.hpp"
```

## TESTS
N/A

## DOCS
N/A

## VERIFY
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target engine_assets`
- `cmake --build --preset linux-gcc-debug --target engine_assets_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_assets`

## REVIEW_FINDINGS
### Summary
- Shader cache now mirrors other asset caches by surfacing `engine::Result` reload outcomes and emitting telemetry for recoverable failures.

### Architectural Impact
- Preserves cache lifecycle invariants (`AI-001`) by keeping shader handles bound only after successful reloads.
- Aligns with `CC-002.3` diagnostics requirements by ensuring telemetry records attempts/failures for shader hot reloads.

### Findings
#### Critical Issues 🔴
- None.

#### Warnings ⚠️
- None.

#### Suggestions 💡
- Consider extracting shared file-loading helpers across caches in a follow-up to reduce duplicated IO error handling logic.

### Documentation Status
- No documentation updates required beyond this implementation log; module READMEs already describe hot-reload telemetry.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Matches the patch set recorded above.

## FOLLOW_UP_TODOS
- [ ] Wire runtime diagnostics snapshot once shader telemetry is available (owner: TBD, priority high, prerequisite for CC-002.3 completion)
- [ ] Ensure telemetry viewer integrations serialize shader hot reload counters (owner: TBD, priority medium, maintain observability parity)
- [ ] Consider refactoring shared cache reload logic to reduce duplication (owner: TBD, priority low, tech debt)
- [ ] Update module READMEs with consolidated telemetry guidance post end-to-end integration (owner: TBD, priority medium)
