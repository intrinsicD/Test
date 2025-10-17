#include <vector>

#include <gtest/gtest.h>

#include "engine/animation/deformation/linear_blend_skinning.hpp"
#include "engine/geometry/deform/linear_blend_skinning.hpp"

namespace engine::geometry
{
    TEST(LinearBlendSkinning, AppliesSkinningToSurfaceMesh)
    {
        SurfaceMesh mesh{};
        mesh.rest_positions = {
            math::vec3{0.0F, 0.0F, 0.0F},
            math::vec3{0.0F, 2.0F, 0.0F},
            math::vec3{0.0F, 3.0F, 0.0F},
        };
        mesh.positions = mesh.rest_positions;
        mesh.indices = {0U, 1U, 2U};

        animation::RigBinding binding{};
        animation::RigJoint root{};
        root.name = "root";
        root.parent = animation::RigBinding::kInvalidIndex;
        root.inverse_bind_pose = math::Transform<float>::Identity();
        binding.joints.push_back(root);

        animation::RigJoint child{};
        child.name = "child";
        child.parent = 0U;
        child.inverse_bind_pose.translation = math::vec3{0.0F, -2.0F, 0.0F};
        binding.joints.push_back(child);

        binding.resize_vertices(mesh.rest_positions.size());
        binding.vertices[0].clear();
        ASSERT_TRUE(binding.vertices[0].add_influence(0U, 1.0F));
        binding.vertices[0].normalize_weights();

        binding.vertices[1].clear();
        ASSERT_TRUE(binding.vertices[1].add_influence(0U, 0.5F));
        ASSERT_TRUE(binding.vertices[1].add_influence(1U, 0.5F));
        binding.vertices[1].normalize_weights();

        binding.vertices[2].clear();
        ASSERT_TRUE(binding.vertices[2].add_influence(1U, 1.0F));
        binding.vertices[2].normalize_weights();

        animation::AnimationRigPose pose{};
        pose.joints.emplace_back("root",
                                 animation::JointPose{math::vec3{0.0F, 0.0F, 0.0F},
                                                       math::quat{1.0F, 0.0F, 0.0F, 0.0F},
                                                       math::vec3{1.0F, 1.0F, 1.0F}});
        pose.joints.emplace_back("child",
                                 animation::JointPose{math::vec3{0.0F, 2.0F, 0.0F},
                                                       math::normalize(math::angle_axis(math::radians(90.0F),
                                                                                        math::vec3{0.0F, 0.0F, 1.0F})),
                                                       math::vec3{1.0F, 1.0F, 1.0F}});

        std::vector<math::Transform<float>> globals(binding.joints.size());
        std::vector<math::Transform<float>> skin(binding.joints.size());
        animation::skinning::build_global_joint_transforms(binding, pose, globals);
        animation::skinning::build_skinning_transforms(binding, globals, skin);

        deform::apply_linear_blend_skinning(binding, skin, mesh);

        ASSERT_EQ(mesh.positions.size(), 3U);
        EXPECT_NEAR(mesh.positions[0][0], 0.0F, 1.0e-5F);
        EXPECT_NEAR(mesh.positions[0][1], 0.0F, 1.0e-5F);
        EXPECT_NEAR(mesh.positions[2][0], -1.0F, 1.0e-3F);
        EXPECT_NEAR(mesh.positions[2][1], 2.0F, 1.0e-3F);
    }
}

