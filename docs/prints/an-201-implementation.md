# AN-201 Implementation & Review Log

## PRIORITY_DECISION
Selected Task: AN-201 — Extend validation regression coverage
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| AN-201 | 5 | 4 | 3 | 3 | 4 | 5 | 24 |
| DC-004.3 | 3 | 3 | 4 | 4 | 2 | 4 | 20 |
| SC-208 | 3 | 3 | 2 | 3 | 3 | 4 | 18 |
Tie-break Rationale: N/A
Decision Rationale
- Task has the nearest delivery date (2025-03-07) and keeps RT-001 on schedule.
- Missing coverage risks regressions in clip validation and blend-tree controllers.
- Completing AN-201 unblocks AN-240 planning and reduces ambiguity for runtime skinning.
- Effort is moderate: primarily new tests plus doc updates, fitting current sprint capacity.
- Aligns directly with roadmap focus and central README next-task entry.

## DESIGN_BRIEF
Problem Statement
- Clip validation currently lacks regression coverage for several error codes, risking undetected regressions in IO/tooling flows.
- Controller APIs (`make_linear_controller`, `advance_controller`) have limited negative-path testing, reducing confidence in playback invariants.

Acceptance Criteria
- Add tests covering remaining `ClipValidationErrorCode` cases: empty clip name, invalid duration, missing tracks, missing joint names, invalid keyframe times, non-finite transforms, zero-length rotations, and duration shorter than last keyframe.
- Add controller-focused tests that verify duration expansion, keyframe ordering, clamping/wrapping semantics, and zero-duration behaviour.
- Update module documentation (README + roadmap) and central roadmap/summary to mark AN-201 complete and surface next task.

Interfaces & Data Flow
- Tests call `engine::animation::validate_clip`, `make_linear_controller`, and `advance_controller`; no API changes required.
- New helper functions inside tests will construct malformed clips to trigger specific error paths.

Invariants & Edge Cases
- Maintain deterministic ordering of error results; tests will search by error code instead of relying on insertion order.
- Ensure controller tests cover both looping and non-looping playback.
- Avoid mutating production code; tests must compile under existing module exports.

Compatibility & Migration
- Purely additive tests; no API or ABI changes.
- Documentation updates reflect new regression coverage.

Security & Performance Considerations
- Tests should run quickly (milliseconds) and avoid dynamic allocations beyond standard containers.
- Controller tests guard against infinite loops by using bounded step counts.

Test Strategy
- Extend `engine/animation/tests/test_clip_serialization.cpp` with additional validation test cases.
- Extend `engine/animation/tests/test_module.cpp` (or add a new test file if clarity requires) with controller behaviour tests.
- Run `ctest --preset linux-gcc-debug --tests-regex engine_animation`.
- If docs change, run `python scripts/validate_docs.py`.

## PATCH
No production code changes; implementation confined to regression tests and documentation.

## TESTS
```diff
diff --git a/engine/animation/tests/test_clip_serialization.cpp b/engine/animation/tests/test_clip_serialization.cpp
@@
+#include <vector>
@@
+namespace
+{
+    using engine::animation::ClipValidationError;
+    using engine::animation::ClipValidationErrorCode;
+
+    [[nodiscard]] bool contains_error(const std::vector<ClipValidationError>& errors,
+                                      ClipValidationErrorCode code,
+                                      const std::string& joint,
+                                      std::size_t track,
+                                      std::size_t keyframe)
+    {
+        const auto it = std::find_if(errors.begin(), errors.end(), [&](const ClipValidationError& error) {
+            return error.code == code && error.joint_name == joint && error.track_index == track
+                   && error.keyframe_index == keyframe;
+        });
+        return it != errors.end();
+    }
+} // namespace
+
+TEST(AnimationClipValidation, DetectsMissingClipMetadata)
+{
+    using namespace engine::animation;
+    AnimationClip clip;
+    clip.duration = -1.0;
+    const auto errors = validate_clip(clip);
+    ASSERT_FALSE(errors.empty());
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kClipNameEmpty,
+                               std::string{},
+                               std::numeric_limits<std::size_t>::max(),
+                               std::numeric_limits<std::size_t>::max()));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kClipDurationInvalid,
+                               std::string{},
+                               std::numeric_limits<std::size_t>::max(),
+                               std::numeric_limits<std::size_t>::max()));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kClipMissingTracks,
+                               std::string{},
+                               std::numeric_limits<std::size_t>::max(),
+                               std::numeric_limits<std::size_t>::max()));
+}
+
+TEST(AnimationClipValidation, DetectsTrackAndKeyframeIssues)
+{
+    using namespace engine::animation;
+    using engine::math::quat;
+    using engine::math::vec3;
+    AnimationClip clip;
+    clip.name = "malformed";
+    clip.duration = 1.0;
+    JointTrack missing_name;
+    missing_name.keyframes.push_back({0.0, JointPose{}});
+    clip.tracks.push_back(missing_name);
+    JointTrack malformed;
+    malformed.joint_name = "root";
+    malformed.keyframes.push_back({-0.5, JointPose{vec3{0.0F, 0.0F, 0.0F}, quat{0.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}});
+    malformed.keyframes.push_back({0.0,
+                                   JointPose{vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
+                                             quat{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F, 1.0F},
+                                             vec3{1.0F, std::numeric_limits<float>::quiet_NaN(), 1.0F}}});
+    malformed.keyframes.push_back({0.0, JointPose{vec3{0.0F, 0.0F, 0.0F}, quat{1.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}});
+    clip.tracks.push_back(malformed);
+    const auto errors = validate_clip(clip);
+    ASSERT_FALSE(errors.empty());
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kTrackMissingJointName,
+                               std::string{},
+                               0U,
+                               std::numeric_limits<std::size_t>::max()));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeTimeInvalid,
+                               "root",
+                               1U,
+                               0U));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeTimeNonIncreasing,
+                               "root",
+                               1U,
+                               2U));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeTranslationNonFinite,
+                               "root",
+                               1U,
+                               1U));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeScaleNonFinite,
+                               "root",
+                               1U,
+                               1U));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeRotationNonFinite,
+                               "root",
+                               1U,
+                               1U));
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kKeyframeRotationZeroLength,
+                               "root",
+                               1U,
+                               0U));
+}
+
+TEST(AnimationClipValidation, DetectsClipDurationShorterThanKeyframes)
+{
+    using namespace engine::animation;
+    AnimationClip clip;
+    clip.name = "duration_mismatch";
+    clip.duration = 0.25;
+    JointTrack track;
+    track.joint_name = "root";
+    track.keyframes.push_back({0.0, JointPose{}});
+    track.keyframes.push_back({0.5, JointPose{}});
+    clip.tracks.push_back(track);
+    const auto errors = validate_clip(clip);
+    ASSERT_FALSE(errors.empty());
+    EXPECT_TRUE(contains_error(errors,
+                               ClipValidationErrorCode::kClipDurationShorterThanLastKeyframe,
+                               std::string{},
+                               std::numeric_limits<std::size_t>::max(),
+                               std::numeric_limits<std::size_t>::max()));
+}

diff --git a/engine/animation/tests/test_module.cpp b/engine/animation/tests/test_module.cpp
@@
+TEST(AnimationModule, MakeLinearControllerExpandsDurationToLastKeyframe) {
+    using namespace engine::animation;
+    AnimationClip clip;
+    clip.name = "duration";
+    clip.duration = 0.1;
+    JointTrack track;
+    track.joint_name = "root";
+    track.keyframes.push_back({0.0, JointPose{}});
+    track.keyframes.push_back({1.25, JointPose{}});
+    clip.tracks.push_back(track);
+    const auto controller = make_linear_controller(std::move(clip));
+    ASSERT_FALSE(controller.clip.tracks.empty());
+    EXPECT_NEAR(controller.clip.duration, 1.25, 1e-9);
+    EXPECT_DOUBLE_EQ(controller.clip.tracks.front().keyframes.front().time, 0.0);
+    EXPECT_DOUBLE_EQ(controller.clip.tracks.front().keyframes.back().time, 1.25);
+}
+
+TEST(AnimationModule, MakeLinearControllerSortsAndDeduplicatesKeyframes) {
+    using namespace engine::animation;
+    AnimationClip clip;
+    clip.name = "sort";
+    clip.duration = 1.0;
+    JointTrack track;
+    track.joint_name = "root";
+    track.keyframes.push_back({0.5, JointPose{}});
+    track.keyframes.push_back({0.1, JointPose{}});
+    track.keyframes.push_back({0.1000000005, JointPose{}});
+    clip.tracks.push_back(track);
+    const auto controller = make_linear_controller(std::move(clip));
+    ASSERT_EQ(controller.clip.tracks.size(), 1U);
+    const auto& keyframes = controller.clip.tracks.front().keyframes;
+    ASSERT_EQ(keyframes.size(), 2U);
+    EXPECT_LT(keyframes.front().time, keyframes.back().time);
+    EXPECT_NEAR(keyframes.front().time, 0.1, 1e-9);
+    EXPECT_NEAR(keyframes.back().time, 0.5, 1e-9);
+}
+
+TEST(AnimationModule, AdvanceControllerClampsWhenNotLooping) {
+    using namespace engine::animation;
+    auto controller = make_linear_controller(make_default_clip());
+    controller.looping = false;
+    advance_controller(controller, 10.0);
+    EXPECT_NEAR(controller.playback_time, controller.clip.duration, 1e-9);
+    advance_controller(controller, -10.0);
+    EXPECT_NEAR(controller.playback_time, 0.0, 1e-9);
+}
+
+TEST(AnimationModule, AdvanceControllerWrapsWhenLooping) {
+    using namespace engine::animation;
+    auto controller = make_linear_controller(make_default_clip());
+    controller.looping = true;
+    advance_controller(controller, controller.clip.duration * 1.5);
+    EXPECT_NEAR(controller.playback_time, 0.5, 1e-9);
+    controller.playback_time = 0.25;
+    advance_controller(controller, -0.75);
+    EXPECT_NEAR(controller.playback_time, 0.5, 1e-9);
+}
+
+TEST(AnimationModule, AdvanceControllerSkipsWhenClipDurationIsZero) {
+    using namespace engine::animation;
+    AnimationController controller;
+    controller.clip.duration = 0.0;
+    controller.playback_time = 0.0;
+    controller.playback_speed = 1.0;
+    controller.looping = true;
+    advance_controller(controller, 1.0);
+    EXPECT_DOUBLE_EQ(controller.playback_time, 0.0);
+}
```

## DOCS
```diff
diff --git a/README.md b/README.md
@@
-| Animation | ✅ Stable | Deterministic clip sampling, validation, JSON import/export, blend-tree controllers, and structured error reporting consumed by the runtime pose system. | `AN-201`: extend regression coverage for validation failures to close out `RT-001`. |
+| Animation | ✅ Stable | Deterministic clip sampling, validation, JSON import/export, blend-tree controllers, and structured error reporting consumed by the runtime pose system. | `AN-230`: prototype GPU/parallel sampling benchmarks once compute queue extensions land. |

diff --git a/docs/ROADMAP.md b/docs/ROADMAP.md
@@
-**Animation** — `AN-201` regression coverage for clip validation (ties to `RT-001`), followed by `AN-230` GPU sampling benchmarks once compute queue extensions land.
+**Animation** — `AN-230` GPU/parallel sampling benchmarks once compute queue extensions land, followed by `AN-240` state-machine authoring spec work.

diff --git a/docs/modules/animation/README.md b/docs/modules/animation/README.md
@@
+- Regression suites exercise clip validation failure codes and controller playback invariants, guarding against regressions in authoring and runtime pipelines.
- Track `AN-201`, `AN-230`, `AN-240` in the central roadmap …
+- Track `AN-230` and `AN-240` in the central roadmap …
-| `AN-201` | … | 🔄 In Progress |
+| `AN-201` | … | ✅ Done |

diff --git a/docs/modules/animation/ROADMAP.md b/docs/modules/animation/ROADMAP.md
@@
-_Last Updated: 2025-02-19_
+ _Last Updated: 2025-02-21_
-| `AN-201` | … | 🔄 In Progress |
+| `AN-201` | … | ✅ Done |
+- 2025-02-21: Added regression coverage for clip validation failure codes and controller playback invariants (`engine/animation/tests/test_clip_serialization.cpp`, `engine/animation/tests/test_module.cpp`).

diff --git a/docs/tasks/T-0113-animation-runtime-skinning.md b/docs/tasks/T-0113-animation-runtime-skinning.md
@@
+- Regression coverage extended via `AnimationClipValidation.*` and `AnimationModule.*` tests to lock down validation and controller failure scenarios (AN-201).
```

## VERIFY
- `cmake --preset linux-gcc-debug` — configured successfully; GLFW disabled due to missing Xrandr headers (expected in container).【c3077d†L1-L18】
- `cmake --build --preset linux-gcc-debug --target engine_animation_tests` — compiled updated animation test suite (warnings unchanged from baseline).【2632af†L1-L1】【6fe818†L1-L10】【541308†L1-L2】
- `ctest --preset linux-gcc-debug --tests-regex engine_animation` — animation tests passed.【59af20†L1-L7】
- `python scripts/validate_docs.py` — documentation validation succeeded.【432270†L1-L2】

## REVIEW_FINDINGS
- Correctness: ✅ Tests cover all remaining `ClipValidationErrorCode` branches and controller invariants; no production logic changes to audit.
- Security: ✅ Changes confined to tests/docs with no impact on data handling.
- Compatibility: ✅ Public APIs untouched; documentation refreshed to guide next tasks.
- Quality & Style: ✅ Test helpers avoid duplication; documentation adopts consistent tone.
- Performance: ✅ Additional tests execute in milliseconds; no runtime impact.
- Tests: ✅ New cases exercised via `engine_animation_tests`; automation covers failure scenarios.
- Observability: ✅ Documentation notes regression coverage; telemetry follow-up tracked in TODOs.
- Docs: ✅ Root README, roadmap, and module/task files updated to reflect AN-201 completion.

## REVIEW_PATCHES
N/A — no additional reviewer patches required.

## FINAL_PATCH
Final patch equals the test and documentation diffs captured above; no further adjustments necessary post-review.

## FOLLOW_UP_TODOS
- [ ] Publish controller edge-case telemetry once diagnostics viewer lands (owner: TBD, priority: medium) — quantifies playback drift.
- [ ] Add continuous monitoring for animation validation errors in CI telemetry (owner: TBD, priority: low).
- [ ] Evaluate templating helpers for constructing malformed clips in multiple suites (owner: TBD, priority: low).
- [ ] Document blend-tree validation strategy in `docs/modules/animation/README.md` once planning begins (owner: TBD, priority: medium).
