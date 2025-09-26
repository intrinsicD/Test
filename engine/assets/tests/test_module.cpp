#include <gtest/gtest.h>

#include "engine/assets/api.hpp"

TEST(AssetsModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::assets::module_name(), "assets");
    EXPECT_STREQ(engine_assets_module_name(), "assets");
}
