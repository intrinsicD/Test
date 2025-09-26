#include <gtest/gtest.h>

#include "engine/compute/api.hpp"

TEST(ComputeModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::compute::module_name(), "compute");
    EXPECT_STREQ(engine_compute_module_name(), "compute");
}

TEST(ComputeModule, IdentityTransformIsMatrixIdentity) {
    const auto transform = engine::compute::identity_transform();

    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            const float expected = (row == column) ? 1.0F : 0.0F;
            EXPECT_FLOAT_EQ(transform[row][column], expected);
        }
    }
}
