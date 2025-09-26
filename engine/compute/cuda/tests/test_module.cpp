#include <gtest/gtest.h>

#include "engine/compute/cuda/api.hpp"

TEST(ComputeCudaModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::compute::cuda::module_name(), "compute.cuda");
    EXPECT_STREQ(engine_compute_cuda_module_name(), "compute.cuda");
}

TEST(ComputeCudaModule, DefaultDeviceAxisIsNormalized) {
    const auto axis = engine::compute::cuda::default_device_axis();
    EXPECT_FLOAT_EQ(axis[0], 0.0F);
    EXPECT_FLOAT_EQ(axis[1], 0.0F);
    EXPECT_FLOAT_EQ(axis[2], 1.0F);
}

TEST(ComputeCudaModule, DefaultDeviceTransformTranslatesBackwards) {
    const auto transform = engine::compute::cuda::default_device_transform();

    for (std::size_t diagonal = 0; diagonal < 4; ++diagonal) {
        EXPECT_FLOAT_EQ(transform[diagonal][diagonal], 1.0F);
    }

    EXPECT_FLOAT_EQ(transform[2][3], -1.0F);
    EXPECT_FLOAT_EQ(transform[0][3], 0.0F);
    EXPECT_FLOAT_EQ(transform[1][3], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][3], 1.0F);
}
