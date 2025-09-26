#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include "engine/runtime/api.hpp"

TEST(RuntimeModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::runtime::module_name(), "runtime");
    EXPECT_STREQ(engine_runtime_module_name(), "runtime");
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
