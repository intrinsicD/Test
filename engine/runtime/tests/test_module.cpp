#include <gtest/gtest.h>

#include <array>
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
    EXPECT_TRUE(root != nullptr);

    const auto& mesh = engine::runtime::current_mesh();
    EXPECT_GE(mesh.bounds.max[1], mesh.bounds.min[1]);
    EXPECT_FALSE(frame.body_positions.empty());

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
