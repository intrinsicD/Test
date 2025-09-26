#include <gtest/gtest.h>

#include "engine/core/api.hpp"

TEST(CoreModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::core::module_name(), "core");
    EXPECT_STREQ(engine_core_module_name(), "core");
}
