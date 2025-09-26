#include <gtest/gtest.h>

#include "engine/animation/api.hpp"

TEST(AnimationModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::animation::module_name(), "animation");
    EXPECT_STREQ(engine_animation_module_name(), "animation");
}
