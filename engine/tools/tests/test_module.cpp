#include "engine/tools/api.hpp"

#include <gtest/gtest.h>

TEST(ToolsModule, ModuleName)
{
    auto name = engine::tools::module_name();
    EXPECT_EQ(name, "tools");
}

TEST(ToolsModule, CAPIModuleName)
{
    auto name = engine_tools_module_name();
    EXPECT_STREQ(name, "tools");
}