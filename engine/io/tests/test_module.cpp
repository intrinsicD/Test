#include <gtest/gtest.h>

#include "engine/io/api.hpp"

TEST(IOModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::io::module_name(), "io");
    EXPECT_STREQ(engine_io_module_name(), "io");
}
