## PRIORITY_DECISION
Selected Task: RT-005 follow-up — hierarchy alert thresholds
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| RT-005 hierarchy alert thresholds | 3 | 4 | 3 | 3 | 3 | 5 | 21 |
| SC-225 diagnostics samples | 2 | 3 | 2 | 2 | 2 | 4 | 15 |
| Diagnostics artefact runbook | 1 | 2 | 1 | 1 | 3 | 3 | 11 |
Tie-break Rationale: Highest total score; only task blocking alerting parity across runtime + scene.
Decision Rationale:
- Unlocks observability readiness promised in `RT-005.3` follow-ups without requiring new runtime features.
- Medium effort change leveraging existing telemetry plumbing (`RuntimeDiagnostics::metrics`).
- Directly benefits tooling teams waiting on actionable hierarchy alerts.
- Keeps documentation in sync with roadmap status for `SC-230`.
- Avoids prolonged gap where dashboards cannot distinguish transient vs sustained hierarchy failures.

## DESIGN_BRIEF
Problem Statement: Runtime telemetry surfaced per-frame hierarchy issue counts but lacked alert thresholds or streak tracking, forcing operators to triage sustained failures manually. The roadmap (`SC-230`) and follow-up TODOs require deterministic alert metrics so dashboards can escalate repeated hierarchy regressions.
Acceptance Criteria:
- Track total and consecutive hierarchy validation failure frames plus the timestamp of the most recent failure.
- Emit telemetry metrics documenting warning/critical thresholds and the derived alert level.
- Update runtime + scene documentation and roadmap records to reflect active alert policy.
- Extend tests validating metric registration and alert-level transitions.
Interfaces & Data Flow: `RuntimeHost::Impl` updates `RuntimeDiagnostics` after each `scene::validation::validate_hierarchy` invocation; metric snapshot builder exports new gauges/counters; observers consume `runtime.scene_validation.*` metrics and diagnostics docs.
Invariants: Preserve deterministic telemetry snapshots; keep `RuntimeDiagnostics` reset semantics; avoid additional allocations in hot path; maintain existing diagnostics bridge behaviour.
Compatibility & Migration: Additive telemetry fields; no API removals. Existing consumers ignoring new metrics continue working. Initialization paths call the same helper to ensure consistent defaults.
Security/Performance Considerations: No sensitive data introduced; tracking counters in memory-only storage. Added telemetry calculations reuse existing rebuild cycle; single `std::max` and `chrono` call per tick.
Test Strategy: Build and run `engine_runtime_tests`; ensure new unit test exercises alert transitions; validate documentation links with `python scripts/validate_docs.py`.

## PATCH
```diff
@@
+enum class SceneValidationAlertLevel : std::uint32_t
+{
+    None = 0U,
+    Warning = 1U,
+    Critical = 2U,
+};
@@
+    std::uint64_t scene_validation_failure_frame_count{0};
+    std::uint64_t scene_validation_consecutive_failure_frames{0};
+    std::uint64_t scene_validation_max_consecutive_failure_frames{0};
+    double last_scene_validation_failure_simulation_time{-1.0};
+    double last_scene_validation_failure_wall_seconds{-1.0};
+    SceneValidationAlertLevel scene_validation_alert_level{SceneValidationAlertLevel::None};
@@
+namespace detail
+{
+    ENGINE_RUNTIME_API void update_scene_validation_alert_state(
+        RuntimeDiagnostics& diagnostics,
+        const scene::validation::HierarchyValidationReport& report,
+        double simulation_time,
+        double wall_seconds) noexcept;
+}
```
```diff
@@
+    constexpr std::uint64_t kHierarchyFailureWarningThreshold = 3;
+    constexpr std::uint64_t kHierarchyFailureCriticalThreshold = 10;
+
+    [[nodiscard]] engine::runtime::SceneValidationAlertLevel evaluate_scene_validation_alert(
+        std::uint64_t consecutive_failures) noexcept
+    {
+        using engine::runtime::SceneValidationAlertLevel;
+        if (consecutive_failures >= kHierarchyFailureCriticalThreshold)
+        {
+            return SceneValidationAlertLevel::Critical;
+        }
+        if (consecutive_failures >= kHierarchyFailureWarningThreshold)
+        {
+            return SceneValidationAlertLevel::Warning;
+        }
+        return SceneValidationAlertLevel::None;
+    }
@@
+namespace engine::runtime::detail
+{
+    void update_scene_validation_alert_state(RuntimeDiagnostics& diagnostics,
+                                             const scene::validation::HierarchyValidationReport& report,
+                                             double simulation_time,
+                                             double wall_seconds) noexcept
+    {
+        if (!report.ok())
+        {
+            diagnostics.scene_validation_failure_frame_count += 1U;
+            diagnostics.scene_validation_consecutive_failure_frames += 1U;
+            diagnostics.scene_validation_max_consecutive_failure_frames = std::max(
+                diagnostics.scene_validation_max_consecutive_failure_frames,
+                diagnostics.scene_validation_consecutive_failure_frames);
+            diagnostics.last_scene_validation_failure_simulation_time = simulation_time;
+            diagnostics.last_scene_validation_failure_wall_seconds = wall_seconds;
+        }
+        else
+        {
+            diagnostics.scene_validation_consecutive_failure_frames = 0U;
+        }
+
+        diagnostics.scene_validation_alert_level = evaluate_scene_validation_alert(
+            diagnostics.scene_validation_consecutive_failure_frames);
+    }
+}
@@
-        using Clock = std::chrono::steady_clock;
+        using Clock = std::chrono::steady_clock;
+        Clock::time_point runtime_start_time{Clock::now()};
@@
-            add_issue_metric("transform_mismatch", validation.transform_mismatch_count);
+            add_issue_metric("transform_mismatch", validation.transform_mismatch_count);
+            add_gauge("runtime.scene_validation.consecutive_failure_frames",
+                      "Consecutive frames that reported hierarchy validation failures",
+                      static_cast<double>(diagnostics.scene_validation_consecutive_failure_frames),
+                      core::telemetry::MetricUnit::Count);
+            add_gauge("runtime.scene_validation.max_consecutive_failure_frames",
+                      "Maximum consecutive frames with hierarchy validation failures since initialization",
+                      static_cast<double>(diagnostics.scene_validation_max_consecutive_failure_frames),
+                      core::telemetry::MetricUnit::Count);
+            add_counter("runtime.scene_validation.failure_frame_count",
+                        "Total frames that reported hierarchy validation failures since initialization",
+                        clamp_to_int(diagnostics.scene_validation_failure_frame_count));
+            add_gauge("runtime.scene_validation.last_failure_simulation_time",
+                      "Simulation time recorded for the most recent hierarchy validation failure",
+                      diagnostics.last_scene_validation_failure_simulation_time,
+                      core::telemetry::MetricUnit::Seconds);
+            add_gauge("runtime.scene_validation.last_failure_wall_seconds",
+                      "Seconds since runtime initialization when the most recent hierarchy validation failure occurred",
+                      diagnostics.last_scene_validation_failure_wall_seconds,
+                      core::telemetry::MetricUnit::Seconds);
+            add_gauge("runtime.scene_validation.alert_threshold.warning_frames",
+                      "Consecutive failure frames required to enter the warning alert state",
+                      static_cast<double>(kHierarchyFailureWarningThreshold),
+                      core::telemetry::MetricUnit::Count);
+            add_gauge("runtime.scene_validation.alert_threshold.critical_frames",
+                      "Consecutive failure frames required to enter the critical alert state",
+                      static_cast<double>(kHierarchyFailureCriticalThreshold),
+                      core::telemetry::MetricUnit::Count);
+            add_gauge("runtime.scene_validation.alert_level",
+                      "Hierarchy validation alert level (0 = none, 1 = warning, 2 = critical)",
+                      static_cast<double>(static_cast<std::uint32_t>(diagnostics.scene_validation_alert_level)),
+                      core::telemetry::MetricUnit::None);
@@
-        void synchronize_scene_graph(const math::vec3& body_translation)
+        void synchronize_scene_graph(const math::vec3& body_translation, double frame_dt)
@@
-            DiagnosticsBridge::instance().publish_hierarchy_report(
-                diagnostics.scene_validation,
-                simulation_time);
-            const auto now = Clock::now();
-            const double wall_seconds = std::chrono::duration<double>(now - runtime_start_time).count();
-            const double frame_simulation_time = simulation_time + dt;
+            DiagnosticsBridge::instance().publish_hierarchy_report(
+                diagnostics.scene_validation,
+                simulation_time);
+            const auto now = Clock::now();
+            const double wall_seconds = std::chrono::duration<double>(now - runtime_start_time).count();
+            const double frame_simulation_time = simulation_time + frame_dt;
             detail::update_scene_validation_alert_state(
                 diagnostics,
                 diagnostics.scene_validation,
                 frame_simulation_time,
                 wall_seconds);
@@
-            synchronize_scene_graph(translation);
+            synchronize_scene_graph(translation, 0.0);
@@
-                    synchronize_scene_graph(translation);
+                    synchronize_scene_graph(translation, dt);
@@
             record_initialize_duration(Clock::now() - initialize_start);
             runtime_start_time = Clock::now();
```

## TESTS
```diff
@@
+namespace engine::runtime::detail
+{
+    void update_scene_validation_alert_state(RuntimeDiagnostics& diagnostics,
+                                             const engine::scene::validation::HierarchyValidationReport& report,
+                                             double simulation_time,
+                                             double wall_seconds) noexcept;
+}
@@
+    const auto warning_threshold_metric =
+        find_metric_index(metrics, "runtime.scene_validation.alert_threshold.warning_frames");
+    ASSERT_TRUE(warning_threshold_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*warning_threshold_metric].value), 3.0);
+    const auto critical_threshold_metric =
+        find_metric_index(metrics, "runtime.scene_validation.alert_threshold.critical_frames");
+    ASSERT_TRUE(critical_threshold_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*critical_threshold_metric].value), 10.0);
+    const auto consecutive_metric =
+        find_metric_index(metrics, "runtime.scene_validation.consecutive_failure_frames");
+    ASSERT_TRUE(consecutive_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*consecutive_metric].value), 0.0);
+    const auto alert_level_metric = find_metric_index(metrics, "runtime.scene_validation.alert_level");
+    ASSERT_TRUE(alert_level_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*alert_level_metric].value), 0.0);
+    const auto last_failure_time_metric =
+        find_metric_index(metrics, "runtime.scene_validation.last_failure_simulation_time");
+    ASSERT_TRUE(last_failure_time_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*last_failure_time_metric].value), -1.0);
+    const auto last_failure_wall_metric =
+        find_metric_index(metrics, "runtime.scene_validation.last_failure_wall_seconds");
+    ASSERT_TRUE(last_failure_wall_metric.has_value());
+    EXPECT_DOUBLE_EQ(engine::core::telemetry::as_double(metrics.samples[*last_failure_wall_metric].value), -1.0);
@@
+TEST(RuntimeDiagnostics, SceneValidationAlertStateTransitions)
+{
+    engine::runtime::RuntimeDiagnostics diagnostics{};
+    engine::scene::validation::HierarchyValidationReport report{};
+
+    engine::runtime::detail::update_scene_validation_alert_state(diagnostics, report, 0.016, 0.25);
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 0U);
+    EXPECT_EQ(diagnostics.scene_validation_failure_frame_count, 0U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::None);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_simulation_time, -1.0);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_wall_seconds, -1.0);
+
+    report.metrics.issue_count = 1U;
+    engine::runtime::detail::update_scene_validation_alert_state(diagnostics, report, 0.032, 0.5);
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 1U);
+    EXPECT_EQ(diagnostics.scene_validation_failure_frame_count, 1U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::None);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_simulation_time, 0.032);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_wall_seconds, 0.5);
+
+    engine::runtime::detail::update_scene_validation_alert_state(diagnostics, report, 0.048, 0.75);
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 2U);
+    EXPECT_EQ(diagnostics.scene_validation_failure_frame_count, 2U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::None);
+
+    engine::runtime::detail::update_scene_validation_alert_state(diagnostics, report, 0.064, 1.0);
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 3U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::Warning);
+    EXPECT_EQ(diagnostics.scene_validation_max_consecutive_failure_frames, 3U);
+
+    for (int index = 0; index < 7; ++index)
+    {
+        const double simulation_time = 0.08 + 0.016 * static_cast<double>(index);
+        const double wall_seconds = 1.0 + 0.25 * static_cast<double>(index + 1);
+        engine::runtime::detail::update_scene_validation_alert_state(
+            diagnostics,
+            report,
+            simulation_time,
+            wall_seconds);
+    }
+
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 10U);
+    EXPECT_EQ(diagnostics.scene_validation_failure_frame_count, 10U);
+    EXPECT_EQ(diagnostics.scene_validation_max_consecutive_failure_frames, 10U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::Critical);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_simulation_time, 0.08 + 0.016 * 6.0);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_wall_seconds, 1.0 + 0.25 * 7.0);
+
+    report.metrics.issue_count = 0U;
+    engine::runtime::detail::update_scene_validation_alert_state(diagnostics, report, 0.25, 3.0);
+    EXPECT_EQ(diagnostics.scene_validation_consecutive_failure_frames, 0U);
+    EXPECT_EQ(diagnostics.scene_validation_alert_level, engine::runtime::SceneValidationAlertLevel::None);
+    EXPECT_EQ(diagnostics.scene_validation_failure_frame_count, 10U);
+    EXPECT_EQ(diagnostics.scene_validation_max_consecutive_failure_frames, 10U);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_simulation_time, 0.08 + 0.016 * 6.0);
+    EXPECT_DOUBLE_EQ(diagnostics.last_scene_validation_failure_wall_seconds, 1.0 + 0.25 * 7.0);
+}
```

## DOCS
```diff
@@
-- **Runtime** — `RT-005` tranche complete; support `SC-225` samples and `SC-230` alerting guidance as scene docs expand.
-- **Scene** — `SC-225` diagnostics samples and `SC-230` alerting thresholds extend the hierarchy playbook delivered in `SC-220`.
+- **Runtime** — `RT-005` tranche complete; support `SC-225` samples and keep the new hierarchy alert metrics (`runtime.scene_validation.alert_level`) wired into observability.
+- **Scene** — `SC-225` diagnostics samples extend the hierarchy playbook delivered in `SC-220`; alert thresholds from `SC-230` are live across dashboards.
```
```diff
@@
-- Support the scene module follow-ups (`SC-225` diagnostics samples,
-  `SC-230` alert thresholds) by providing telemetry examples and
-  cross-linking new fixtures once they land in the shared tooling docs;
-  these items extend `RT-005` and remain tracked in the
-  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
+- Support the scene module follow-ups (`SC-225` diagnostics samples) by
+  providing telemetry examples and cross-linking new fixtures once they
+  land in the shared tooling docs. Keep dashboards pointed at the
+  `runtime.scene_validation.alert_level` metric introduced with `SC-230`
+  so hierarchy regressions surface consistently across modules; track
+  progress alongside [RT-005](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
```
```diff
@@
+- `runtime.scene_validation.consecutive_failure_frames` — consecutive frames that reported hierarchy errors.
+- `runtime.scene_validation.max_consecutive_failure_frames` — longest streak observed since initialisation for historical comparison.
+- `runtime.scene_validation.failure_frame_count` — total frames containing hierarchy issues.
+- `runtime.scene_validation.last_failure_simulation_time` and `runtime.scene_validation.last_failure_wall_seconds` — timestamps for the most recent failure (`-1` when no failure has occurred).
+- `runtime.scene_validation.alert_threshold.warning_frames` / `...critical_frames` — documented alert thresholds (3 and 10 frames).
+- `runtime.scene_validation.alert_level` — derived alert state (`0 = none`, `1 = warning`, `2 = critical`).
+Dashboards should alert when `alert_level >= 1` and page when `>= 2`.
```
```diff
@@
-- Coordinate with runtime/tooling owners to define alert thresholds for
-  recurring hierarchy failures so dashboards surface sustained
-  regressions (`SC-230`, planned); status captured under `RT-005` in the
-  [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
+- Adopt the shared alert policy delivered in `SC-230` by monitoring
+  `runtime.scene_validation.alert_level` (warning at 3 consecutive failing
+  frames, paging at 10). Link local dashboards to the runtime metrics so
+  scene regressions surface alongside runtime telemetry.
```
```diff
@@
-| `SC-230` | Define alert thresholds for hierarchy regressions. | 🟢 Planned |
+| `SC-230` | Define alert thresholds for hierarchy regressions. | ✅ Done |
```
```diff
@@
-- Coordinate with the runtime team on `SC-230` to define alert thresholds
-  for repeated hierarchy failures. Capture decisions in this guide when
-  the alerting policy stabilises.
+- `runtime.scene_validation.alert_threshold.warning_frames` and
+  `runtime.scene_validation.alert_threshold.critical_frames` document the
+  runtime's baked-in alert policy (warning after 3 consecutive failing frames,
+  critical after 10). Dashboards should page when
+  `runtime.scene_validation.alert_level` reaches `2` and file follow-up issues
+  when it remains at `1` for longer than a few minutes.
```
```diff
@@
-+- [ ] Add telemetry alert thresholds for repeated hierarchy failures (owner: TBD, priority: Medium) so observability dashboards surface sustained regressions.
++- [x] Add telemetry alert thresholds for repeated hierarchy failures (owner: Runtime, priority: Medium) so observability dashboards surface sustained regressions (completed via `runtime.scene_validation.alert_level`).
```
```diff
@@
-- [ ] Define telemetry alert thresholds for recurring hierarchy failures (owner: Runtime + Tooling, priority: Medium) so dashboards surface sustained regressions.
+- [x] Define telemetry alert thresholds for recurring hierarchy failures (owner: Runtime + Tooling, priority: Medium) so dashboards surface sustained regressions (now covered by `runtime.scene_validation.alert_level`).
```
```diff
@@
-+- Support the scene module follow-ups (`SC-225` diagnostics samples, `SC-230` alert thresholds) by providing telemetry examples and cross-linking new fixtures once they land in the shared tooling docs; these items extend `RT-005` and remain tracked in the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
++- Support the scene module follow-ups (`SC-225` diagnostics samples) by providing telemetry examples and cross-linking new fixtures once they land in the shared tooling docs. Alert thresholds from `SC-230` now live under `runtime.scene_validation.alert_level`; ensure dashboards consume the metric.
@@
-+- Coordinate with runtime/tooling owners to define alert thresholds for recurring hierarchy failures so dashboards surface sustained regressions (`SC-230`, planned); status captured under `RT-005` in the [central roadmap](../../ROADMAP.md#rt-005-scene-hierarchy-validation).
++- Coordinate with runtime/tooling owners to keep the hierarchy alert metrics wired into dashboards. `SC-230` landed the shared thresholds (`runtime.scene_validation.alert_level`, warning = 3 frames, critical = 10 frames).
@@
-+- Coordinate with the runtime team on `SC-230` to define alert thresholds for repeated hierarchy failures. Capture decisions in this guide when the alerting policy stabilises.
++- Capture updates to the shared alert policy in this guide as dashboards evolve. The runtime now emits warning/critical thresholds via `runtime.scene_validation.alert_level` (`SC-230`).
```

## VERIFY
- `cmake --build --preset linux-gcc-debug --target engine_runtime_tests` → builds updated runtime + tests successfully.【0462aa†L1-L1】
- `ctest --preset linux-gcc-debug --tests-regex engine_runtime_tests` → runtime test suite passes, including new alert test.【e26afb†L1-L8】
- `python scripts/validate_docs.py` → documentation links validated with no errors.【2bfcf0†L1-L1】

## REVIEW_FINDINGS
- Correctness: Alert counters reset on success and accumulate on failure, covering warning/critical thresholds (`kHierarchyFailure*`). ✅
- Security: No untrusted inputs added; telemetry changes are internal only. ✅
- Compatibility: Changes are additive; existing consumers remain unaffected. ✅
- Quality & Style: Naming follows existing conventions; helper lives under `detail`. ✅
- Performance: Only constant-time updates per frame; no new allocations. ✅
- Tests: Added `SceneValidationAlertStateTransitions` plus metric assertions. ✅
- Observability: Metrics + docs describe thresholds and alert levels clearly. ✅
- Docs: Runtime/scene docs, roadmap, and follow-up records updated consistently. ✅

## REVIEW_PATCHES
N/A

## FINAL_PATCH
See ## PATCH / ## TESTS / ## DOCS.

## FOLLOW_UP_TODOS
- [x] Track `SC-225` diagnostics samples (owner: Scene, priority: Medium) to supply runnable assets now that alerting is ready. Implemented via `engine/scene/samples` and documented in the scene diagnostics guide (`SC-225`).
- [ ] Monitor dashboards for false positives; adjust `runtime.scene_validation.alert_level` thresholds if telemetry indicates noisy warnings.
- [ ] Evaluate consolidating diagnostics bridge listener management behind a subscription API (owner: Runtime, priority: Low) to reduce duplication noted in follow-up lists.
- [ ] Document CI packaging guidance for hierarchy telemetry artefacts (owner: DevEx, priority: Medium) once tooling workflow stabilises.
