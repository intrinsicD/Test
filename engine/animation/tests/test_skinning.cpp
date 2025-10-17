#include <vector>

#include <gtest/gtest.h>

#include "engine/animation/deformation/linear_blend_skinning.hpp"
#include "engine/math/transform.hpp"

namespace engine::animation
{
    TEST(LinearBlendSkinning, BuildsGlobalAndSkinningTransforms)
    {
        RigBinding binding{};
        binding.joints.push_back(RigJoint{"root", RigBinding::kInvalidIndex,
                                          math::Transform<float>::Identity()});
        RigJoint child{};
        child.name = "child";
        child.parent = 0U;
        child.inverse_bind_pose.translation = math::vec3{0.0F, -2.0F, 0.0F};
        binding.joints.push_back(child);
        binding.resize_vertices(2);

        binding.vertices[0].clear();
        ASSERT_TRUE(binding.vertices[0].add_influence(0U, 1.0F));
        binding.vertices[0].normalize_weights();

        binding.vertices[1].clear();
        ASSERT_TRUE(binding.vertices[1].add_influence(1U, 1.0F));
        binding.vertices[1].normalize_weights();

        AnimationRigPose pose{};
        pose.joints.emplace_back("root",
                                 JointPose{math::vec3{0.0F, 0.0F, 0.0F},
                                           math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                           math::vec3{1.0F, 1.0F, 1.0F}});
        pose.joints.emplace_back("child",
                                 JointPose{math::vec3{0.0F, 2.0F, 0.0F},
                                           math::normalize(math::angle_axis(math::radians(90.0F),
                                                                            math::vec3{0.0F, 0.0F, 1.0F})),
                                           math::vec3{1.0F, 1.0F, 1.0F}});

        std::vector<math::Transform<float>> globals(binding.joints.size());
        std::vector<math::Transform<float>> skin(binding.joints.size());

        skinning::build_global_joint_transforms(binding, pose, globals, math::vec3{1.0F, 0.0F, 0.0F});
        skinning::build_skinning_transforms(binding, globals, skin);

        ASSERT_EQ(globals.size(), 2U);
        ASSERT_EQ(skin.size(), 2U);

        const auto root_point = math::transform_point(skin[0], math::vec3{0.0F, 0.0F, 0.0F});
        EXPECT_NEAR(root_point[0], 1.0F, 1.0e-5F);
        EXPECT_NEAR(root_point[1], 0.0F, 1.0e-5F);
        EXPECT_NEAR(root_point[2], 0.0F, 1.0e-5F);

        const auto child_point = math::transform_point(skin[1], math::vec3{0.0F, 3.0F, 0.0F});
        EXPECT_NEAR(child_point[0], 0.0F, 1.0e-4F);
        EXPECT_NEAR(child_point[1], 2.0F, 1.0e-4F);
        EXPECT_NEAR(child_point[2], 0.0F, 1.0e-4F);
    }
}

