#include <gtest/gtest.h>

#include "engine/geometry/api.hpp"

TEST(GeometryModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::geometry::module_name(), "geometry");
    EXPECT_STREQ(engine_geometry_module_name(), "geometry");
}
