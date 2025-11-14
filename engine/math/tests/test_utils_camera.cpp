#include <cmath>

#include <gtest/gtest.h>

#include "engine/math/matrix.hpp"
#include "engine/math/utils/utils_camera.hpp"

namespace
{
    constexpr float kTolerance = 1.0e-6F;
}

TEST(CameraUtils, PerspectiveUsesColumnMajorLayout)
{
    const float fov = engine::math::utils::radians(60.0F);
    const float aspect = 16.0F / 9.0F;
    const float near_plane = 0.1F;
    const float far_plane = 100.0F;

    const auto matrix = engine::math::utils::perspective(fov, aspect, near_plane, far_plane);

    const float f = 1.0F / std::tan(fov / 2.0F);
    const float expected_22 = (far_plane + near_plane) / (near_plane - far_plane);
    const float expected_23 = (2.0F * far_plane * near_plane) / (near_plane - far_plane);

    EXPECT_NEAR(matrix[0][0], f / aspect, kTolerance);
    EXPECT_NEAR(matrix[1][1], f, kTolerance);
    EXPECT_NEAR(matrix[2][2], expected_22, kTolerance);
    EXPECT_NEAR(matrix[2][3], expected_23, kTolerance);
    EXPECT_NEAR(matrix[3][2], -1.0F, kTolerance);
    EXPECT_NEAR(matrix[3][3], 0.0F, kTolerance);

    EXPECT_NEAR(matrix[3][0], 0.0F, kTolerance);
    EXPECT_NEAR(matrix[3][1], 0.0F, kTolerance);
    EXPECT_NEAR(matrix[0][3], 0.0F, kTolerance);
    EXPECT_NEAR(matrix[1][3], 0.0F, kTolerance);
}

TEST(CameraUtils, OrthographicUsesColumnMajorLayout)
{
    const float left = -5.0F;
    const float right = 7.0F;
    const float bottom = -3.0F;
    const float top = 9.0F;
    const float near_plane = 0.5F;
    const float far_plane = 150.0F;

    const auto matrix =
        engine::math::utils::orthographic(left, right, bottom, top, near_plane, far_plane);

    EXPECT_NEAR(matrix[0][0], 2.0F / (right - left), kTolerance);
    EXPECT_NEAR(matrix[1][1], 2.0F / (top - bottom), kTolerance);
    EXPECT_NEAR(matrix[2][2], -2.0F / (far_plane - near_plane), kTolerance);

    EXPECT_NEAR(matrix[0][3], -(right + left) / (right - left), kTolerance);
    EXPECT_NEAR(matrix[1][3], -(top + bottom) / (top - bottom), kTolerance);
    EXPECT_NEAR(matrix[2][3], -(far_plane + near_plane) / (far_plane - near_plane), kTolerance);
    EXPECT_NEAR(matrix[3][3], 1.0F, kTolerance);

    EXPECT_NEAR(matrix[3][0], 0.0F, kTolerance);
    EXPECT_NEAR(matrix[3][1], 0.0F, kTolerance);
    EXPECT_NEAR(matrix[3][2], 0.0F, kTolerance);
}
