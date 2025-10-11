#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "engine/animation/api.hpp"

namespace
{
    [[nodiscard]] engine::animation::AnimationClip make_test_clip()
    {
        using namespace engine::animation;

        AnimationClip clip;
        clip.name = "test.clip";
        clip.duration = 1.0;

        JointTrack root_track;
        root_track.joint_name = "root";
        root_track.keyframes.push_back({0.0, JointPose{math::vec3{0.0F, 0.0F, 0.0F}, math::quat{1.0F, 0.0F, 0.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}});
        root_track.keyframes.push_back({1.0, JointPose{math::vec3{0.0F, 1.0F, 0.0F}, math::quat{0.0F, 0.0F, 1.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}});

        JointTrack arm_track;
        arm_track.joint_name = "arm";
        arm_track.keyframes.push_back({0.0, JointPose{math::vec3{1.0F, 0.0F, 0.0F}, math::quat{1.0F, 0.0F, 0.0F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}});
        arm_track.keyframes.push_back({1.0, JointPose{math::vec3{1.0F, 0.5F, 0.0F}, math::quat{0.70710677F, 0.0F, 0.70710677F, 0.0F}, math::vec3{1.0F, 1.0F, 1.0F}}});

        clip.tracks.push_back(std::move(root_track));
        clip.tracks.push_back(std::move(arm_track));

        return clip;
    }
} // namespace

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

    bool duplicate_track_detected = false;
    bool non_increasing_detected = false;
    for (const auto& error : errors)
    {
        if (error.message.find("Duplicate joint track") != std::string::npos)
        {
            duplicate_track_detected = true;
        }
        if (error.message.find("strictly increasing") != std::string::npos)
        {
            non_increasing_detected = true;
        }
    }

    EXPECT_TRUE(duplicate_track_detected);
    EXPECT_TRUE(non_increasing_detected);
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
