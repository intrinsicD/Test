#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "engine/animation/api.hpp"

namespace
{
    [[nodiscard]] engine::animation::AnimationClip make_test_clip()
    {
        using namespace engine::animation;
        using namespace engine::math;

        AnimationClip clip;
        clip.name = "test.clip";
        clip.duration = 1.0;

        JointTrack root_track;
        root_track.joint_name = "root";
        root_track.keyframes.push_back({
            0.0, JointPose{vec3{0.0F, 0.0F, 0.0F}, quat{1.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
        });
        root_track.keyframes.push_back({
            1.0, JointPose{vec3{0.0F, 1.0F, 0.0F}, quat{0.0F, 0.0F, 1.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
        });

        JointTrack arm_track;
        arm_track.joint_name = "arm";
        arm_track.keyframes.push_back({
            0.0, JointPose{vec3{1.0F, 0.0F, 0.0F}, quat{1.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
        });
        arm_track.keyframes.push_back({
            1.0, JointPose{vec3{1.0F, 0.5F, 0.0F}, quat{0.70710677F, 0.0F, 0.70710677F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
        });

        clip.tracks.push_back(std::move(root_track));
        clip.tracks.push_back(std::move(arm_track));

        return clip;
    }
} // namespace

namespace
{
    using engine::animation::ClipValidationError;
    using engine::animation::ClipValidationErrorCode;

    [[nodiscard]] bool contains_error(const std::vector<ClipValidationError>& errors,
                                      ClipValidationErrorCode code,
                                      const std::string& joint,
                                      std::size_t track,
                                      std::size_t keyframe)
    {
        const auto it = std::find_if(errors.begin(), errors.end(), [&](const ClipValidationError& error)
        {
            return error.code == code && error.joint_name == joint && error.track_index == track
                && error.keyframe_index == keyframe;
        });
        return it != errors.end();
    }
} // namespace

TEST(AnimationClipValidation, DetectsMissingClipMetadata)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.duration = -1.0;

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kClipNameEmpty,
                               std::string{},
                               std::numeric_limits<std::size_t>::max(),
                               std::numeric_limits<std::size_t>::max()));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kClipDurationInvalid,
                               std::string{},
                               std::numeric_limits<std::size_t>::max(),
                               std::numeric_limits<std::size_t>::max()));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kClipMissingTracks,
                               std::string{},
                               std::numeric_limits<std::size_t>::max(),
                               std::numeric_limits<std::size_t>::max()));
}

TEST(AnimationClipValidation, DetectsTrackAndKeyframeIssues)
{
    using namespace engine::animation;
    using engine::math::quat;
    using engine::math::vec3;

    AnimationClip clip;
    clip.name = "malformed";
    clip.duration = 1.0;

    JointTrack missing_name;
    missing_name.keyframes.push_back({0.0, JointPose{}});
    clip.tracks.push_back(missing_name);

    JointTrack malformed;
    malformed.joint_name = "root";
    malformed.keyframes.push_back({
        -0.5, JointPose{vec3{0.0F, 0.0F, 0.0F}, quat{0.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
    });
    malformed.keyframes.push_back({
        0.0,
        JointPose{
            vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
            quat{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F, 1.0F},
            vec3{1.0F, std::numeric_limits<float>::quiet_NaN(), 1.0F}
        }
    });
    malformed.keyframes.push_back({
        0.0, JointPose{vec3{0.0F, 0.0F, 0.0F}, quat{1.0F, 0.0F, 0.0F, 0.0F}, vec3{1.0F, 1.0F, 1.0F}}
    });
    clip.tracks.push_back(malformed);

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kTrackMissingJointName,
                               std::string{},
                               0U,
                               std::numeric_limits<std::size_t>::max()));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeTimeInvalid,
                               "root",
                               1U,
                               0U));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeTimeNonIncreasing,
                               "root",
                               1U,
                               2U));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeTranslationNonFinite,
                               "root",
                               1U,
                               1U));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeScaleNonFinite,
                               "root",
                               1U,
                               1U));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeRotationNonFinite,
                               "root",
                               1U,
                               1U));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeRotationZeroLength,
                               "root",
                               1U,
                               0U));
}

TEST(AnimationClipValidation, DetectsClipDurationShorterThanKeyframes)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "duration_mismatch";
    clip.duration = 0.25;

    JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back({0.0, JointPose{}});
    track.keyframes.push_back({0.5, JointPose{}});
    clip.tracks.push_back(track);

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kClipDurationShorterThanLastKeyframe,
                               std::string{},
                               std::numeric_limits<std::size_t>::max(),
                               std::numeric_limits<std::size_t>::max()));
}

TEST(AnimationClipValidation, DetectsInvalidTracks)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "invalid";
    clip.duration = 1.0;

    JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back({0.0, JointPose{}});
    track.keyframes.push_back({0.0, JointPose{}});
    clip.tracks.push_back(track);
    clip.tracks.push_back(track);

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kTrackDuplicateJoint,
                               "root",
                               1U,
                               std::numeric_limits<std::size_t>::max()));
    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeTimeNonIncreasing,
                               "root",
                               0U,
                               1U));
}

TEST(AnimationClipValidation, DetectsEmptyTrackKeyframes)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "empty_track";
    clip.duration = 1.0;

    JointTrack track;
    track.joint_name = "root";
    clip.tracks.push_back(track);

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kTrackEmptyKeyframes,
                               "root",
                               0U,
                               std::numeric_limits<std::size_t>::max()));
}

TEST(AnimationClipValidation, DetectsUnorderedKeyframes)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "unordered_keys";
    clip.duration = 1.0;

    JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back({0.25, JointPose{}});
    track.keyframes.push_back({0.10, JointPose{}});
    clip.tracks.push_back(track);

    const auto errors = validate_clip(clip);
    ASSERT_FALSE(errors.empty());

    EXPECT_TRUE(contains_error(errors,
                               ClipValidationErrorCode::kKeyframeTimeNonIncreasing,
                               "root",
                               0U,
                               1U));
}

TEST(AnimationClipSerialization, RoundTripJson)
{
    using namespace engine::animation;

    const AnimationClip original = make_test_clip();

    std::stringstream buffer;
    write_clip_json(original, buffer, true);

    buffer.seekg(0);
    const AnimationClip restored = read_clip_json(buffer);

    ASSERT_EQ(restored.tracks.size(), original.tracks.size());
    EXPECT_EQ(restored.name, original.name);
    EXPECT_NEAR(restored.duration, original.duration, 1e-6);

    for (std::size_t track_index = 0; track_index < original.tracks.size(); ++track_index)
    {
        const auto& lhs = original.tracks[track_index];
        const auto& rhs = restored.tracks[track_index];
        EXPECT_EQ(lhs.joint_name, rhs.joint_name);
        ASSERT_EQ(lhs.keyframes.size(), rhs.keyframes.size());

        for (std::size_t keyframe_index = 0; keyframe_index < lhs.keyframes.size(); ++keyframe_index)
        {
            const auto& lhs_key = lhs.keyframes[keyframe_index];
            const auto& rhs_key = rhs.keyframes[keyframe_index];
            EXPECT_NEAR(lhs_key.time, rhs_key.time, 1e-6);
            for (int axis = 0; axis < 3; ++axis)
            {
                EXPECT_NEAR(lhs_key.pose.translation[axis], rhs_key.pose.translation[axis], 1e-6F);
                EXPECT_NEAR(lhs_key.pose.scale[axis], rhs_key.pose.scale[axis], 1e-6F);
            }
            for (int component = 0; component < 4; ++component)
            {
                EXPECT_NEAR(lhs_key.pose.rotation[component], rhs_key.pose.rotation[component], 1e-5F);
            }
        }
    }
}