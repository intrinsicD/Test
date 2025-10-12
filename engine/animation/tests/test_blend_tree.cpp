#include <gtest/gtest.h>

#include "engine/animation/api.hpp"

namespace
{
engine::animation::AnimationClip make_translation_clip(float y)
{
    engine::animation::AnimationClip clip;
    clip.name = "test.translation";
    clip.duration = 1.0;

    engine::animation::JointTrack track;
    track.joint_name = "root";
    track.keyframes.push_back(engine::animation::Keyframe{0.0, engine::animation::JointPose{engine::math::vec3{0.0F, y, 0.0F},
                                                                                          engine::math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                                                                          engine::math::vec3{1.0F, 1.0F, 1.0F}}});
    track.keyframes.push_back(engine::animation::Keyframe{1.0, engine::animation::JointPose{engine::math::vec3{0.0F, y, 0.0F},
                                                                                          engine::math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                                                                          engine::math::vec3{1.0F, 1.0F, 1.0F}}});

    engine::animation::sort_keyframes(track);
    clip.tracks.push_back(track);
    return clip;
}
} // namespace

TEST(AnimationBlendTree, EvaluatesLinearBlend)
{
    engine::animation::AnimationBlendTree tree;
    const auto base = engine::animation::add_clip_node(tree, make_translation_clip(0.0F));
    const auto target = engine::animation::add_clip_node(tree, make_translation_clip(1.0F));
    const auto blend = engine::animation::add_linear_blend_node(tree, base, target, 0.0F);
    engine::animation::set_blend_tree_root(tree, blend);

    ASSERT_TRUE(engine::animation::blend_tree_valid(tree));

    auto pose = engine::animation::evaluate_blend_tree(tree);
    const auto* root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.0F, 1e-4F);

    engine::animation::set_linear_blend_weight(tree, blend, 0.5F);
    pose = engine::animation::evaluate_blend_tree(tree);
    root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.5F, 1e-4F);

    engine::animation::advance_blend_tree(tree, 0.25);
    pose = engine::animation::evaluate_blend_tree(tree);
    root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.5F, 1e-4F);
}

TEST(AnimationBlendTree, DetectsInvalidTopology)
{
    engine::animation::AnimationBlendTree tree;
    const auto orphan = engine::animation::add_linear_blend_node(tree, 5U, 6U, 0.25F);
    (void)orphan;
    EXPECT_FALSE(engine::animation::blend_tree_valid(tree));
    EXPECT_TRUE(engine::animation::evaluate_blend_tree(tree).joints.empty());
}

TEST(AnimationBlendTree, BindsBlendWeightToFloatParameter)
{
    engine::animation::AnimationBlendTree tree;
    const auto base = engine::animation::add_clip_node(tree, make_translation_clip(0.0F));
    const auto target = engine::animation::add_clip_node(tree, make_translation_clip(1.0F));
    const auto blend = engine::animation::add_linear_blend_node(tree, base, target, 0.25F);
    engine::animation::set_blend_tree_root(tree, blend);

    const auto parameter = engine::animation::add_float_parameter(tree, "weight", 0.75F);
    engine::animation::bind_linear_blend_weight(tree, blend, parameter);

    ASSERT_TRUE(engine::animation::blend_tree_valid(tree));

    auto pose = engine::animation::evaluate_blend_tree(tree);
    const auto* root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.75F, 1e-4F);

    EXPECT_TRUE(engine::animation::set_float_parameter(tree, parameter, 0.1F));
    pose = engine::animation::evaluate_blend_tree(tree);
    root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.1F, 1e-4F);

    EXPECT_TRUE(engine::animation::set_float_parameter(tree, "weight", 0.9F));
    pose = engine::animation::evaluate_blend_tree(tree);
    root = pose.find("root");
    ASSERT_NE(root, nullptr);
    EXPECT_NEAR(root->translation[1], 0.9F, 1e-4F);
}

TEST(AnimationBlendTree, ManagesBoolAndEventParameters)
{
    engine::animation::AnimationBlendTree tree;
    const auto bool_index = engine::animation::add_bool_parameter(tree, "enabled", true);
    const auto event_index = engine::animation::add_event_parameter(tree, "trigger");

    ASSERT_LT(bool_index, tree.parameters.size());
    ASSERT_LT(event_index, tree.parameters.size());

    EXPECT_TRUE(tree.parameters[bool_index].bool_value);
    EXPECT_TRUE(engine::animation::set_bool_parameter(tree, "enabled", false));
    EXPECT_FALSE(tree.parameters[bool_index].bool_value);

    EXPECT_FALSE(engine::animation::consume_event_parameter(tree, event_index));
    EXPECT_TRUE(engine::animation::trigger_event_parameter(tree, "trigger"));
    EXPECT_TRUE(tree.parameters[event_index].event_value);
    EXPECT_TRUE(engine::animation::consume_event_parameter(tree, "trigger"));
    EXPECT_FALSE(tree.parameters[event_index].event_value);
    EXPECT_FALSE(engine::animation::consume_event_parameter(tree, event_index));
}
