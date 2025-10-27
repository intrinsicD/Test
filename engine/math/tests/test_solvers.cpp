#include <array>

#include <gtest/gtest.h>

#include "engine/math/math.hpp"
#include "engine/math/solvers.hpp"

using namespace engine::math;

namespace
{
    template <typename VectorLike, typename T>
    void ExpectVectorNear(const VectorLike& value, std::initializer_list<T> expected, T tolerance)
    {
        std::size_t index = 0;
        for (const auto& component : expected)
        {
            EXPECT_NEAR(value[index], component, tolerance);
            ++index;
        }
    }
}

TEST(MathSolvers, TrySolveLinearSystem3x3)
{
    mat3 A{
        3.0, 2.0, -1.0,
        2.0, -2.0, 4.0,
        -1.0, 0.5, -1.0
    };
    vec3 b{1.0F, -2.0F, 0.0F};

    auto solution = solvers::try_solve_linear_system(A, b);
    ASSERT_TRUE(solution.has_value());
    ExpectVectorNear(solution.value(), {1.0F, -2.0F, -2.0F}, 1e-5F);
}

TEST(MathSolvers, TrySolveLinearSystem4x4)
{
    const mat4 A{
        4.0F, 1.0F, 0.0F, 0.0F,
        1.0F, 3.0F, 1.0F, 0.0F,
        0.0F, 1.0F, 2.0F, 1.0F,
        0.0F, 0.0F, 1.0F, 2.0F
    };
    const vec4 expected{3.0F, 2.0F, 1.0F, 2.0F};
    const vec4 b = A * expected;

    auto solution = solvers::try_solve_linear_system(A, b);
    ASSERT_TRUE(solution.has_value());
    ExpectVectorNear(solution.value(), {expected[0], expected[1], expected[2], expected[3]}, 1e-5F);
}

TEST(MathSolvers, TrySolveLinearSystemSingular)
{
    mat3 A{
        1.0, 2.0, 3.0,
        2.0, 4.0, 6.0,
        1.0, -1.0, 0.0
    };
    vec3 b{1.0F, 2.0F, 3.0F};

    auto solution = solvers::try_solve_linear_system(A, b);
    EXPECT_FALSE(solution.has_value());
}

TEST(MathSolvers, SolveQuadraticTwoRealRoots)
{
    std::array < float, 2 > roots{};
    const std::size_t count = solvers::solve_quadratic(1.0F, -5.0F, 6.0F, roots);
    ASSERT_EQ(count, 2U);
    EXPECT_NEAR(roots[0], 2.0F, 1e-5F);
    EXPECT_NEAR(roots[1], 3.0F, 1e-5F);
}

TEST(MathSolvers, SolveQuadraticRepeatedRoot)
{
    std::array < double, 2 > roots{};
    const std::size_t count = solvers::solve_quadratic(1.0, -4.0, 4.0, roots);
    ASSERT_EQ(count, 1U);
    EXPECT_NEAR(roots[0], 2.0, 1e-9);
}

TEST(MathSolvers, SolveQuadraticLinearFallback)
{
    std::array < float, 2 > roots{};
    const std::size_t count = solvers::solve_quadratic(1e-8F, -3.0F, 2.0F, roots);
    ASSERT_EQ(count, 1U);
    EXPECT_NEAR(roots[0], 2.0F / 3.0F, 1e-5F);
}

TEST(MathSolvers, SolveQuadraticNoRealRoots)
{
    std::array < float, 2 > roots{};
    const std::size_t count = solvers::solve_quadratic(1.0F, 0.0F, 1.0F, roots);
    EXPECT_EQ(count, 0U);
}

TEST(MathSolvers, SolveCubicThreeRealRoots)
{
    std::array < double, 3 > roots{};
    const std::size_t count = solvers::solve_cubic(1.0, -6.0, 11.0, -6.0, roots);
    ASSERT_EQ(count, 3U);
    EXPECT_NEAR(roots[0], 1.0, 1e-9);
    EXPECT_NEAR(roots[1], 2.0, 1e-9);
    EXPECT_NEAR(roots[2], 3.0, 1e-9);
}

TEST(MathSolvers, SolveCubicSingleRealRoot)
{
    std::array < float, 3 > roots{};
    const std::size_t count = solvers::solve_cubic(1.0F, 1.0F, 1.0F, 1.0F, roots);
    ASSERT_EQ(count, 1U);
    EXPECT_NEAR(roots[0], -1.0F, 1e-5F);
}

TEST(MathSolvers, SolveCubicMultipleRoot)
{
    std::array < double, 3 > roots{};
    const std::size_t count = solvers::solve_cubic(1.0, 0.0, -3.0, 2.0, roots);
    ASSERT_EQ(count, 2U);
    EXPECT_NEAR(roots[0], -2.0, 1e-9);
    EXPECT_NEAR(roots[1], 1.0, 1e-9);
}

TEST(MathSolvers, SolveCubicQuadraticFallback)
{
    std::array < float, 3 > roots{};
    const std::size_t count = solvers::solve_cubic(1e-8F, 1.0F, -5.0F, 6.0F, roots);
    ASSERT_EQ(count, 2U);
    EXPECT_NEAR(roots[0], 2.0F, 1e-5F);
    EXPECT_NEAR(roots[1], 3.0F, 1e-5F);
}