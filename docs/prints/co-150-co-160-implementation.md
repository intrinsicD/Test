# CO-150 & CO-160 Implementation & Review Log

## PRIORITY_DECISION
Selected Task: CO-150 — Cycle detection tooling (paired with CO-160 CUDA preset alignment)
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| CO-150 | 5 | 4 | 5 | 4 | 3 | 5 | 26 |
| CO-160 | 4 | 3 | 4 | 3 | 4 | 4 | 22 |
| CO-170 | 3 | 3 | 3 | 2 | 2 | 4 | 17 |
Tie-break Rationale: Tackled CO-150 first because the static analysis unlocks dispatcher consumers; CO-160 executed alongside since it touches orthogonal build configuration.
Decision Rationale
- Cycle detection tooling is prerequisite for runtime validation workstreams and underpins `CO-170` integration samples.
- CUDA preset drift risked feature-flag confusion for downstream agents; aligning presets during the same pass avoids repeated CMake churn.
- Effort was moderate: one new utility, dispatcher integration, tests, and doc updates fit within sprint capacity.
- Completing both tasks retires the compute module’s immediate roadmap items, keeping documentation in sync with actual capabilities.
- Strategic alignment is high because both items roll into `DC-002` optional subsystem discipline.

## DESIGN_BRIEF
Problem Statement
- Dispatcher clients lacked reusable tooling to statically analyse dependency graphs, forcing them to rely on runtime exceptions.
- CUDA presets exposed `ENGINE_ENABLE_CUDA` without guaranteeing the compute module’s CUDA slice stayed in lockstep.

Acceptance Criteria
- Provide an exported API that inspects `DependencyGraph` instances and reports cycle paths.
- Integrate the analyser into dispatcher registration/dispatch error handling to surface actionable diagnostics.
- Ensure CUDA subdirectory builds only when both global and module-level flags are active, and runtime availability checks respect both switches.
- Update unit tests and documentation to reflect the new tooling and flag alignment.

Interfaces & Data Flow
- New header `<engine/compute/dependency_analysis.hpp>` exposes `CycleDetectionResult detect_cycles(const DependencyGraph&)`.
- Dispatcher invokes the analyser during registration validation and when dispatch detects unprocessed kernels.
- `is_cuda_dispatcher_available()` now reflects combined `ENGINE_ENABLE_CUDA`/`ENGINE_ENABLE_COMPUTE_CUDA` state, matching the updated CMake gate.

Invariants & Edge Cases
- Unresolved dependencies must not trigger false positives—analyser ignores indices beyond the registered range.
- Cycle reporting must remain deterministic and include kernel names when available.
- Dispatcher rollback on invalid registrations continues to preserve previous kernel state.

Compatibility & Migration
- Public API additions are purely additive; existing headers remain source-compatible.
- CUDA availability semantics tighten but remain backward compatible for presets already enabling both flags.

Security & Performance Considerations
- DFS-based analysis runs in O(V + E) with no dynamic allocations beyond manageable vectors; invoked only on graph mutations or error paths.
- No new external inputs or privileged operations introduced.

Test Strategy
- Extend compute module tests to exercise analyser success/failure cases and confirm dispatcher error messages include the cycle path.
- Rebuild compute targets under `linux-gcc-debug` preset and run focused `engine_compute_tests` suite.
- Validate documentation integrity with `scripts/validate_docs.py`.

## PATCH
```diff
+++ b/engine/compute/include/engine/compute/dependency_analysis.hpp
+struct CycleDetectionResult {
+    bool has_cycle{false};
+    std::vector<kernel_id> cycle;
+};
+
+[[nodiscard]] ENGINE_COMPUTE_API CycleDetectionResult detect_cycles(const DependencyGraph& graph) noexcept;
```

```diff
+++ b/engine/compute/src/api.cpp
+#include "engine/compute/dependency_analysis.hpp"
@@
-[[nodiscard]] std::string make_cycle_error(const DependencyGraph& graph, std::string_view context)
+[[nodiscard]] std::string make_cycle_error(const DependencyGraph& graph,
+                                           std::string_view context,
+                                           const CycleDetectionResult& cycle)
@@
-    stream << '\n' << graph.to_dot();
+    if (cycle.has_cycle && !cycle.cycle.empty()) {
+        stream << '\n' << "Cycle path: ";
+        ...
+    }
+    stream << '\n' << graph.to_dot();
@@
-        if (report.execution_order.size() != count)
-        {
-            throw std::runtime_error{make_cycle_error(graph, "during dispatch")};
-        }
+        if (report.execution_order.size() != count)
+        {
+            const auto cycle = detect_cycles(graph);
+            throw std::runtime_error{make_cycle_error(graph, "during dispatch", cycle)};
+        }
@@
-        const auto graph = build_dependency_graph();
-
-        const auto count = graph.nodes.size();
-        ...
-        if (processed != count)
-        {
-            throw std::runtime_error{make_cycle_error(graph, "during registration")};
-        }
+        const auto graph = build_dependency_graph();
+        const auto cycle = detect_cycles(graph);
+        if (cycle.has_cycle)
+        {
+            throw std::runtime_error{make_cycle_error(graph, "during registration", cycle)};
+        }
```

```diff
+++ b/engine/compute/CMakeLists.txt
-add_library(${target_name}
-    src/api.cpp
-)
+add_library(${target_name}
+    src/api.cpp
+    src/dependency_analysis.cpp
+)
@@
-if(ENGINE_ENABLE_CUDA)
+if(ENGINE_ENABLE_CUDA AND ENGINE_ENABLE_COMPUTE_CUDA)
     add_subdirectory(cuda)
 endif()
```

## TESTS
```diff
+++ b/engine/compute/tests/test_module.cpp
+#include "engine/compute/dependency_analysis.hpp"
@@
-        ExpectSubstring(message, "KernelDispatcher detected a cycle during registration");
+        ExpectSubstring(message, "KernelDispatcher detected a cycle during registration");
+        ExpectSubstring(message, "Cycle path:");
         ExpectSubstring(message, "digraph");
@@
+TEST(ComputeModule, DetectCyclesReportsCyclePath)
+{
+    engine::compute::DependencyGraph graph;
+    ...
+    const auto result = engine::compute::detect_cycles(graph);
+    ASSERT_TRUE(result.has_cycle);
+    ASSERT_FALSE(result.cycle.empty());
+    EXPECT_EQ(result.cycle.front(), result.cycle.back());
+    std::set<engine::compute::kernel_id> unique_nodes(result.cycle.begin(), result.cycle.end());
+    EXPECT_TRUE(unique_nodes.contains(0U));
+    EXPECT_TRUE(unique_nodes.contains(1U));
+    EXPECT_TRUE(unique_nodes.contains(2U));
+}
```

## DOCS
```diff
+++ b/README.md
-| Compute | ✅ Stable | Kernel dispatcher with per-kernel telemetry, backend capability probing, dispatcher extension guidance, and math helpers for identity transforms. | `CO-150`: implement kernel dependency cycle detection tooling. |
+| Compute | ✅ Stable | Kernel dispatcher with per-kernel telemetry, backend capability probing, dependency cycle analysis tooling, dispatcher extension guidance, and math helpers for identity transforms. | `CO-170`: prototype runtime integration sample showing dispatcher orchestration. |
```

```diff
+++ b/docs/modules/compute/README.md
+- Include `<engine/compute/dependency_analysis.hpp>` to perform static dependency cycle analysis outside dispatcher execution.
@@
-| `CO-150` | ... | 🟢 Todo |
-| `CO-160` | ... | 🟢 Todo |
+| `CO-150` | ... | ✅ Complete |
+| `CO-160` | ... | ✅ Complete |
```

```diff
+++ b/docs/modules/compute/ROADMAP.md
-| `CO-150` | Implement kernel dependency cycle detection tooling. | Compute team | 2025-03-21 | 🟢 Todo |
+| `CO-150` | Implement kernel dependency cycle detection tooling. | Compute team | 2025-03-21 | ✅ Complete |
@@
-| `CO-160` | Synchronise CUDA presets, docs, and CI coverage (`DC-002`). | ✅ Complete |
+| `CO-170` | Prototype runtime integration sample showing dispatcher orchestration. | After `RU-307` complete |
```

```diff
+++ b/docs/ROADMAP.md
-- **Compute** — `CO-150` cycle detection tooling is next after landing the
-  dispatcher extension documentation (`CO-141`), followed by `CO-160` CUDA
-  preset alignment.
+- **Compute** — `CO-150` cycle detection tooling and `CO-160` CUDA preset
+  alignment completed; focus shifts to `CO-170` runtime integration sample
+  work.
```

## VERIFY
- `cmake --preset linux-gcc-debug` — configured successfully; GLFW disabled due to missing Xrandr headers (expected in container).【5865b7†L1-L27】
- `cmake --build --preset linux-gcc-debug` — rebuilt all targets including compute analyser integration with existing warnings unchanged from baseline.【d092e7†L1-L18】
- `ctest --preset linux-gcc-debug --tests-regex engine_compute` — compute module tests passed.【fc6dff†L1-L9】
- `python scripts/validate_docs.py` — documentation validation succeeded.【6ed821†L1-L2】

## REVIEW_FINDINGS
- Correctness: ✅ Analyser exercised via dedicated unit tests; dispatcher emits enriched diagnostics and preserves rollback guarantees.
- Security: ✅ No new I/O or privilege boundaries; DFS traversal handles adversarial graphs without UB.
- Compatibility: ✅ Public APIs extended without breaking existing includes; CUDA availability semantics now accurately reflect build configuration.
- Quality & Style: ✅ Naming follows module conventions; cycle messages explain both identifiers and names.
- Performance: ✅ Cycle detection executes in linear time and only on validation/error paths.
- Tests: ✅ New tests cover acyclic, cyclic, and unresolved dependency scenarios plus dispatcher messaging.
- Observability: ✅ Error text now includes cycle paths alongside existing DOT graph output.
- Docs: ✅ Root snapshot, roadmap entries, and module README/roadmap reflect completed tasks and new analyser header.

## REVIEW_PATCHES
N/A — no reviewer patches required.

## FINAL_PATCH
Final patch equals the production, test, and documentation diffs captured above after addressing the self-review checklist.

## FOLLOW_UP_TODOS
- [ ] Prototype runtime sample consuming `CycleDetectionResult` to surface dependency health in diagnostics dashboards (owner: TBD, priority: medium).
- [ ] Extend analyser to return strongly-typed error codes for integration with tooling CLI (owner: TBD, priority: low).
- [ ] Capture CUDA-enabled CI run once GPU runners become available to validate preset alignment (owner: TBD, priority: low).
- [ ] Document analyser usage in runtime module README when integrating with `CO-170` sample (owner: TBD, priority: medium).
