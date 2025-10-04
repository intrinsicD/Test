#include <array>
#include <cmath>
#include <type_traits>

#include <gtest/gtest.h>

#include "engine/math/common.hpp"
#include "engine/math/math.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/quaternion.hpp"

using namespace engine::math;

namespace {

template <typename T, std::size_t N>
void ExpectVectorEqual(const Vector<T, N>& value, const std::array<T, N>& expected) {
    for (std::size_t i = 0; i < N; ++i) {
        EXPECT_EQ(value[i], expected[i]);
    }
}

template <typename T>
void ExpectQuaternionEqual(const quaternion<T>& value, const std::array<T, 4>& expected) {
    EXPECT_EQ(value.w, expected[0]);
    EXPECT_EQ(value.x, expected[1]);
    EXPECT_EQ(value.y, expected[2]);
    EXPECT_EQ(value.z, expected[3]);
}

}  // namespace

TEST(MathCommon, ZeroAndOneHelpers) {
    EXPECT_EQ(detail::zero<int>(), 0);
    EXPECT_EQ(detail::zero<double>(), 0.0);

    EXPECT_EQ(detail::one<int>(), 1);
    EXPECT_EQ(detail::one<double>(), 1.0);
}

TEST(Vector, DefaultConstructedIsZeroInitialized) {
    Vector<int, 3> value;
    ExpectVectorEqual(value, {0, 0, 0});
}

TEST(Vector, ScalarConstructorFillsAllElements) {
    Vector<float, 4> value(2.5F);
    ExpectVectorEqual(value, {2.5F, 2.5F, 2.5F, 2.5F});
}

TEST(Vector, VariadicConstructorAssignsElements) {
    const Vector<double, 3> value(1.0, 2.0, 3.0);
    ExpectVectorEqual(value, {1.0, 2.0, 3.0});
}

TEST(Vector, ElementAccessSupportsConstAndNonConst) {
    Vector<int, 2> value(1);
    value[0] = 5;
    value[1] = 7;

    const Vector<int, 2>& const_ref = value;
    EXPECT_EQ(const_ref[0], 5);
    EXPECT_EQ(const_ref[1], 7);
}

TEST(Vector, ArithmeticOperators) {
    const vec3 lhs{1.0F, 2.0F, 3.0F};
    const vec3 rhs{4.0F, 5.0F, 6.0F};

    ExpectVectorEqual(lhs + rhs, {5.0F, 7.0F, 9.0F});
    ExpectVectorEqual(rhs - lhs, {3.0F, 3.0F, 3.0F});

    ExpectVectorEqual(lhs * 2.0F, {2.0F, 4.0F, 6.0F});
    ExpectVectorEqual(2.0F * lhs, {2.0F, 4.0F, 6.0F});

    ExpectVectorEqual(rhs / 2.0F, {2.0F, 2.5F, 3.0F});
}

TEST(Vector, CompoundAssignmentOperators) {
    vec3 value{1.0F, 2.0F, 3.0F};
    const vec3 rhs{0.5F, 1.0F, 1.5F};

    value += rhs;
    ExpectVectorEqual(value, {1.5F, 3.0F, 4.5F});

    value -= rhs;
    ExpectVectorEqual(value, {1.0F, 2.0F, 3.0F});

    value *= 2.0F;
    ExpectVectorEqual(value, {2.0F, 4.0F, 6.0F});

    value /= 2.0F;
    ExpectVectorEqual(value, {1.0F, 2.0F, 3.0F});
}

TEST(Vector, EqualityComparison) {
    const vec3 lhs{1.0F, 2.0F, 3.0F};
    const vec3 rhs{1.0F, 2.0F, 3.0F};
    const vec3 different{1.0F, 2.5F, 3.0F};

    EXPECT_TRUE(lhs == rhs);
    EXPECT_TRUE(!(lhs != rhs));
    EXPECT_TRUE(lhs != different);
}

TEST(Vector, DotLengthAndNormalize) {
    const vec3 value{3.0F, 4.0F, 0.0F};
    EXPECT_FLOAT_EQ(dot(value, value), 25.0F);
    EXPECT_FLOAT_EQ(length_squared(value), 25.0F);
    EXPECT_FLOAT_EQ(length(value), 5.0F);

    const vec3 normalized = normalize(value);
    ExpectVectorEqual(normalized, {0.6F, 0.8F, 0.0F});

    const vec3 zero{};
    const auto normalized_zero = normalize(zero);
    EXPECT_TRUE(normalized_zero == zero);
}

TEST(Vector, CrossProduct) {
    const vec3 lhs{1.0F, 0.0F, 0.0F};
    const vec3 rhs{0.0F, 1.0F, 0.0F};

    const vec3 result = cross(lhs, rhs);
    ExpectVectorEqual(result, {0.0F, 0.0F, 1.0F});
}

TEST(Vector, TypeAliasesProvideExpectedDimensions) {
    static_assert(std::is_same_v<vec2::value_type, float>);
    static_assert(std::is_same_v<dvec4::value_type, double>);
    static_assert(std::is_same_v<ivec3::value_type, int>);

    vec4 value(1.0F);
    ExpectVectorEqual(value, {1.0F, 1.0F, 1.0F, 1.0F});
}

TEST(Matrix, DefaultConstructedIsZeroInitialized) {
    Matrix<int, 2, 3> value;
    for (std::size_t row = 0; row < 2; ++row) {
        ExpectVectorEqual(value[row], {0, 0, 0});
    }
}

TEST(Matrix, VariadicConstructorFillsRows) {
    const Matrix<float, 2, 2> value(1.0F, 2.0F, 3.0F, 4.0F);
    ExpectVectorEqual(value[0], {1.0F, 2.0F});
    ExpectVectorEqual(value[1], {3.0F, 4.0F});
}

TEST(Matrix, RowAccessSupportsConstAndNonConst) {
    Matrix<int, 2, 2> value;
    value[0][0] = 1;
    value[0][1] = 2;
    value[1][0] = 3;
    value[1][1] = 4;

    const Matrix<int, 2, 2>& const_ref = value;
    ExpectVectorEqual(const_ref[0], {1, 2});
    ExpectVectorEqual(const_ref[1], {3, 4});
}

TEST(Matrix, ArithmeticOperators) {
    const Matrix<float, 2, 2> lhs(1.0F, 2.0F, 3.0F, 4.0F);
    const Matrix<float, 2, 2> rhs(0.5F, 0.5F, 0.5F, 0.5F);

    ExpectVectorEqual((lhs + rhs)[0], {1.5F, 2.5F});
    ExpectVectorEqual((lhs + rhs)[1], {3.5F, 4.5F});

    ExpectVectorEqual((lhs - rhs)[0], {0.5F, 1.5F});
    ExpectVectorEqual((lhs - rhs)[1], {2.5F, 3.5F});

    ExpectVectorEqual((lhs * 2.0F)[0], {2.0F, 4.0F});
    ExpectVectorEqual((2.0F * lhs)[1], {6.0F, 8.0F});
}

TEST(Matrix, CompoundAssignmentOperators) {
    Matrix<double, 2, 2> value(1.0, 2.0, 3.0, 4.0);
    const Matrix<double, 2, 2> rhs(0.5, 0.5, 0.5, 0.5);

    value += rhs;
    ExpectVectorEqual(value[0], {1.5, 2.5});
    ExpectVectorEqual(value[1], {3.5, 4.5});

    value -= rhs;
    ExpectVectorEqual(value[0], {1.0, 2.0});
    ExpectVectorEqual(value[1], {3.0, 4.0});

    value *= 2.0;
    ExpectVectorEqual(value[0], {2.0, 4.0});
    ExpectVectorEqual(value[1], {6.0, 8.0});
}

TEST(Matrix, MatrixVectorMultiplication) {
    const Matrix<float, 3, 3> mat(
        1.0F, 2.0F, 3.0F,
        4.0F, 5.0F, 6.0F,
        7.0F, 8.0F, 9.0F);
    const vec3 vec{1.0F, 2.0F, 3.0F};

    const vec3 result = mat * vec;
    ExpectVectorEqual(result, {14.0F, 32.0F, 50.0F});
}

TEST(Matrix, MatrixMatrixMultiplication) {
    const Matrix<int, 2, 3> lhs(1, 2, 3, 4, 5, 6);
    const Matrix<int, 3, 2> rhs(7, 8, 9, 10, 11, 12);

    const Matrix<int, 2, 2> result = lhs * rhs;
    ExpectVectorEqual(result[0], {58, 64});
    ExpectVectorEqual(result[1], {139, 154});
}

TEST(Quaternion, DefaultConstructedIsZeroInitialized) {
    quaternion<int> value;
    ExpectQuaternionEqual(value, {0, 0, 0, 0});
}

TEST(Quaternion, ComponentConstructorAssignsValues) {
    const quaternion<float> value(1.0F, 2.0F, 3.0F, 4.0F);
    ExpectQuaternionEqual(value, {1.0F, 2.0F, 3.0F, 4.0F});
}

TEST(Quaternion, ArithmeticOperators) {
    const quaternion<float> lhs(1.0F, 2.0F, 3.0F, 4.0F);
    const quaternion<float> rhs(0.5F, 1.0F, -1.0F, 2.0F);

    ExpectQuaternionEqual(lhs + rhs, {1.5F, 3.0F, 2.0F, 6.0F});
    ExpectQuaternionEqual(lhs - rhs, {0.5F, 1.0F, 4.0F, 2.0F});
    ExpectQuaternionEqual(lhs * 2.0F, {2.0F, 4.0F, 6.0F, 8.0F});
    ExpectQuaternionEqual(2.0F * lhs, {2.0F, 4.0F, 6.0F, 8.0F});
    ExpectQuaternionEqual(lhs / 2.0F, {0.5F, 1.0F, 1.5F, 2.0F});
}

TEST(Quaternion, HamiltonProduct) {
    const quaternion<float> identity(1.0F, 0.0F, 0.0F, 0.0F);
    const quaternion<float> value(0.0F, 1.0F, 0.0F, 0.0F);
    const quaternion<float> other(0.0F, 0.0F, 1.0F, 0.0F);

    EXPECT_TRUE(identity * value == value);
    const quaternion<float> result = value * other;
    ExpectQuaternionEqual(result, {-0.0F, 0.0F, 0.0F, 1.0F});
}

TEST(Quaternion, ConjugateLengthNormalizeAndInverse) {
    const quaternion<double> value(1.0, 2.0, 3.0, 4.0);

    const quaternion<double> conjugated = conjugate(value);
    ExpectQuaternionEqual(conjugated, {1.0, -2.0, -3.0, -4.0});

    const double tolerance = 1e-12;
    EXPECT_TRUE(std::abs(length_squared(value) - 30.0) <= tolerance);
    EXPECT_TRUE(std::abs(length(value) - std::sqrt(30.0)) <= tolerance);

    const quaternion<double> normalized = normalize(value);
    const double inv_len = 1.0 / std::sqrt(30.0);
    EXPECT_TRUE(std::abs(normalized.w - 1.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.x - 2.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.y - 3.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.z - 4.0 * inv_len) <= tolerance);

    const quaternion<double> inverse_value = inverse(value);
    const quaternion<double> identity = value * inverse_value;
    EXPECT_TRUE(std::abs(identity.w - 1.0) <= tolerance);
    EXPECT_TRUE(std::abs(identity.x) <= tolerance);
    EXPECT_TRUE(std::abs(identity.y) <= tolerance);
    EXPECT_TRUE(std::abs(identity.z) <= tolerance);
}

//TODO : Add tests for from_angle_axis, to_rotation_matrix, from_rotation_matrix, cast

TEST(Matrix, Transpose) {
    const Matrix<int, 2, 3> value(1, 2, 3, 4, 5, 6);
    const Matrix<int, 3, 2> transposed = transpose(value);

    ExpectVectorEqual(transposed[0], {1, 4});
    ExpectVectorEqual(transposed[1], {2, 5});
    ExpectVectorEqual(transposed[2], {3, 6});
}

TEST(Matrix, IdentityMatrixHasOnesOnDiagonal) {
    const auto id = identity_matrix<double, 3>();
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            const double expected = (r == c) ? detail::one<double>() : detail::zero<double>();
            EXPECT_EQ(id[r][c], expected);
        }
    }
}

TEST(Matrix, TranslationProducesAffineMatrix) {
    const vec3 offset{1.0F, 2.0F, 3.0F};
    const Matrix<float, 4, 4> transform = translation(offset);

    for (std::size_t i = 0; i < 3; ++i) {
        EXPECT_FLOAT_EQ(transform[i][i], 1.0F);
        EXPECT_FLOAT_EQ(transform[i][3], offset[i]);
    }
    EXPECT_FLOAT_EQ(transform[3][0], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][1], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][2], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][3], 1.0F);
}

TEST(Matrix, ScaleSetsDiagonalAndLeavesTranslationZero) {
    const vec3 factors{2.0F, 3.0F, 4.0F};
    const Matrix<float, 4, 4> transform = scale(factors);

    EXPECT_FLOAT_EQ(transform[0][0], 2.0F);
    EXPECT_FLOAT_EQ(transform[1][1], 3.0F);
    EXPECT_FLOAT_EQ(transform[2][2], 4.0F);
    EXPECT_FLOAT_EQ(transform[3][3], 1.0F);

    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            if (r != c) {
                EXPECT_FLOAT_EQ(transform[r][c], 0.0F);
            }
        }
    }
}

TEST(Matrix, TypeAliasesCompile) {
    mat4 float_mat = identity_matrix<float, 4>();
    dmat3 double_mat = identity_matrix<double, 3>();

    EXPECT_FLOAT_EQ(float_mat[0][0], 1.0F);
    EXPECT_EQ(double_mat[0][0], 1.0);
}

