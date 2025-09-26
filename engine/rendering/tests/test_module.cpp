#include <gtest/gtest.h>

#include "engine/rendering/api.hpp"

TEST(RenderingModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::rendering::module_name(), "rendering");
    EXPECT_STREQ(engine_rendering_module_name(), "rendering");
}
