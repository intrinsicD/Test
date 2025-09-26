#include <gtest/gtest.h>

#include "engine/physics/api.hpp"

TEST(PhysicsModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::physics::module_name(), "physics");
    EXPECT_STREQ(engine_physics_module_name(), "physics");
}
