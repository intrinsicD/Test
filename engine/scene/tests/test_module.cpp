#include <gtest/gtest.h>

#include "engine/scene/api.hpp"

TEST(SceneModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::scene::module_name(), "scene");
    EXPECT_STREQ(engine_scene_module_name(), "scene");
}
