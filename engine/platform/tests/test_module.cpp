#include <gtest/gtest.h>

#include "engine/platform/api.hpp"

TEST(PlatformModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::platform::module_name(), "platform");
    EXPECT_STREQ(engine_platform_module_name(), "platform");
}
