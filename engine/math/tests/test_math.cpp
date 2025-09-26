#include <gtest/gtest.h>

#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

using namespace engine::math;

TEST(MathModule, IdentityMatrixIsDiagonal) {
    const auto identity = identity_matrix<float, 4>();

    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            const float expected = (row == column) ? detail::one<float>() : detail::zero<float>();
            EXPECT_FLOAT_EQ(identity[row][column], expected);
        }
    }
}

TEST(MathModule, VectorArithmeticBehavesAsExpected) {
    const vec3 lhs{1.0F, 2.0F, 3.0F};
    const vec3 rhs{4.0F, 5.0F, 6.0F};

    const auto sum = lhs + rhs;
    EXPECT_EQ(sum[0], 5.0F);
    EXPECT_EQ(sum[1], 7.0F);
    EXPECT_EQ(sum[2], 9.0F);

    const auto difference = rhs - lhs;
    EXPECT_EQ(difference[0], 3.0F);
    EXPECT_EQ(difference[1], 3.0F);
    EXPECT_EQ(difference[2], 3.0F);

    const auto scaled = lhs * 2.0F;
    EXPECT_EQ(scaled[0], 2.0F);
    EXPECT_EQ(scaled[1], 4.0F);
    EXPECT_EQ(scaled[2], 6.0F);

    EXPECT_FLOAT_EQ(dot(lhs, rhs), 32.0F);
    EXPECT_FLOAT_EQ(length(normalize(lhs)), 1.0F);
}
