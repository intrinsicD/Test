#include <array>
#include <cmath>
#include <type_traits>
#include <engine/math/utils_rotation.hpp>

#include <gtest/gtest.h>

#include "engine/math/common.hpp"
#include "engine/math/math.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/quaternion.hpp"

using namespace engine::math;

namespace
{
    template <typename VectorLike, typename T>
    void ExpectVectorEqual(VectorLike value, std::initializer_list<T> expected)
    {
        std::size_t index = 0;
        for (const auto& component : expected)
        {
            EXPECT_EQ(value[index], component);
            ++index;
        }
    }

    template <typename T>
    void ExpectQuaternionEqual(const Quaternion<T>& value, const std::array<T, 4>& expected)
    {
        EXPECT_EQ(value.w, expected[0]);
        EXPECT_EQ(value.x, expected[1]);
        EXPECT_EQ(value.y, expected[2]);
        EXPECT_EQ(value.z, expected[3]);
    }

    template <typename T>
    void ExpectNear(T value, T expected, T tolerance)
    {
        EXPECT_TRUE(std::abs(value - expected) <= tolerance);
    }
} // namespace

TEST(MathCommon, ZeroAndOneHelpers)
{
    EXPECT_EQ(detail::zero<int>(), 0);
    EXPECT_EQ(detail::zero<double>(), 0.0);

    EXPECT_EQ(detail::one<int>(), 1);
    EXPECT_EQ(detail::one<double>(), 1.0);
}

TEST(Vector, DefaultConstructedIsZeroInitialized)
{
    Vector<int, 3> value;
    ExpectVectorEqual(value, {0, 0, 0});
}

TEST(Vector, ScalarConstructorFillsAllElements)
{
    Vector<float, 4> value(2.5F);
    ExpectVectorEqual(value, {2.5F, 2.5F, 2.5F, 2.5F});
}

TEST(Vector, VariadicConstructorAssignsElements)
{
    const Vector<double, 3> value(1.0, 2.0, 3.0);
    ExpectVectorEqual(value, {1.0, 2.0, 3.0});
}

TEST(Vector, ElementAccessSupportsConstAndNonConst)
{
    Vector<int, 2> value(1);
    value[0] = 5;
    value[1] = 7;

    const Vector<int, 2>& const_ref = value;
    EXPECT_EQ(const_ref[0], 5);
    EXPECT_EQ(const_ref[1], 7);
}

TEST(Vector, ArithmeticOperators)
{
    const vec3 lhs{1.0F, 2.0F, 3.0F};
    const vec3 rhs{4.0F, 5.0F, 6.0F};

    ExpectVectorEqual(lhs + rhs, {5.0F, 7.0F, 9.0F});
    ExpectVectorEqual(rhs - lhs, {3.0F, 3.0F, 3.0F});

    ExpectVectorEqual(lhs * 2.0F, {2.0F, 4.0F, 6.0F});
    ExpectVectorEqual(2.0F * lhs, {2.0F, 4.0F, 6.0F});

    ExpectVectorEqual(rhs / 2.0F, {2.0F, 2.5F, 3.0F});
}

TEST(Vector, CompoundAssignmentOperators)
{
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

TEST(Vector, EqualityComparison)
{
    const vec3 lhs{1.0F, 2.0F, 3.0F};
    const vec3 rhs{1.0F, 2.0F, 3.0F};
    const vec3 different{1.0F, 2.5F, 3.0F};

    EXPECT_TRUE(lhs == rhs);
    EXPECT_TRUE(!(lhs != rhs));
    EXPECT_TRUE(lhs != different);
}

TEST(Vector, DotLengthAndNormalize)
{
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

TEST(Vector, CrossProduct)
{
    const vec3 lhs{1.0F, 0.0F, 0.0F};
    const vec3 rhs{0.0F, 1.0F, 0.0F};

    const vec3 result = cross(lhs, rhs);
    ExpectVectorEqual(result, {0.0F, 0.0F, 1.0F});
}


TEST(Vector, ReflectAndRefract) {
    const vec3 incident{1.0F, -1.0F, 0.0F};
    const vec3 normal{0.0F, 1.0F, 0.0F};
    const vec3 reflected = reflect(incident, normal);
    ExpectVectorEqual(reflected, {1.0F, 1.0F, 0.0F});

    const vec3 refract_incident{0.0F, -1.0F, 0.0F};
    const float eta = 1.0F / 1.5F;
    const vec3 refracted = refract(refract_incident, normal, eta);
    ExpectVectorEqual(refracted, {0.0F, -1.0F, 0.0F});

    const float sqrt2 = std::sqrt(2.0F);
    const vec3 oblique = normalize(vec3{sqrt2 * 0.5F, -sqrt2 * 0.5F, 0.0F});
    const vec3 tir = refract(oblique, normal, 1.5F);
    EXPECT_FLOAT_EQ(length(tir), 0.0F);
}

TEST(Vector, ProjectionAndLerp) {
    const vec3 a{3.0F, 4.0F, 0.0F};
    const vec3 b{1.0F, 0.0F, 0.0F};
    EXPECT_FLOAT_EQ(projection_coefficient(a, b), 3.0F);
    const vec3 proj = project(a, b);
    ExpectVectorEqual(proj, {3.0F, 0.0F, 0.0F});

    const vec3 zero{};
    EXPECT_FLOAT_EQ(projection_coefficient(a, zero), 0.0F);
    ExpectVectorEqual(project(a, zero), {0.0F, 0.0F, 0.0F});

    const vec3 start{0.0F, 0.0F, 0.0F};
    const vec3 end{2.0F, 2.0F, 2.0F};
    const vec3 mid = lerp(start, end, 0.25F);
    ExpectVectorEqual(mid, {0.5F, 0.5F, 0.5F});
    ExpectVectorEqual(lerp(start, end, 0.0F), {0.0F, 0.0F, 0.0F});
    ExpectVectorEqual(lerp(start, end, 1.0F), {2.0F, 2.0F, 2.0F});
}

TEST(Vector, TypeAliasesProvideExpectedDimensions)
{
    static_assert(std::is_same_v<vec2::value_type, float>);
    static_assert(std::is_same_v<dvec4::value_type, double>);
    static_assert(std::is_same_v<ivec3::value_type, int>);

    vec4 value(1.0F);
    ExpectVectorEqual(value, {1.0F, 1.0F, 1.0F, 1.0F});
}

TEST(Matrix, DefaultConstructedIsZeroInitialized)
{
    Matrix<int, 2, 3> value;
    for (std::size_t row = 0; row < 2; ++row)
    {
        ExpectVectorEqual(value[row], {0, 0, 0});
    }
}

TEST(Matrix, VariadicConstructorFillsRows)
{
    const Matrix<float, 2, 2> value(1.0F, 2.0F, 3.0F, 4.0F);
    ExpectVectorEqual(value[0], {1.0F, 2.0F});
    ExpectVectorEqual(value[1], {3.0F, 4.0F});
}

TEST(Matrix, RowAccessSupportsConstAndNonConst)
{
    Matrix<int, 2, 2> value;
    value[0][0] = 1;
    value[0][1] = 2;
    value[1][0] = 3;
    value[1][1] = 4;

    const Matrix<int, 2, 2>& const_ref = value;
    ExpectVectorEqual(const_ref[0], {1, 2});
    ExpectVectorEqual(const_ref[1], {3, 4});
}

TEST(Matrix, ArithmeticOperators)
{
    const Matrix<float, 2, 2> lhs(1.0F, 2.0F, 3.0F, 4.0F);
    const Matrix<float, 2, 2> rhs(0.5F, 0.5F, 0.5F, 0.5F);

    ExpectVectorEqual((lhs + rhs)[0], {1.5F, 2.5F});
    ExpectVectorEqual((lhs + rhs)[1], {3.5F, 4.5F});

    ExpectVectorEqual((lhs - rhs)[0], {0.5F, 1.5F});
    ExpectVectorEqual((lhs - rhs)[1], {2.5F, 3.5F});

    ExpectVectorEqual((lhs * 2.0F)[0], {2.0F, 4.0F});
    ExpectVectorEqual((2.0F * lhs)[1], {6.0F, 8.0F});
}

TEST(Matrix, CompoundAssignmentOperators)
{
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

TEST(Matrix, MatrixVectorMultiplication)
{
    const Matrix<float, 3, 3> mat(
        1.0F, 2.0F, 3.0F,
        4.0F, 5.0F, 6.0F,
        7.0F, 8.0F, 9.0F);
    const vec3 vec{1.0F, 2.0F, 3.0F};

    const vec3 result = mat * vec;
    ExpectVectorEqual(result, {14.0F, 32.0F, 50.0F});
}

TEST(Matrix, MatrixMatrixMultiplication)
{
    const Matrix<int, 2, 3> lhs(1, 2, 3, 4, 5, 6);
    const Matrix<int, 3, 2> rhs(7, 8, 9, 10, 11, 12);

    const Matrix<int, 2, 2> result = lhs * rhs;
    ExpectVectorEqual(result[0], {58, 64});
    ExpectVectorEqual(result[1], {139, 154});
}

TEST(Quaternion, DefaultConstructedIsZeroInitialized)
{
    Quaternion<int> value;
    ExpectQuaternionEqual(value, {0, 0, 0, 0});
}

TEST(Quaternion, ComponentConstructorAssignsValues)
{
    const Quaternion<float> value(1.0F, 2.0F, 3.0F, 4.0F);
    ExpectQuaternionEqual(value, {1.0F, 2.0F, 3.0F, 4.0F});
}

TEST(Quaternion, ArithmeticOperators)
{
    const Quaternion<float> lhs(1.0F, 2.0F, 3.0F, 4.0F);
    const Quaternion<float> rhs(0.5F, 1.0F, -1.0F, 2.0F);

    ExpectQuaternionEqual(lhs + rhs, {1.5F, 3.0F, 2.0F, 6.0F});
    ExpectQuaternionEqual(lhs - rhs, {0.5F, 1.0F, 4.0F, 2.0F});
    ExpectQuaternionEqual(lhs * 2.0F, {2.0F, 4.0F, 6.0F, 8.0F});
    ExpectQuaternionEqual(2.0F * lhs, {2.0F, 4.0F, 6.0F, 8.0F});
    ExpectQuaternionEqual(lhs / 2.0F, {0.5F, 1.0F, 1.5F, 2.0F});
}

TEST(Quaternion, HamiltonProduct)
{
    const Quaternion<float> identity(1.0F, 0.0F, 0.0F, 0.0F);
    const Quaternion<float> value(0.0F, 1.0F, 0.0F, 0.0F);
    const Quaternion<float> other(0.0F, 0.0F, 1.0F, 0.0F);

    EXPECT_TRUE(identity * value == value);
    const Quaternion<float> result = value * other;
    ExpectQuaternionEqual(result, {-0.0F, 0.0F, 0.0F, 1.0F});
}

TEST(Quaternion, ConjugateLengthNormalizeAndInverse)
{
    const Quaternion<double> value(1.0, 2.0, 3.0, 4.0);

    const Quaternion<double> conjugated = conjugate(value);
    ExpectQuaternionEqual(conjugated, {1.0, -2.0, -3.0, -4.0});

    const double tolerance = 1e-12;
    EXPECT_TRUE(std::abs(length_squared(value) - 30.0) <= tolerance);
    EXPECT_TRUE(std::abs(length(value) - std::sqrt(30.0)) <= tolerance);

    const Quaternion<double> normalized = normalize(value);
    const double inv_len = 1.0 / std::sqrt(30.0);
    EXPECT_TRUE(std::abs(normalized.w - 1.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.x - 2.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.y - 3.0 * inv_len) <= tolerance);
    EXPECT_TRUE(std::abs(normalized.z - 4.0 * inv_len) <= tolerance);

    const Quaternion<double> inverse_value = inverse(value);
    const Quaternion<double> identity = value * inverse_value;
    EXPECT_TRUE(std::abs(identity.w - 1.0) <= tolerance);
    EXPECT_TRUE(std::abs(identity.x) <= tolerance);
    EXPECT_TRUE(std::abs(identity.y) <= tolerance);
    EXPECT_TRUE(std::abs(identity.z) <= tolerance);
}

TEST(Quaternion, SlerpSquadAndCast) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const Quaternion<float> identity = Quaternion<float>::Identity();
    const vec3 axis{0.0F, 0.0F, 1.0F};
    const Quaternion<float> quarter = from_angle_axis(pi / 2.0F, axis);
    const Quaternion<float> eighth = from_angle_axis(pi / 4.0F, axis);

    const Quaternion<float> slerp_half = slerp(identity, quarter, 0.5F);
    const float tol = 1e-5F;
    ExpectNear(slerp_half.w, eighth.w, tol);
    ExpectNear(slerp_half.x, eighth.x, tol);
    ExpectNear(slerp_half.y, eighth.y, tol);
    ExpectNear(slerp_half.z, eighth.z, tol);

    const Quaternion<float> squad_half = squad(identity, identity, quarter, quarter, 0.5F);
    ExpectNear(squad_half.w, eighth.w, tol);
    ExpectNear(squad_half.x, eighth.x, tol);
    ExpectNear(squad_half.y, eighth.y, tol);
    ExpectNear(squad_half.z, eighth.z, tol);

    const Quaternion<float> cast_src(0.25F, -0.5F, 0.75F, -1.0F);
    const auto cast_dst = cast<double>(cast_src);
    ExpectNear(cast_dst.w, 0.25, 1e-12);
    ExpectNear(cast_dst.x, -0.5, 1e-12);
    ExpectNear(cast_dst.y, 0.75, 1e-12);
    ExpectNear(cast_dst.z, -1.0, 1e-12);
}

TEST(Quaternion, AngleAxisAndEulerConversions) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const vec3 axis = normalize(vec3{1.0F, 2.0F, 3.0F});
    const Quaternion<float> q = from_angle_axis(pi / 3.0F, axis);
    const Vector<float, 4> aa = to_angle_axis(q);
    const float tol = 1e-5F;
    ExpectNear(aa[0], pi / 3.0F, tol);
    const vec3 recovered_axis{aa[1], aa[2], aa[3]};
    ExpectNear(length(recovered_axis), 1.0F, tol);
    ExpectNear(utils::abs(dot(recovered_axis, axis)), 1.0F, tol);

    const Quaternion<float> qx = from_angle_axis(pi / 2.0F, vec3{1.0F, 0.0F, 0.0F});
    const Vector<float, 3> euler = to_euler_angles(qx);
    ExpectNear(euler[0], pi / 2.0F, tol);
    ExpectNear(euler[1], 0.0F, tol);
    ExpectNear(euler[2], 0.0F, tol);
}

TEST(RotationUtils, QuaternionMatrixRoundTrip) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const vec3 axis = normalize(vec3{0.3F, 0.4F, 0.5F});
    const Quaternion<float> q = from_angle_axis(pi / 5.0F, axis);
    const mat3 rot = utils::to_rotation_matrix(q);
    const Quaternion<float> round = normalize(utils::to_quaternion(rot));
    const float alignment = utils::abs(dot(q, round));
    ExpectNear(alignment, 1.0F, 1e-5F);
}

TEST(RotationUtils, AngleAxisOverloadsProduceConsistentMatrices) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const float angle = pi / 6.0F;
    const vec3 axis = normalize(vec3{1.0F, 1.0F, 0.5F});
    const vec3 angle_axis3 = axis * angle;
    const vec4 angle_axis4{angle, axis[0], axis[1], axis[2]};

    const mat3 R1 = utils::to_rotation_matrix(angle, axis);
    const mat3 R2 = utils::to_rotation_matrix(angle_axis3);
    const mat3 R3 = utils::to_rotation_matrix(angle_axis4);

    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            ExpectNear(R1[r][c], R2[r][c], 1e-5F);
            ExpectNear(R1[r][c], R3[r][c], 1e-5F);
        }
    }
}

TEST(Quaternion, FromAngleAxis)
{
    // Test 90-degree rotation around Z-axis
    const vec3 z_axis{0.0F, 0.0F, 1.0F};
    const auto quat = from_angle_axis(std::numbers::pi_v<float> / 2.0F, z_axis);

    const float half_angle = std::numbers::pi_v<float> / 4.0F;
    const float cos_half = std::cos(half_angle);
    const float sin_half = std::sin(half_angle);

    EXPECT_FLOAT_EQ(quat.w, cos_half);
    EXPECT_FLOAT_EQ(quat.x, 0.0F);
    EXPECT_FLOAT_EQ(quat.y, 0.0F);
    EXPECT_FLOAT_EQ(quat.z, sin_half);

    // Test identity rotation (zero angle)
    const auto identity = from_angle_axis(0.0F, z_axis);
    ExpectQuaternionEqual(identity, {1.0F, 0.0F, 0.0F, 0.0F});
}

TEST(Quaternion, ToRotationMatrix)
{
    // Test 90-degree rotation around Z-axis
    const vec3 z_axis{0.0F, 0.0F, 1.0F};
    const auto quat = from_angle_axis(std::numbers::pi_v<float> / 2.0F, z_axis);
    const mat4 rotation = engine::math::utils::to_rotation_matrix(quat);

    // Apply rotation to X-axis vector, should get Y-axis
    const vec4 x_vec{1.0F, 0.0F, 0.0F, 1.0F};
    const vec4 result = rotation * x_vec;

    const float tolerance = 1e-6F;
    EXPECT_TRUE(utils::nearly_equal(result[0], 0.0F, tolerance));
    EXPECT_TRUE(utils::nearly_equal(result[1], 1.0F, tolerance));
    EXPECT_TRUE(utils::nearly_equal(result[2], 0.0F, tolerance));
    EXPECT_TRUE(utils::nearly_equal(result[3], 1.0F, tolerance));
}

TEST(Quaternion, FromRotationMatrix)
{
    // Create a known rotation matrix (90 degrees around Z)
    const vec3 z_axis{0.0F, 0.0F, 1.0F};
    const auto original_quat = from_angle_axis(std::numbers::pi_v<float> / 2.0F, z_axis);
    const mat4 rotation = utils::to_rotation_matrix(original_quat);

    // Convert back to quaternion
    const auto recovered_quat = from_rotation_matrix(rotation);

    const float tolerance = 1e-6F;
    EXPECT_TRUE(utils::nearly_equal(recovered_quat.w, original_quat.w, tolerance));
    EXPECT_TRUE(utils::nearly_equal(recovered_quat.x, original_quat.x, tolerance));
    EXPECT_TRUE(utils::nearly_equal(recovered_quat.y, original_quat.y, tolerance));
    EXPECT_TRUE(utils::nearly_equal(recovered_quat.z, original_quat.z, tolerance));
}

TEST(TypeConversion, Cast)
{
    // Test vector cast
    const vec3 float_vec{1.5F, 2.5F, 3.5F};
    const auto int_vec = cast<int>(float_vec);
    ExpectVectorEqual(int_vec, {1, 2, 3});

    const auto double_vec = cast<double>(float_vec);
    EXPECT_EQ(double_vec[0], 1.5);
    EXPECT_EQ(double_vec[1], 2.5);
    EXPECT_EQ(double_vec[2], 3.5);

    // Test quaternion cast
    const Quaternion<float> float_quat(1.5F, 2.5F, 3.5F, 4.5F);
    const auto double_quat = cast<double>(float_quat);
    EXPECT_EQ(double_quat.w, 1.5);
    EXPECT_EQ(double_quat.x, 2.5);
    EXPECT_EQ(double_quat.y, 3.5);
    EXPECT_EQ(double_quat.z, 4.5);
}

TEST(Matrix, Transpose)
{
    const Matrix<int, 2, 3> value(1, 2, 3, 4, 5, 6);
    const Matrix<int, 3, 2> transposed = transpose(value);

    ExpectVectorEqual(transposed[0], {1, 4});
    ExpectVectorEqual(transposed[1], {2, 5});
    ExpectVectorEqual(transposed[2], {3, 6});
}

TEST(Matrix, StoresColumnsInColumnMajorOrder)
{
    const Matrix<int, 2, 3> value(1, 2, 3, 4, 5, 6);

    EXPECT_EQ(value.columns[0][0], 1);
    EXPECT_EQ(value.columns[0][1], 4);
    EXPECT_EQ(value.columns[1][0], 2);
    EXPECT_EQ(value.columns[1][1], 5);
    EXPECT_EQ(value.columns[2][0], 3);
    EXPECT_EQ(value.columns[2][1], 6);
}

TEST(Matrix, IdentityMatrixHasOnesOnDiagonal)
{
    const auto id = identity_matrix<double, 3>();
    for (std::size_t r = 0; r < 3; ++r)
    {
        for (std::size_t c = 0; c < 3; ++c)
        {
            const double expected = (r == c) ? detail::one<double>() : detail::zero<double>();
            EXPECT_EQ(id[r][c], expected);
        }
    }
}

TEST(Matrix, TranslationProducesAffineMatrix)
{
    const vec3 offset{1.0F, 2.0F, 3.0F};
    const Matrix<float, 4, 4> transform = translation(offset);

    for (std::size_t i = 0; i < 3; ++i)
    {
        EXPECT_FLOAT_EQ(transform[i][i], 1.0F);
        EXPECT_FLOAT_EQ(transform[i][3], offset[i]);
    }
    EXPECT_FLOAT_EQ(transform[3][0], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][1], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][2], 0.0F);
    EXPECT_FLOAT_EQ(transform[3][3], 1.0F);
}

TEST(Matrix, ScaleSetsDiagonalAndLeavesTranslationZero)
{
    const vec3 factors{2.0F, 3.0F, 4.0F};
    const Matrix<float, 4, 4> transform = scale(factors);

    EXPECT_FLOAT_EQ(transform[0][0], 2.0F);
    EXPECT_FLOAT_EQ(transform[1][1], 3.0F);
    EXPECT_FLOAT_EQ(transform[2][2], 4.0F);
    EXPECT_FLOAT_EQ(transform[3][3], 1.0F);

    for (std::size_t r = 0; r < 4; ++r)
    {
        for (std::size_t c = 0; c < 4; ++c)
        {
            if (r != c)
            {
                EXPECT_FLOAT_EQ(transform[r][c], 0.0F);
            }
        }
    }
}

TEST(Matrix, TypeAliasesCompile)
{
    mat4 float_mat = identity_matrix<float, 4>();
    dmat3 double_mat = identity_matrix<double, 3>();

    EXPECT_FLOAT_EQ(float_mat[0][0], 1.0F);
    EXPECT_EQ(double_mat[0][0], 1.0);
}

TEST(Matrix, DeterminantMatchesAnalyticValues) {
    const Matrix<float, 2, 2> m2(3.0F, 4.0F,
                                 2.0F, 5.0F);
    EXPECT_FLOAT_EQ(determinant(m2), 7.0F);

    const Matrix<float, 3, 3> m3(2.0F, 0.0F, 0.0F,
                                 1.0F, 3.0F, 0.0F,
                                 4.0F, 5.0F, 4.0F);
    EXPECT_FLOAT_EQ(determinant(m3), 24.0F);

    const Matrix<float, 4, 4> m4(2.0F, 0.0F, 0.0F, 0.0F,
                                 1.0F, 3.0F, 0.0F, 0.0F,
                                 4.0F, 5.0F, 4.0F, 0.0F,
                                 7.0F, 8.0F, 9.0F, 5.0F);
    EXPECT_FLOAT_EQ(determinant(m4), 120.0F);
}

TEST(Matrix, TryInverseReturnsExpectedResult) {
    const Matrix<float, 2, 2> m2(4.0F, 7.0F,
                                 2.0F, 6.0F);
    const auto inv2 = try_inverse(m2);
    ASSERT_TRUE(inv2.has_value());
    ExpectNear((*inv2)[0][0], 0.6F, 1e-6F);
    ExpectNear((*inv2)[0][1], -0.7F, 1e-6F);
    ExpectNear((*inv2)[1][0], -0.2F, 1e-6F);
    ExpectNear((*inv2)[1][1], 0.4F, 1e-6F);

    const Matrix<float, 3, 3> m3(0.0F, -1.0F, 0.0F,
                                 1.0F,  0.0F, 0.0F,
                                 0.0F,  0.0F, 1.0F);
    const auto inv3 = try_inverse(m3);
    ASSERT_TRUE(inv3.has_value());
    const Matrix<float, 3, 3> expected3(0.0F, 1.0F, 0.0F,
                                        -1.0F, 0.0F, 0.0F,
                                        0.0F, 0.0F, 1.0F);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            ExpectNear((*inv3)[r][c], expected3[r][c], 1e-6F);
        }
    }

    Matrix<float, 4, 4> m4 = identity_matrix<float, 4>();
    m4[0][0] = 2.0F; m4[1][1] = 3.0F; m4[2][2] = 4.0F;
    m4[0][3] = 1.0F; m4[1][3] = 2.0F; m4[2][3] = 3.0F;
    const auto inv4 = try_inverse(m4);
    ASSERT_TRUE(inv4.has_value());
    const Matrix<float, 4, 4> expected4(0.5F, 0.0F, 0.0F, -0.5F,
                                        0.0F, 1.0F / 3.0F, 0.0F, -2.0F / 3.0F,
                                        0.0F, 0.0F, 0.25F, -0.75F,
                                        0.0F, 0.0F, 0.0F, 1.0F);
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            ExpectNear((*inv4)[r][c], expected4[r][c], 1e-5F);
        }
    }

    const Matrix<float, 2, 2> singular(1.0F, 2.0F,
                                       2.0F, 4.0F);
    EXPECT_FALSE(try_inverse(singular).has_value());
}
