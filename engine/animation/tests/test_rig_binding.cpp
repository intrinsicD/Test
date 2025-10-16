#include <algorithm>
#include <cstdint>

#include <gtest/gtest.h>

#include "engine/animation/rigging/rig_binding.hpp"

namespace engine::animation {

TEST(RigBinding, DefaultsToEmpty) {
    RigBinding binding{};
    EXPECT_TRUE(binding.empty());
    EXPECT_TRUE(binding.vertices.empty());
    EXPECT_TRUE(binding.joints.empty());
    EXPECT_TRUE(binding.normalized());
}

TEST(RigBinding, FindsJointByName) {
    RigBinding binding{};
    binding.joints.push_back(RigJoint{"root", RigBinding::kInvalidIndex});
    binding.joints.push_back(RigJoint{"spine", 0});

    const auto root_index = binding.find_joint_index("root");
    ASSERT_TRUE(root_index.has_value());
    EXPECT_EQ(*root_index, 0U);

    const auto missing = binding.find_joint_index("hand");
    EXPECT_FALSE(missing.has_value());
}

TEST(RigBinding, VertexBindingNormalizesWeights) {
    VertexBinding vertex{};
    ASSERT_TRUE(vertex.add_influence(0, 0.25F));
    ASSERT_TRUE(vertex.add_influence(1, 0.25F));
    ASSERT_TRUE(vertex.add_influence(2, 0.50F));

    vertex.normalize_weights();
    EXPECT_TRUE(vertex.weights_normalized());
}

TEST(RigBinding, RejectsInvalidInfluenceAssignments) {
    RigBinding binding{};
    binding.joints.push_back(RigJoint{"root", RigBinding::kInvalidIndex});
    binding.resize_vertices(1);

    const VertexInfluence influences[] = {
        VertexInfluence{1, 0.5F},
    };

    EXPECT_FALSE(binding.set_vertex_influences(0, influences));
}

TEST(RigBinding, AssignsInfluencesAndNormalizes) {
    RigBinding binding{};
    binding.joints.push_back(RigJoint{"root", RigBinding::kInvalidIndex});
    binding.joints.push_back(RigJoint{"spine", 0});
    binding.resize_vertices(2);

    const VertexInfluence influences[] = {
        VertexInfluence{0, 0.2F},
        VertexInfluence{1, 0.8F},
    };

    ASSERT_TRUE(binding.set_vertex_influences(1, influences));
    EXPECT_TRUE(binding.normalized());
    ASSERT_EQ(binding.vertices[1].influence_count, 2);
    EXPECT_NEAR(binding.vertices[1].influences[0].weight + binding.vertices[1].influences[1].weight, 1.0F, 1.0e-5F);
}

TEST(RigBinding, DropsSmallestWeightWhenFull) {
    VertexBinding vertex{};
    ASSERT_TRUE(vertex.add_influence(0, 0.1F));
    ASSERT_TRUE(vertex.add_influence(1, 0.2F));
    ASSERT_TRUE(vertex.add_influence(2, 0.3F));
    ASSERT_TRUE(vertex.add_influence(3, 0.4F));

    EXPECT_FALSE(vertex.add_influence(4, 0.05F));
    EXPECT_TRUE(vertex.add_influence(4, 0.6F));

    float min_weight = vertex.influences[0].weight;
    for (std::uint8_t i = 1; i < vertex.influence_count; ++i) {
        min_weight = std::min(min_weight, vertex.influences[i].weight);
    }
    EXPECT_GT(min_weight, 0.1F);
}

}  // namespace engine::animation

