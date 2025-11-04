# CR-125 & CR-130 Implementation & Review Log

## PRIORITY_DECISION
Selected Task: CR-125 — Plugin lifecycle audit (with dependent CR-130 documentation refresh)
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| CR-125 | 4 | 4 | 4 | 4 | 3 | 5 | 24 |
| CR-130 | 3 | 3 | 3 | 3 | 4 | 4 | 20 |
| DC-004 follow-up doc sweep | 1 | 2 | 2 | 2 | 4 | 3 | 14 |
Tie-break Rationale: CR-125 precedes CR-130 on the module roadmap and strengthens DC-001 invariants.
Decision Rationale
- Lifecycle correctness touches every runtime host; regressions would ripple into all subsystems.
- Auditing lifecycle invariants unblocks the pending configuration doc refresh and upcoming runtime work.
- Risk of not doing is high: partial initialization currently lacks regression coverage and documented sequencing.
- Effort is moderate (targeted runtime changes + docs) and aligns tightly with the core module roadmap.
- Documentation refresh is naturally bundled after lifecycle fixes to keep guidance synchronized.

## DESIGN_BRIEF
Problem Statement
- Runtime subsystem plugins lack regression coverage for lifecycle ordering and context propagation.
- `RuntimeHost::initialize` does not guarantee teardown of already-started subsystems when a later plugin throws.
- Core documentation around plugin lifecycle/configuration is stale, still pointing to legacy summaries.

Acceptance Criteria
- `RuntimeHost` ensures partially initialized plugins are shut down when initialization fails and preserves determinism of lifecycle order.
- Lifecycle context exposes the runtime/scene identifier consistently; regression tests cover both success and failure paths.
- Core README/roadmap, central roadmap, and plugin architecture notes describe initialization/shutdown sequencing and configuration defaults (IO thread pool, runtime naming).
- Configuration documentation enumerates default values and override mechanisms for `RuntimeHostDependencies::streaming_config` and subsystem selection.

Interfaces & Data Flow
- Touch `engine/runtime/src/api.cpp` to wrap subsystem initialization in guarded logic.
- Extend `engine/runtime/tests/test_module.cpp` with instrumentation-only plugins for assertions on lifecycle order and context.
- Update `engine/core/plugin/isubsystem_interface.hpp` comments describing lifecycle expectations.
- Refresh docs in `docs/modules/core/README.md`, `docs/modules/core/ROADMAP.md`, `docs/ROADMAP.md`, `README.md`, and `docs/design/plugin_architecture.md`.

Invariants & Edge Cases
- Ensure `RuntimeHost::initialize` remains idempotent: repeated calls without intervening `shutdown` should no-op.
- Preserve existing telemetry accounting (initialize/shutdown/tick counters) when tearing down partially initialized plugins.
- Guard against plugins returning `nullptr` and skip them gracefully (current behaviour must remain).
- Thread pool configuration must unwind when initialization fails to avoid leaking worker threads into later tests.

Compatibility & Migration
- No ABI surface changes; lifecycle contract remains the same but documented more explicitly.
- Plugins implementing `ISubsystemInterface` require no code changes beyond complying with documented invariants.

Security & Performance Considerations
- Shutdown on failure must avoid throwing; rely on interface `noexcept` guarantee and swallow unexpected exceptions defensively if necessary.
- Additional bookkeeping is limited to a short vector of initialized plugins, avoiding allocations on the happy path beyond existing work.
- Tests should not extend runtime duration significantly; keep instrumentation lightweight.

Test Strategy
- Add GTest cases covering successful lifecycle context propagation, teardown on failure, and recovery after transient failure.
- Run `cmake --build --preset linux-gcc-debug --target engine_runtime_tests` and `ctest --preset linux-gcc-debug --tests-regex engine_runtime`.
- Execute `python scripts/validate_docs.py` after documentation updates.

## PATCH
```diff
diff --git a/engine/runtime/src/api.cpp b/engine/runtime/src/api.cpp
@@ -1040,18 +1040,46 @@ namespace engine::runtime
             synchronize_scene_graph(translation);
             const engine::core::plugin::SubsystemLifecycleContext lifecycle{runtime_name_view()};
-            for (const auto& plugin : dependencies.subsystem_plugins)
+            std::vector<std::shared_ptr<core::plugin::ISubsystemInterface>> started_plugins{};
+            started_plugins.reserve(dependencies.subsystem_plugins.size());
+            try
             {
-                if (plugin != nullptr)
+                for (const auto& plugin : dependencies.subsystem_plugins)
                 {
+                    if (plugin == nullptr)
+                    {
+                        continue;
+                    }
+
                     const std::string name{plugin->name()};
                     const auto start = Clock::now();
                     plugin->initialize(lifecycle);
+                    started_plugins.push_back(plugin);
                     const auto duration = Clock::now() - start;
                     record_subsystem_event(name, duration, SubsystemPhase::Initialize);
                 }
-            }
-            initialized = true;
+                initialized = true;
+            }
+            catch (...)
+            {
+                for (auto it = started_plugins.rbegin(); it != started_plugins.rend(); ++it)
+                {
+                    if (*it == nullptr)
+                    {
+                        continue;
+                    }
+
+                    const std::string name{(*it)->name()};
+                    const auto start = Clock::now();
+                    (*it)->shutdown(lifecycle);
+                    const auto duration = Clock::now() - start;
+                    record_subsystem_event(name, duration, SubsystemPhase::Shutdown);
+                }
+
+                core::threading::IoThreadPool::instance().shutdown();
+                throw;
+            }

             record_initialize_duration(Clock::now() - initialize_start);
             refresh_streaming_metrics();
             rebuild_metric_snapshot();
diff --git a/engine/core/include/engine/core/plugin/isubsystem_interface.hpp b/engine/core/include/engine/core/plugin/isubsystem_interface.hpp
@@
-struct SubsystemLifecycleContext {
-    std::string_view runtime_name{};
-};
-
-struct SubsystemUpdateContext {
-    double delta_time{0.0};
-};
-
-class ISubsystemInterface {
+struct SubsystemLifecycleContext {
+    /// Human-readable identifier for the runtime instance requesting lifecycle events.
+    ///
+    /// When invoked by the runtime host this maps to the active scene name. Plugins
+    /// must treat the value as non-owning and avoid caching dangling references.
+    std::string_view runtime_name{};
+};
+
+struct SubsystemUpdateContext {
+    /// Simulation timestep in seconds supplied to `tick`.
+    double delta_time{0.0};
+};
+
+/// Subsystems discovered at runtime implement this interface to integrate with the
+/// host lifecycle. Implementations must be deterministic and re-entrant: `initialize`
+/// may run multiple times across the process lifetime, and `shutdown` is always
+/// invoked in reverse registration order whenever initialization succeeds.
+class ISubsystemInterface {
 public:
     virtual ~ISubsystemInterface() = default;

     [[nodiscard]] virtual std::string_view name() const noexcept = 0;

     [[nodiscard]] virtual std::span<const std::string_view> dependencies() const noexcept = 0;

-    virtual void initialize(const SubsystemLifecycleContext& context) = 0;

-    virtual void shutdown(const SubsystemLifecycleContext& context) noexcept = 0;
+    /// Prepare the subsystem for use. Implementations may throw to signal unrecoverable
+    /// startup failures; the runtime host guarantees previously initialized subsystems are
+    /// shut down before the exception propagates.
+    virtual void initialize(const SubsystemLifecycleContext& context) = 0;

+    /// Tear down the subsystem. Must be noexcept and idempotent because the runtime host
+    /// will invoke `shutdown` when initialization fails partway through and again during
+    /// normal teardown.
+    virtual void shutdown(const SubsystemLifecycleContext& context) noexcept = 0;

     virtual void tick(const SubsystemUpdateContext& context) = 0;
```

## TESTS
```diff
diff --git a/engine/runtime/tests/test_module.cpp b/engine/runtime/tests/test_module.cpp
@@ -139,6 +139,75 @@ std::shared_ptr<engine::core::plugin::ISubsystemInterface> make_test_subsystem(
     return std::make_shared<TestSubsystem>(std::move(name), std::move(dependencies));
 }

+class RecordingLifecycleSubsystem final : public engine::core::plugin::ISubsystemInterface
+{
+public:
+    explicit RecordingLifecycleSubsystem(std::string name, std::vector<std::string> dependencies = {})
+        : name_(std::move(name)), dependencies_storage_(std::move(dependencies))
+    {
+        dependency_views_.reserve(dependencies_storage_.size());
+        for (const auto& dependency : dependencies_storage_)
+        {
+            dependency_views_.push_back(dependency);
+        }
+    }
+
+    [[nodiscard]] std::string_view name() const noexcept override
+    {
+        return name_;
+    }
+
+    [[nodiscard]] std::span<const std::string_view> dependencies() const noexcept override
+    {
+        return dependency_views_;
+    }
+
+    void initialize(const engine::core::plugin::SubsystemLifecycleContext& context) override
+    {
+        initialize_calls += 1U;
+        initialize_contexts.emplace_back(context.runtime_name);
+        if (remaining_failures > 0U)
+        {
+            --remaining_failures;
+            throw std::runtime_error("RecordingLifecycleSubsystem forced failure");
+        }
+    }
+
+    void shutdown(const engine::core::plugin::SubsystemLifecycleContext& context) noexcept override
+    {
+        shutdown_calls += 1U;
+        shutdown_contexts.emplace_back(context.runtime_name);
+    }
+
+    void tick(const engine::core::plugin::SubsystemUpdateContext&) override {}
+
+    void fail_initialization(std::size_t count = 1U) noexcept
+    {
+        remaining_failures = count;
+    }
+
+    void clear_failures() noexcept
+    {
+        remaining_failures = 0U;
+    }
+
+    std::string name_{};
+    std::vector<std::string> dependencies_storage_{};
+    std::vector<std::string_view> dependency_views_{};
+    std::size_t initialize_calls{0U};
+    std::size_t shutdown_calls{0U};
+    std::vector<std::string> initialize_contexts{};
+    std::vector<std::string> shutdown_contexts{};
+    std::size_t remaining_failures{0U};
+};

+std::shared_ptr<RecordingLifecycleSubsystem> make_recording_subsystem(
+    std::string name,
+    std::vector<std::string> dependencies = {})
+{
+    return std::make_shared<RecordingLifecycleSubsystem>(std::move(name), std::move(dependencies));
+}

@@ -683,6 +752,94 @@ TEST(RuntimeHost, LoadsSubsystemsFromRegistrySelection) {
     host.shutdown();
 }

+TEST(RuntimeHost, ProvidesLifecycleContextForSubsystems)
+{
+    auto plugin = make_recording_subsystem("alpha");
+
+    engine::runtime::RuntimeHostDependencies deps{};
+    deps.scene_name = "runtime.alpha";
+    deps.subsystem_plugins = {plugin};
+
+    engine::runtime::RuntimeHost host{deps};
+    host.initialize();
+
+    ASSERT_EQ(plugin->initialize_calls, 1U);
+    ASSERT_EQ(plugin->initialize_contexts.size(), 1U);
+    EXPECT_EQ(plugin->initialize_contexts.front(), deps.scene_name);
+
+    host.shutdown();
+
+    ASSERT_EQ(plugin->shutdown_calls, 1U);
+    ASSERT_EQ(plugin->shutdown_contexts.size(), 1U);
+    EXPECT_EQ(plugin->shutdown_contexts.front(), deps.scene_name);
+}
+
+TEST(RuntimeHost, ShutsDownInitializedSubsystemsWhenLaterPluginFails)
+{
+    auto first = make_recording_subsystem("alpha");
+    auto second = make_recording_subsystem("beta");
+    second->fail_initialization();
+
+    engine::runtime::RuntimeHostDependencies deps{};
+    deps.scene_name = "runtime.failure";
+    deps.subsystem_plugins = {first, second};
+
+    engine::runtime::RuntimeHost host{deps};
+
+    EXPECT_THROW(host.initialize(), std::runtime_error);
+    EXPECT_FALSE(host.is_initialized());
+
+    EXPECT_EQ(first->initialize_calls, 1U);
+    EXPECT_EQ(first->shutdown_calls, 1U);
+    EXPECT_EQ(second->initialize_calls, 1U);
+    EXPECT_EQ(second->shutdown_calls, 0U);
+    ASSERT_FALSE(first->initialize_contexts.empty());
+    ASSERT_FALSE(first->shutdown_contexts.empty());
+    EXPECT_EQ(first->initialize_contexts.front(), deps.scene_name);
+    EXPECT_EQ(first->shutdown_contexts.front(), deps.scene_name);
+
+    const auto& diagnostics = host.diagnostics();
+    const auto timing_it = std::find_if(
+        diagnostics.subsystem_timings.begin(),
+        diagnostics.subsystem_timings.end(),
+        [](const engine::runtime::RuntimeSubsystemTiming& timing) { return timing.name == "alpha"; });
+    ASSERT_NE(timing_it, diagnostics.subsystem_timings.end());
+    EXPECT_EQ(timing_it->initialize_count, 1U);
+    EXPECT_EQ(timing_it->shutdown_count, 1U);
+    EXPECT_EQ(timing_it->tick_count, 0U);
+
+    host.shutdown();
+}
+
+TEST(RuntimeHost, RecoversAfterSubsystemInitializationFailure)
+{
+    auto first = make_recording_subsystem("alpha");
+    auto second = make_recording_subsystem("beta");
+    second->fail_initialization();
+
+    engine::runtime::RuntimeHostDependencies deps{};
+    deps.subsystem_plugins = {first, second};
+
+    engine::runtime::RuntimeHost host{deps};
+
+    EXPECT_THROW(host.initialize(), std::runtime_error);
+    EXPECT_FALSE(host.is_initialized());
+    EXPECT_EQ(first->initialize_calls, 1U);
+    EXPECT_EQ(first->shutdown_calls, 1U);
+
+    second->clear_failures();
+
+    EXPECT_NO_THROW(host.initialize());
+    EXPECT_TRUE(host.is_initialized());
+    EXPECT_EQ(first->initialize_calls, 2U);
+    EXPECT_EQ(second->initialize_calls, 2U);
+
+    host.shutdown();
+
+    EXPECT_EQ(first->shutdown_calls, 2U);
+    EXPECT_EQ(second->shutdown_calls, 1U);
+}
```

## DOCS
```diff
diff --git a/docs/modules/core/README.md b/docs/modules/core/README.md
@@
- Track `CR-125`, `CR-130` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — maintains `DC-001` and supports `CC-001`.
- Next roadmap slice: `CR-135` dependency cycle diagnostics for subsystem plugins.
@@
| `CR-125` | Audit plugin lifecycle contracts (`DC-001`). | Document init/shutdown sequencing, add regression coverage. | ✅ Done |
| `CR-130` | Update configuration docs. | Refresh config API README section with latest presets and defaults. | ✅ Done |
| `CR-135` | Subsystem dependency diagnostics. | Detect cycles during registration and document remediation guidance. | 🟡 Planned |

## Subsystem Lifecycle Contract

- `engine::core::plugin::ISubsystemInterface` implementers receive lifecycle callbacks in
  registration order. `initialize` is invoked sequentially; failures trigger shutdown of any
  previously started subsystems in reverse order to maintain `DC-001` determinism.
- `SubsystemLifecycleContext.runtime_name` mirrors the active runtime scene identifier.
  Treat the string view as non-owning; copy when persisting beyond the callback.
- `shutdown` is always invoked when initialization succeeds and when a later plugin fails.
  Implementations must be idempotent and `noexcept`. Regression coverage lives in
  `engine/runtime/tests/test_module.cpp` (see lifecycle tests).

## Configuration Defaults

- `RuntimeHostDependencies::streaming_config` defaults to two IO workers, queue capacity
  of 64, and async streaming enabled. Override per runtime by mutating the struct before
  constructing `RuntimeHost` or via subsystem configuration presets.
- Subsystem selection derives from either explicit `subsystem_plugins` or discovery through
  `SubsystemRegistry::load`. When `enabled_subsystems` is empty the runtime loads modules
  flagged `enabled_by_default` and resolves dependencies transitively.
- The runtime name (`RuntimeHostDependencies::scene_name`) feeds diagnostic output and the
  subsystem lifecycle context. Align naming across tooling and telemetry for consistent
  aggregation.

diff --git a/docs/design/plugin_architecture.md b/docs/design/plugin_architecture.md
@@
-The runtime subsystem plugin contract now lives in the
-[Runtime module README](../modules/runtime/README.md#subsystem-plugin-contract).
-Consult that section for lifecycle expectations, helper APIs, and configuration guidance.
+Subsystem plugins exposed through `engine::core::plugin::ISubsystemInterface` enable
+modules to opt into runtime orchestration without direct linking. This document captures
+the authoritative lifecycle expectations aligned with `DC-001`.
+
+## Lifecycle Sequencing
+
+- Plugins are initialized sequentially in the order they are registered with the runtime
+  host or resolved from `SubsystemRegistry`. Each call receives a
+  `SubsystemLifecycleContext` whose `runtime_name` mirrors the active scene/runtime ID.
+- If any plugin throws during `initialize`, the runtime host immediately shuts down all
+  previously initialized plugins in reverse order before rethrowing. This preserves
+  determinism and prevents partially initialized subsystems from leaking resources.
+- `shutdown` is invoked exactly once per successful initialization and is guaranteed to
+  execute during normal teardown and after initialization failures. Implementations must be
+  `noexcept` and idempotent.
+- `tick` follows the same registration order on every frame. Subsystems should treat the
+  supplied `SubsystemUpdateContext::delta_time` as authoritative.

diff --git a/README.md b/README.md
@@
-| Core | ✅ Stable | EnTT-backed registry façade, subsystem discovery helpers, and module bootstrap plumbing consumed by higher-level systems. | `CR-125`: plugin lifecycle audit to keep `DC-001` fresh. |
+| Core | ✅ Stable | EnTT-backed registry façade, subsystem discovery helpers, and module bootstrap plumbing consumed by higher-level systems. | `CR-135`: subsystem dependency diagnostics to harden `DC-001`. |

diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
@@
-- **Core** — `CR-118` diagnostics bridge specification (`CC-001`), followed by
-  `CR-125` plugin lifecycle audit to keep `DC-001` fresh.
+- **Core** — Completed `CR-125` lifecycle audit and `CR-130` configuration refresh;
+  next deliverable is `CR-135` subsystem dependency diagnostics to extend `DC-001`.
```

## VERIFY
- `cmake --preset linux-gcc-debug` — configured project; GLFW disabled due to missing Xrandr headers in container (expected).【e097b5†L1-L18】
- `cmake --build --preset linux-gcc-debug --target engine_runtime_tests` — compiled runtime + dependencies (warnings unchanged from baseline).【e0c059†L1-L1】【c94350†L1-L11】【0b799a†L1-L1】
- `ctest --preset linux-gcc-debug --tests-regex engine_runtime` — runtime suite passed.【41d98e†L1-L3】【8e8887†L1-L5】
- `python scripts/validate_docs.py` — documentation links validated.【bc1455†L1-L2】

## REVIEW_FINDINGS
- Correctness: ✅ RuntimeHost now unwinds partially initialized plugins; new tests cover success, failure, and recovery paths.
- Documentation: ✅ Core README/roadmap, design note, and central roadmap updated alongside implementation.
- Observability: ✅ Structured logging for initialization failures landed via `CR-136`, enabling downstream diagnostics to correlate lifecycle telemetry with failure context.
- Tests: ✅ Runtime suite extended; build + targeted ctest executed.

## REVIEW_PATCHES
N/A — reviewer approved without additional patches.

## FINAL_PATCH
Final patch equals the implementation diffs above; no post-review adjustments required.

## FOLLOW_UP_TODOS
- [ ] Catalogue plugin dependency cycle detection (`CR-135` placeholder) (owner: Core, priority: medium) — ensure registry rejects cycles and document diagnostics.
- [x] Expand observability around subsystem initialization failures via structured logging (owner: Runtime, priority: low) — forward metrics to diagnostics bridge.
- [ ] Audit long-term: refactor subsystem registry to cache dependency resolutions (owner: Runtime, priority: low) — reduce repeated traversals.
- [ ] Publish runbook for subsystem configuration overrides in `docs/modules/runtime/` (owner: Docs, priority: medium).
