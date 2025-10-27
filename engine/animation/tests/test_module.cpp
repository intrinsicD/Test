#include <gtest/gtest.h>

#include "engine/animation/api.hpp"

TEST(AnimationModule, ModuleNameMatchesNamespace)
{
    EXPECT_EQ(engine::animation::module_name(), "animation");
    EXPECT_STREQ(engine_animation_module_name(), "animation");
}

TEST(AnimationModule, ControllerEvaluatesInterpolatedPose)
{
    auto clip = engine::animation::make_default_clip();
    auto controller = engine::animation::make_linear_controller(std::move(clip));

    engine::animation::advance_controller(controller, 0.25);
    auto pose = engine::animation::evaluate_controller(controller);

    EXPECT_FALSE(pose.joints.empty());
    const auto* root = pose.find("root");
    EXPECT_TRUE(root != nullptr);
    EXPECT_NEAR(root->translation[1], 0.25F, 1e-4F);

    engine::animation::advance_controller(controller, 0.50);
    pose = engine::animation::evaluate_controller(controller);
    root = pose.find("root");
    EXPECT_TRUE(root != nullptr);
    EXPECT_NEAR(root->translation[1], 0.25F, 1e-4F);
}

TEST(AnimationModule, MakeLinearControllerExpandsDurationToLastKeyframe)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "duration";
    clip.duration = 0.1;

    JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back({0.0, JointPose{}});
    track.keyframes.push_back({1.25, JointPose{}});
    clip.tracks.push_back(track);

    const auto controller = make_linear_controller(std::move(clip));

    ASSERT_FALSE(controller.clip.tracks.empty());
    EXPECT_NEAR(controller.clip.duration, 1.25, 1e-9);
    EXPECT_DOUBLE_EQ(controller.clip.tracks.front().keyframes.front().time, 0.0);
    EXPECT_DOUBLE_EQ(controller.clip.tracks.front().keyframes.back().time, 1.25);
}

TEST(AnimationModule, MakeLinearControllerSortsAndDeduplicatesKeyframes)
{
    using namespace engine::animation;

    AnimationClip clip;
    clip.name = "sort";
    clip.duration = 1.0;

    JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back({0.5, JointPose{}});
    track.keyframes.push_back({0.1, JointPose{}});
    track.keyframes.push_back({0.1000000005, JointPose{}});
    clip.tracks.push_back(track);

    const auto controller = make_linear_controller(std::move(clip));

    ASSERT_EQ(controller.clip.tracks.size(), 1U);
    const auto& keyframes = controller.clip.tracks.front().keyframes;
    ASSERT_EQ(keyframes.size(), 2U);
    EXPECT_LT(keyframes.front().time, keyframes.back().time);
    EXPECT_NEAR(keyframes.front().time, 0.1, 1e-9);
    EXPECT_NEAR(keyframes.back().time, 0.5, 1e-9);
}

TEST(AnimationModule, AdvanceControllerClampsWhenNotLooping)
{
    using namespace engine::animation;

    auto controller = make_linear_controller(make_default_clip());
    controller.looping = false;

    advance_controller(controller, 10.0);
    EXPECT_NEAR(controller.playback_time, controller.clip.duration, 1e-9);

    advance_controller(controller, -10.0);
    EXPECT_NEAR(controller.playback_time, 0.0, 1e-9);
}

TEST(AnimationModule, AdvanceControllerWrapsWhenLooping)
{
    using namespace engine::animation;

    auto controller = make_linear_controller(make_default_clip());
    controller.looping = true;

    advance_controller(controller, controller.clip.duration * 1.5);
    EXPECT_NEAR(controller.playback_time, 0.5, 1e-9);

    controller.playback_time = 0.25;
    advance_controller(controller, -0.75);
    EXPECT_NEAR(controller.playback_time, 0.5, 1e-9);
}

TEST(AnimationModule, AdvanceControllerSkipsWhenClipDurationIsZero)
{
    using namespace engine::animation;

    AnimationController controller;
    controller.clip.duration = 0.0;
    controller.playback_time = 0.0;
    controller.playback_speed = 1.0;
    controller.looping = true;

    advance_controller(controller, 1.0);
    EXPECT_DOUBLE_EQ(controller.playback_time, 0.0);
}