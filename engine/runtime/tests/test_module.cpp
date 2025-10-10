#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <string_view>

#include "engine/runtime/api.hpp"

TEST(RuntimeModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::runtime::module_name(), "runtime");
    EXPECT_STREQ(engine_runtime_module_name(), "runtime");
}

TEST(RuntimeModule, ExecutesSimulationPipeline) {
    engine::runtime::shutdown();
    engine::runtime::initialize();

    const auto frame = engine::runtime::tick(0.016);
    EXPECT_GE(frame.dispatch_report.execution_order.size(), 4U);
    EXPECT_EQ(frame.dispatch_report.execution_order.front(), "animation.evaluate");
    EXPECT_EQ(frame.dispatch_report.execution_order.back(), "geometry.finalize");

    EXPECT_FALSE(frame.pose.joints.empty());
    const auto* root = frame.pose.find("root");
    ASSERT_TRUE(root != nullptr);

    const auto& mesh = engine::runtime::current_mesh();
    EXPECT_GE(mesh.bounds.max[1], mesh.bounds.min[1]);
    EXPECT_FALSE(frame.body_positions.empty());
    EXPECT_FALSE(frame.scene_nodes.empty());
    const auto& root_node = frame.scene_nodes.front();
    EXPECT_EQ(root_node.name, "root");
    const float expected_root_height = frame.body_positions.front()[1] + root->translation[1];
    EXPECT_NEAR(root_node.transform.translation[1], expected_root_height, 1e-4F);
    EXPECT_EQ(engine_runtime_scene_node_count(), frame.scene_nodes.size());
    EXPECT_STREQ(engine_runtime_scene_node_name(0), "root");
    float scales[3]{};
    float rotations[4]{};
    float translations[3]{};
    engine_runtime_scene_node_transform(0, scales, rotations, translations);
    EXPECT_FLOAT_EQ(translations[1], root_node.transform.translation[1]);

    engine::runtime::shutdown();
}

TEST(RuntimeModule, EnumeratesAllEngineModules) {
    constexpr std::array expected{
        std::string_view{"animation"},
        std::string_view{"assets"},
        std::string_view{"compute"},
        std::string_view{"compute.cuda"},
        std::string_view{"core"},
        std::string_view{"geometry"},
        std::string_view{"io"},
        std::string_view{"physics"},
        std::string_view{"platform"},
        std::string_view{"rendering"},
        std::string_view{"scene"},
    };

    ASSERT_EQ(engine::runtime::module_count(), expected.size());
    EXPECT_EQ(engine_runtime_module_count(), expected.size());

    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(engine::runtime::module_name_at(index), expected[index]);
        EXPECT_STREQ(engine_runtime_module_at(index), expected[index].data());
    }

    EXPECT_TRUE(engine::runtime::module_name_at(expected.size()).empty());
    EXPECT_EQ(engine_runtime_module_at(expected.size()), nullptr);
}
