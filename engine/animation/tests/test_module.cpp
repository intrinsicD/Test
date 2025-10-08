#include <gtest/gtest.h>

#include "engine/animation/api.hpp"

TEST(AnimationModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::animation::module_name(), "animation");
    EXPECT_STREQ(engine_animation_module_name(), "animation");
}

TEST(AnimationModule, ControllerEvaluatesInterpolatedPose) {
    auto clip = engine::animation::make_default_clip();
    auto controller = engine::animation::make_linear_controller(std::move(clip));

    engine::animation::advance_controller(controller, 0.25);
    auto pose = engine::animation::evaluate_controller(controller);

    ASSERT_FALSE(pose.joints.empty());
    const auto* root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR((*root).translation[1], 0.25F, 1e-4F);

    engine::animation::advance_controller(controller, 0.50);
    pose = engine::animation::evaluate_controller(controller);
    root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR((*root).translation[1], 0.0F, 1e-4F);
}
