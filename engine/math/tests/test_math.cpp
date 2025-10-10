#include <array>
#include <cmath>
#include <type_traits>
#include <numbers>


#include <gtest/gtest.h>

#include "engine/math/common.hpp"
#include "engine/math/utils_rotation.hpp"
#include "engine/math/math.hpp"

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

    template <typename T>
    Quaternion<T> AlignQuaternion(const Quaternion<T>& value, const Quaternion<T>& reference)
    {
        Quaternion<T> aligned = value;
        if (dot(aligned, reference) < T(0))
        {
            aligned.w = -aligned.w;
            aligned.x = -aligned.x;
            aligned.y = -aligned.y;
            aligned.z = -aligned.z;
        }
        return aligned;
    }

    template <typename T>
    void ExpectQuaternionNear(const Quaternion<T>& value, const Quaternion<T>& expected, T tolerance)
    {
        EXPECT_NEAR(value.w, expected.w, tolerance);
        EXPECT_NEAR(value.x, expected.x, tolerance);
        EXPECT_NEAR(value.y, expected.y, tolerance);
        EXPECT_NEAR(value.z, expected.z, tolerance);
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
    EXPECT_NEAR(slerp_half.w, eighth.w, tol);
    EXPECT_NEAR(slerp_half.x, eighth.x, tol);
    EXPECT_NEAR(slerp_half.y, eighth.y, tol);
    EXPECT_NEAR(slerp_half.z, eighth.z, tol);

    const Quaternion<float> squad_half = squad(identity, identity, quarter, quarter, 0.5F);
    EXPECT_NEAR(squad_half.w, eighth.w, tol);
    EXPECT_NEAR(squad_half.x, eighth.x, tol);
    EXPECT_NEAR(squad_half.y, eighth.y, tol);
    EXPECT_NEAR(squad_half.z, eighth.z, tol);

    const Quaternion<float> cast_src(0.25F, -0.5F, 0.75F, -1.0F);
    const auto cast_dst = cast<double>(cast_src);
    EXPECT_NEAR(cast_dst.w, 0.25, 1e-12);
    EXPECT_NEAR(cast_dst.x, -0.5, 1e-12);
    EXPECT_NEAR(cast_dst.y, 0.75, 1e-12);
    EXPECT_NEAR(cast_dst.z, -1.0, 1e-12);
}

TEST(Quaternion, AngleAxisAndEulerConversions) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const vec3 axis = normalize(vec3{1.0F, 2.0F, 3.0F});
    const Quaternion<float> q = from_angle_axis(pi / 3.0F, axis);
    const Vector<float, 4> aa = to_angle_axis(q);
    const float tol = 1e-5F;
    EXPECT_NEAR(aa[0], pi / 3.0F, tol);
    const vec3 recovered_axis{aa[1], aa[2], aa[3]};
    EXPECT_NEAR(length(recovered_axis), 1.0F, tol);
    EXPECT_NEAR(utils::abs(dot(recovered_axis, axis)), 1.0F, tol);

    const Quaternion<float> qx = from_angle_axis(pi / 2.0F, vec3{1.0F, 0.0F, 0.0F});
    const Vector<float, 3> euler = to_euler_angles(qx);
    EXPECT_NEAR(euler[0], pi / 2.0F, tol);
    EXPECT_NEAR(euler[1], 0.0F, tol);
    EXPECT_NEAR(euler[2], 0.0F, tol);
}

TEST(RotationUtils, QuaternionMatrixRoundTrip) {
    const float pi = static_cast<float>(std::acos(-1.0));
    const vec3 axis = normalize(vec3{0.3F, 0.4F, 0.5F});
    const Quaternion<float> q = from_angle_axis(pi / 5.0F, axis);
    const mat3 rot = utils::to_rotation_matrix(q);
    const Quaternion<float> round = normalize(utils::to_quaternion(rot));
    const float alignment = utils::abs(dot(q, round));
    EXPECT_NEAR(alignment, 1.0F, 1e-5F);
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
            EXPECT_NEAR(R1[r][c], R2[r][c], 1e-5F);
            EXPECT_NEAR(R1[r][c], R3[r][c], 1e-5F);
        }
    }
}

TEST(RotationUtils, OrthonormalBasisProducesRightHandedFrame)
{
    const vec3 direction = normalize(vec3{1.0F, 2.0F, 3.0F});
    const auto basis = utils::orthonormal_basis(direction);

    const vec3& tangent = basis[0];
    const vec3& bitangent = basis[1];
    const vec3& normal = basis[2];

    const float tol = 1e-5F;
    EXPECT_NEAR(length(tangent), 1.0F, tol);
    EXPECT_NEAR(length(bitangent), 1.0F, tol);
    EXPECT_NEAR(length(normal), 1.0F, tol);

    EXPECT_NEAR(dot(tangent, bitangent), 0.0F, tol);
    EXPECT_NEAR(dot(tangent, normal), 0.0F, tol);
    EXPECT_NEAR(dot(bitangent, normal), 0.0F, tol);

    const vec3 reconstructed = cross(tangent, bitangent);
    EXPECT_NEAR(dot(reconstructed, normal), 1.0F, tol);
}

TEST(RotationUtils, OrthonormalBasisHandlesDegenerateInputs)
{
    const auto canonical = utils::orthonormal_basis(vec3{0.0F, 0.0F, 0.0F});
    ExpectVectorEqual(canonical[0], {1.0F, 0.0F, 0.0F});
    ExpectVectorEqual(canonical[1], {0.0F, 1.0F, 0.0F});
    ExpectVectorEqual(canonical[2], {0.0F, 0.0F, 1.0F});

    const vec3 negative_z{0.0F, 0.0F, -1.0F};
    const auto basis = utils::orthonormal_basis(negative_z);
    ExpectVectorEqual(basis[0], {0.0F, -1.0F, 0.0F});
    ExpectVectorEqual(basis[1], {-1.0F, 0.0F, 0.0F});
    ExpectVectorEqual(basis[2], {0.0F, 0.0F, -1.0F});
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
    EXPECT_NEAR((*inv2)[0][0], 0.6F, 1e-6F);
    EXPECT_NEAR((*inv2)[0][1], -0.7F, 1e-6F);
    EXPECT_NEAR((*inv2)[1][0], -0.2F, 1e-6F);
    EXPECT_NEAR((*inv2)[1][1], 0.4F, 1e-6F);

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
            EXPECT_NEAR((*inv3)[r][c], expected3[r][c], 1e-6F);
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
            EXPECT_NEAR((*inv4)[r][c], expected4[r][c], 1e-5F);
        }
    }

    const Matrix<float, 2, 2> singular(1.0F, 2.0F,
                                       2.0F, 4.0F);
    EXPECT_FALSE(try_inverse(singular).has_value());
}

TEST(Matrix, InverseAndCombineReturnIdentity)
{
    const Matrix<float, 2, 2> m2(4.0F, 7.0F,
                                  2.0F, 6.0F);
    const auto inv2 = try_inverse(m2);
    ASSERT_TRUE(inv2.has_value());
    const auto matrix2 = (*inv2) * m2;

    for (std::size_t r = 0; r < 2; ++r) {
        for (std::size_t c = 0; c < 2; ++c) {
            const float expected = (r == c) ? 1.0F : 0.0F;
            EXPECT_NEAR(matrix2[r][c], expected, 1e-6F);
        }
    }

    const Matrix<float, 3, 3> m3(0.0F, -1.0F, 0.0F,
                                 1.0F,  0.0F, 0.0F,
                                 0.0F,  0.0F, 1.0F);
    const auto inv3 = try_inverse(m3);
    ASSERT_TRUE(inv3.has_value());
    const auto matrix3 = (*inv3) * m3;
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            const float expected = (r == c) ? 1.0F : 0.0F;
            EXPECT_NEAR(matrix3[r][c], expected, 1e-6F);
        }
    }

    Matrix<float, 4, 4> m4 = identity_matrix<float, 4>();
    m4[0][0] = 2.0F; m4[1][1] = 3.0F; m4[2][2] = 4.0F;
    m4[0][3] = 1.0F; m4[1][3] = 2.0F; m4[2][3] = 3.0F;
    const auto inv4 = try_inverse(m4);
    ASSERT_TRUE(inv4.has_value());
    const auto matrix4 = (*inv4) * m4;
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            const float expected = (r == c) ? 1.0F : 0.0F;
            EXPECT_NEAR(matrix4[r][c], expected, 1e-5F);
        }
    }
}

TEST(Quaternion, CayleyParameterizationRoundTrip)
{
    const float tol = 1e-5F;
    const std::array<Vector<float, 4>, 4> samples{
        Vector<float, 4>{0.0F, 1.0F, 0.0F, 0.0F},
        Vector<float, 4>{std::numbers::pi_v<float> / 3.0F, 0.0F, 1.0F, 0.0F},
        Vector<float, 4>{-std::numbers::pi_v<float> / 2.0F, 0.0F, 0.0F, 1.0F},
        Vector<float, 4>{std::numbers::pi_v<float> * 0.75F, 1.0F, 1.0F, -0.5F},
    };

    for (const auto& sample : samples)
    {
        const Vector<float, 4> normalized_axis{
            sample[0],
            sample[1],
            sample[2],
            sample[3],
        };

        const Quaternion<float> original = normalize(from_angle_axis(normalized_axis));
        const Vector<float, 3> cayley = to_cayley_parameters(original);
        const Quaternion<float> reconstructed = normalize(from_cayley_parameters(cayley));

        const Quaternion<float> aligned = AlignQuaternion(reconstructed, original);
        ExpectQuaternionNear(aligned, original, tol);

        const Vector<float, 4> round_trip_axis = to_angle_axis(reconstructed);
        const Quaternion<float> round_trip = normalize(from_angle_axis(round_trip_axis));
        const Quaternion<float> aligned_round_trip = AlignQuaternion(round_trip, original);
        ExpectQuaternionNear(aligned_round_trip, original, tol);
    }
}

TEST(Transform, ToMatrixMatchesComponents)
{
    const vec3 scale{1.5F, 0.25F, -2.0F};
    const Quaternion<float> rotation = normalize(from_angle_axis(std::numbers::pi_v<float> / 5.0F,
                                                                 normalize(vec3{0.3F, -0.7F, 0.2F})));
    const vec3 translation{0.5F, -1.0F, 3.0F};
    const Transform<float> transform{scale, rotation, translation};

    const mat4 matrix = to_matrix(transform);

    mat4 expected = utils::to_rotation_matrix(rotation);
    for (std::size_t column = 0; column < 3; ++column)
    {
        expected.columns[column] *= scale[column];
    }
    expected[0][3] = translation[0];
    expected[1][3] = translation[1];
    expected[2][3] = translation[2];

    for (std::size_t r = 0; r < 4; ++r)
    {
        for (std::size_t c = 0; c < 4; ++c)
        {
            EXPECT_NEAR(matrix[r][c], expected[r][c], 1e-5F);
        }
    }
}

TEST(Transform, MatrixRoundTripPreservesComponents)
{
    const vec3 scale{2.0F, -3.0F, 0.5F};
    const Quaternion<float> rotation = normalize(from_angle_axis(std::numbers::pi_v<float> / 4.0F,
                                                                 normalize(vec3{0.2F, 1.0F, -0.3F})));
    const vec3 translation{4.0F, -2.5F, 1.0F};
    const Transform<float> transform{scale, rotation, translation};

    const mat4 matrix = to_matrix(transform);
    const Transform<float> recovered = from_matrix(matrix);

    ExpectVectorNear(recovered.scale, {scale[0], scale[1], scale[2]}, 1e-4F);
    ExpectVectorNear(recovered.translation, {translation[0], translation[1], translation[2]}, 1e-5F);

    const Quaternion<float> aligned = AlignQuaternion(normalize(recovered.rotation), normalize(rotation));
    ExpectQuaternionNear(aligned, normalize(rotation), 1e-5F);
}

TEST(Transform, PointAndVectorTransformMatchMatrixApplication)
{
    const vec3 scale{0.75F, 1.25F, 1.5F};
    const Quaternion<float> rotation = normalize(from_angle_axis(std::numbers::pi_v<float> / 6.0F,
                                                                 normalize(vec3{-0.5F, 0.8F, 0.3F})));
    const vec3 translation{-2.0F, 0.5F, 1.0F};
    const Transform<float> transform{scale, rotation, translation};

    const vec3 point{1.0F, -2.0F, 0.5F};
    const vec3 direction{-0.25F, 0.75F, 1.0F};

    const vec3 transformed_point = transform_point(transform, point);
    const vec3 transformed_vector = transform_vector(transform, direction);

    const mat4 matrix = to_matrix(transform);
    const Vector<float, 4> point4(point[0], point[1], point[2], 1.0F);
    const Vector<float, 4> direction4(direction[0], direction[1], direction[2], 0.0F);

    const Vector<float, 4> matrix_point = matrix * point4;
    const Vector<float, 4> matrix_vector = matrix * direction4;

    ExpectVectorNear(transformed_point, {matrix_point[0], matrix_point[1], matrix_point[2]}, 1e-5F);
    EXPECT_NEAR(matrix_point[3], 1.0F, 1e-5F);

    ExpectVectorNear(transformed_vector, {matrix_vector[0], matrix_vector[1], matrix_vector[2]}, 1e-5F);
    EXPECT_NEAR(matrix_vector[3], 0.0F, 1e-5F);
}

TEST(Transform, InverseAndCombineReturnIdentity)
{
    const float s = 1.25F;
    const vec3 scale{s, s, s}; // uniform scale enables exact inverse in current representation
    const Quaternion<float> rotation = normalize(from_angle_axis(std::numbers::pi_v<float> / 3.5F,
                                                                 normalize(vec3{0.4F, -0.6F, 0.7F})));
    const vec3 translation{1.0F, -0.5F, 2.0F};
    const Transform<float> transform{scale, rotation, translation};

    const Transform<float> inverse_transform = inverse(transform);
    const Transform<float> composed = combine(transform, inverse_transform);

    const mat4 matrix = to_matrix(composed);
    for (std::size_t r = 0; r < 4; ++r)
    {
        for (std::size_t c = 0; c < 4; ++c)
        {
            const float expected = (r == c) ? 1.0F : 0.0F;
            EXPECT_NEAR(matrix[r][c], expected, 1e-4F);
        }
    }

    const vec3 point{0.25F, -0.75F, 1.5F};
    const vec3 forward = transform_point(transform, point);
    const vec3 back = transform_point(inverse_transform, forward);
    ExpectVectorNear(back, {point[0], point[1], point[2]}, 1e-4F);
}

// ===================== SparseMatrix tests =====================

TEST(SparseMatrix, BuildFromTripletsAndMultiply)
{
    using T = float;
    using SM = SparseMatrix<T>;

    // Matrix:
    // [ 10  2  0 ]
    // [  0  3  4 ]
    // [  1  0  5 ]
    std::vector<SM::Triplet> trips = {
        {0,0,10}, {2,0,1},
        {0,1, 2}, {1,1,3},
        {1,2, 4}, {2,2,5}
    };
    SM A = SM::from_triplets(3, 3, trips, /*sum_duplicates*/true);

    ASSERT_EQ(A.rows(), 3u);
    ASSERT_EQ(A.cols(), 3u);
    ASSERT_TRUE(A.is_column_sorted());
    ASSERT_EQ(A.nnz(), 6u);

    // Multiply by x = [1,2,3]^T
    const std::vector<T> x{1,2,3};
    const auto y = A * x;

    ASSERT_EQ(y.size(), 3u);
    // Expected y = [10*1 + 2*2, 3*2 + 4*3, 1*1 + 5*3] = [14, 18, 16]
    EXPECT_NEAR(y[0], 14.0F, 1e-6F);
    EXPECT_NEAR(y[1], 18.0F, 1e-6F);
    EXPECT_NEAR(y[2], 16.0F, 1e-6F);

    // accumulate into y2
    std::vector<T> y2(3, 1.0F); // start from [1,1,1]
    A.multiply_accumulate(x, y2);
    EXPECT_NEAR(y2[0], 15.0F, 1e-6F);
    EXPECT_NEAR(y2[1], 19.0F, 1e-6F);
    EXPECT_NEAR(y2[2], 17.0F, 1e-6F);
}

TEST(SparseMatrix, TryGetSetAddToAndOrdering)
{
    using T = double;
    using SM = SparseMatrix<T>;

    // Start with a single column, test ordering on inserts
    SM A(4, 3);
    // Insert via set (new entries)
    A.set(2, 0, 5.0); // (2,0)=5
    A.set(0, 0, 1.0); // (0,0)=1  (should keep rows sorted: 0 then 2)
    A.set(3, 0, 7.0); // (3,0)=7

    // Another column
    A.add_to(1, 1, 2.5); // (1,1)+=2.5  (creates)
    A.add_to(1, 1, 0.5); // (1,1)+=0.5  (now 3.0)
    A.set(0, 2, -4.0);

    ASSERT_TRUE(A.is_column_sorted());
    EXPECT_EQ(A.nnz(), 5u);

    auto v00 = A.try_get(0,0); ASSERT_TRUE(v00.has_value()); EXPECT_NEAR(*v00, 1.0, 1e-12);
    auto v20 = A.try_get(2,0); ASSERT_TRUE(v20.has_value()); EXPECT_NEAR(*v20, 5.0, 1e-12);
    auto v30 = A.try_get(3,0); ASSERT_TRUE(v30.has_value()); EXPECT_NEAR(*v30, 7.0, 1e-12);
    auto v11 = A.try_get(1,1); ASSERT_TRUE(v11.has_value()); EXPECT_NEAR(*v11, 3.0, 1e-12);
    auto v02 = A.try_get(0,2); ASSERT_TRUE(v02.has_value()); EXPECT_NEAR(*v02, -4.0, 1e-12);
    auto v12 = A.try_get(1,2); EXPECT_FALSE(v12.has_value()); // not present

    // mat-vec quick check
    const std::vector<T> x{1.0, 2.0, -1.0}; // size=cols
    const auto y = A * x;
    ASSERT_EQ(y.size(), 4u);
    // y = col0*1 + col1*2 + col2*(-1)
    // col0 = [(0,0)=1, (2,0)=5, (3,0)=7] -> [1,0,5,7]
    // col1 = [(1,1)=3] -> [0,3,0,0]
    // col2 = [(0,2)=-4] -> [-4,0,0,0]
    // y = [1 + 0*2 + (-4)*(-1), 0 + 3*2 + 0, 5 + 0 + 0, 7 + 0 + 0]
    //   = [1 + 0 + 4, 6, 5, 7] = [5, 6, 5, 7]
    EXPECT_NEAR(y[0], 5.0, 1e-12);
    EXPECT_NEAR(y[1], 6.0, 1e-12);
    EXPECT_NEAR(y[2], 5.0, 1e-12);
    EXPECT_NEAR(y[3], 7.0, 1e-12);
}

TEST(SparseMatrix, FromTripletsSumsDuplicatesAndDropsZeros)
{
    using T = int;
    using SM = SparseMatrix<T>;

    std::vector<SM::Triplet> trips = {
        {0,0, 2}, {0,0, -2}, // cancels to zero -> dropped
        {1,0, 3}, {1,0, 1},  // sums to 4
        {0,1, 5}
    };
    SM A = SM::from_triplets(2, 2, trips, /*sum_duplicates*/true);

    EXPECT_EQ(A.nnz(), 2u);
    auto v10 = A.try_get(1,0); ASSERT_TRUE(v10.has_value()); EXPECT_EQ(*v10, 4);
    auto v01 = A.try_get(0,1); ASSERT_TRUE(v01.has_value()); EXPECT_EQ(*v01, 5);
    auto v00 = A.try_get(0,0); EXPECT_FALSE(v00.has_value());
}

TEST(SparseMatrix, PlusMinusScalarMulAndPrune)
{
    using T = float;
    using SM = SparseMatrix<T>;

    // A:
    // [ 1 0 ]
    // [ 2 3 ]
    // [ 0 4 ]
    std::vector<SM::Triplet> Ta = {{0,0,1},{1,0,2},{1,1,3},{2,1,4}};
    SM A = SM::from_triplets(3, 2, Ta, true);

    // B:
    // [ 2 5 ]
    // [ 0 0 ]
    // [ 1 1 ]
    std::vector<SM::Triplet> Tb = {{0,0,2},{0,1,5},{2,0,1},{2,1,1}};
    SM B = SM::from_triplets(3, 2, Tb, true);

    SM C = A + B;
    // C expected:
    // [ 3 5 ]
    // [ 2 3 ]
    // [ 1 5 ]
    {
        auto c00 = C.try_get(0,0); ASSERT_TRUE(c00.has_value()); EXPECT_NEAR(*c00, 3.0F, 1e-6F);
        auto c10 = C.try_get(1,0); ASSERT_TRUE(c10.has_value()); EXPECT_NEAR(*c10, 2.0F, 1e-6F);
        auto c20 = C.try_get(2,0); ASSERT_TRUE(c20.has_value()); EXPECT_NEAR(*c20, 1.0F, 1e-6F);

        auto c01 = C.try_get(0,1); ASSERT_TRUE(c01.has_value()); EXPECT_NEAR(*c01, 5.0F, 1e-6F);
        auto c11 = C.try_get(1,1); ASSERT_TRUE(c11.has_value()); EXPECT_NEAR(*c11, 3.0F, 1e-6F);
        auto c21 = C.try_get(2,1); ASSERT_TRUE(c21.has_value()); EXPECT_NEAR(*c21, 5.0F, 1e-6F);
    }

    SM D = C - A; // should equal B
    EXPECT_EQ(D.nnz(), B.nnz());
    // spot-check via multiply
    const std::vector<T> x{2.0F, -1.0F};
    const auto yB = B * x;
    const auto yD = D * x;
    ASSERT_EQ(yB.size(), yD.size());
    for (size_t i = 0; i < yB.size(); ++i) {
        EXPECT_NEAR(yB[i], yD[i], 1e-6F);
    }

    // Scalar multiply then prune
    C *= 0.0F;
    // Now all stored values are explicit zeros; prune_zeros should drop them.
    const auto nnz_before = C.nnz();
    C.prune_zeros();
    EXPECT_LE(C.nnz(), nnz_before);
    EXPECT_EQ((SM::size_type)0, C.nnz());
}

TEST(SparseMatrix, TransposeAdjointIdentity)
{
    using T = double;
    using SM = SparseMatrix<T>;

    // Random-looking small A (3x4)
    std::vector<SM::Triplet> trips = {
        {0,0, 1.0}, {2,0, -2.0},
        {1,1, 3.0},
        {0,2, 4.0}, {2,2, 5.0},
        {1,3, -1.0}
    };
    SM A = SM::from_triplets(3, 4, trips, true);
    SM AT = A.transpose();

    // Check (A x, y) == (x, A^T y)
    // x in R^4, y in R^3
    const std::vector<T> x{1.0, -2.0, 0.5, 3.0};
    const std::vector<T> y{0.25, -1.0, 2.0};

    const auto Ax = A * x;          // size 3
    ASSERT_EQ(Ax.size(), y.size());

    // dot(Ax, y)
    T lhs = 0.0;
    for (size_t i = 0; i < y.size(); ++i) lhs += Ax[i] * y[i];

    // A^T y
    const auto ATy = AT * y;        // size 4
    ASSERT_EQ(ATy.size(), x.size());

    // dot(x, ATy)
    T rhs = 0.0;
    for (size_t i = 0; i < x.size(); ++i) rhs += x[i] * ATy[i];

    EXPECT_NEAR(lhs, rhs, 1e-12);
}

TEST(SparseMatrix, MultiplyAccumulateMatchesOperatorTimes)
{
    using T = float;
    using SM = SparseMatrix<T>;

    // Diagonal-ish 5x5
    std::vector<SM::Triplet> trips;
    trips.push_back({0,0, 2});
    trips.push_back({1,1, 3});
    trips.push_back({2,2, 4});
    trips.push_back({3,3, 5});
    trips.push_back({4,4, 6});
    trips.push_back({4,0, 1}); // some off-diagonal
    SM A = SM::from_triplets(5, 5, trips, true);

    std::vector<T> x{1,2,3,4,5};
    const auto y = A * x;

    std::vector<T> y_acc(5, 0.0F);
    A.multiply_accumulate(x, y_acc);

    ASSERT_EQ(y.size(), y_acc.size());
    for (size_t i = 0; i < y.size(); ++i) {
        EXPECT_NEAR(y[i], y_acc[i], 1e-6F);
    }
}

TEST(SparseMatrix, IsColumnSortedPersistsAfterEdits)
{
    using T = float;
    using SM = SparseMatrix<T>;

    SM A(6, 2);
    // Insert in shuffled order in col 1 and 0
    A.set(5, 1, 1.0F);
    A.set(0, 1, 2.0F);
    A.set(3, 1, 3.0F);
    A.add_to(2, 0, 1.0F);
    A.add_to(1, 0, 1.0F);
    A.add_to(4, 0, 1.0F);

    ASSERT_TRUE(A.is_column_sorted());

    // Add in-between entries (should stay sorted)
    A.set(2, 1, 4.0F);
    A.add_to(3, 0, 2.0F);
    ASSERT_TRUE(A.is_column_sorted());
}

TEST(SparseMatrix, DenseVsSparse_MatVec_Consistency)
{
    using T = float;
    constexpr std::size_t Rows = 5;
    constexpr std::size_t Cols = 4;

    // Build a deterministic dense matrix with some zeros and negatives.
    Matrix<T, Rows, Cols> M{};
    for (std::size_t r = 0; r < Rows; ++r) {
        for (std::size_t c = 0; c < Cols; ++c) {
            // Pattern: base value with sign flip, and introduce sparsity by zeroing some entries.
            T v = static_cast<T>((r + 1) * (c + 2));
            if ((r + c) % 2 == 0) v = -v;           // flip sign on a checkerboard
            if (((r + 2 * c) % 3) == 0) v = T(0);   // zero-out every ~3rd entry to ensure sparsity
            M[r][c] = v;
        }
    }

    // Convert to triplets (drop zeros) and build SparseMatrix.
    using SM = SparseMatrix<T>;
    std::vector<SM::Triplet> trips;
    trips.reserve(Rows * Cols);
    for (std::size_t c = 0; c < Cols; ++c) {
        for (std::size_t r = 0; r < Rows; ++r) {
            const T v = M[r][c];
            if (v != T(0)) trips.push_back({r, c, v});
        }
    }
    SM A = SM::from_triplets(Rows, Cols, trips, /*sum_duplicates*/true);
    ASSERT_TRUE(A.is_column_sorted());
    ASSERT_EQ(A.rows(), Rows);
    ASSERT_EQ(A.cols(), Cols);

    // Helper to compare dense vs sparse y = A * x
    auto check_vec = [&](const std::array<T, Cols>& x_arr) {
        // Dense y = M * x
        Vector<T, Cols> x_dense{};
        for (std::size_t i = 0; i < Cols; ++i) x_dense[i] = x_arr[i];
        const Vector<T, Rows> y_dense = M * x_dense;

        // Sparse y = A * x
        std::vector<T> x_sparse(Cols);
        for (std::size_t i = 0; i < Cols; ++i) x_sparse[i] = x_arr[i];
        const std::vector<T> y_sparse = A * x_sparse;

        ASSERT_EQ(y_sparse.size(), Rows);
        for (std::size_t r = 0; r < Rows; ++r) {
            EXPECT_NEAR(y_sparse[r], y_dense[r], 1e-5F);
        }
    };

    // Test multiple input vectors (basis, ones, ramp, mixed)
    check_vec({1, 0, 0, 0});         // e0
    check_vec({0, 1, 0, 0});         // e1
    check_vec({0, 0, 1, 0});         // e2
    check_vec({0, 0, 0, 1});         // e3
    check_vec({1, 1, 1, 1});         // all ones
    check_vec({-1, 2, -3, 4});       // mixed signs
    check_vec({0.5F, -0.25F, 1.5F, -2.0F}); // fractional
}

TEST(SparseMatrix, DenseVsSparse_MatVec_TransposeConsistency)
{
    using T = double;
    constexpr std::size_t Rows = 4;
    constexpr std::size_t Cols = 6;

    // Dense matrix with explicit structure: diagonal-ish plus some off-diagonals; enforce zeros too.
    Matrix<T, Rows, Cols> M{};
    for (std::size_t r = 0; r < Rows; ++r) {
        for (std::size_t c = 0; c < Cols; ++c) {
            T v = (r == (c % Rows)) ? T(2 + r) : T(0); // place a diagonal pattern
            if ((r + 3*c) % 5 == 0) v = -v;            // some negatives
            if (((r + c) % 4) == 0) v = T(0);          // induce sparsity
            M[r][c] = v;
        }
    }

    // Build sparse from triplets
    using SM = SparseMatrix<T>;
    std::vector<SM::Triplet> trips;
    for (std::size_t c = 0; c < Cols; ++c) {
        for (std::size_t r = 0; r < Rows; ++r) {
            const T v = M[r][c];
            if (v != T(0)) trips.push_back({r, c, v});
        }
    }
    SM A = SM::from_triplets(Rows, Cols, trips, /*sum_duplicates*/true);
    ASSERT_TRUE(A.is_column_sorted());

    // Choose y in R^Rows and x in R^Cols, test inner-product adjoint identity:
    // <A x, y> == <x, A^T y>
    const std::array<T, Cols> x_arr = {1.0, -2.0, 0.5, 3.0, -1.0, 2.5};
    const std::array<T, Rows> y_arr = {0.25, -1.0, 2.0, -0.5};

    // Dense side
    Vector<T, Cols> x_dense{};
    for (std::size_t i = 0; i < Cols; ++i) x_dense[i] = x_arr[i];
    Vector<T, Rows> y_dense{};
    for (std::size_t i = 0; i < Rows; ++i) y_dense[i] = y_arr[i];

    const Vector<T, Rows> Ax_dense = M * x_dense;                   // (Rows)
    const Matrix<T, Cols, Rows> MT = transpose(M);                  // Cols x Rows
    const Vector<T, Cols> ATy_dense = MT * y_dense;                 // (Cols)

    T lhs_dense = T(0), rhs_dense = T(0);
    for (std::size_t i = 0; i < Rows; ++i) lhs_dense += Ax_dense[i] * y_dense[i];
    for (std::size_t i = 0; i < Cols; ++i) rhs_dense += x_dense[i] * ATy_dense[i];

    // Sparse side
    const std::vector<T> x_sparse(x_arr.begin(), x_arr.end());
    const std::vector<T> y_sparse(y_arr.begin(), y_arr.end());

    const std::vector<T> Ax_sparse = A * x_sparse;                  // (Rows)
    const SM AT = A.transpose();
    const std::vector<T> ATy_sparse = AT * y_sparse;                // (Cols)

    T lhs_sparse = T(0), rhs_sparse = T(0);
    for (std::size_t i = 0; i < Rows; ++i) lhs_sparse += Ax_sparse[i] * y_sparse[i];
    for (std::size_t i = 0; i < Cols; ++i) rhs_sparse += x_sparse[i] * ATy_sparse[i];

    // Cross-check dense vs sparse results and adjoint identity
    EXPECT_NEAR(lhs_sparse, rhs_sparse, 1e-12);
    EXPECT_NEAR(lhs_dense,  rhs_dense,  1e-12);
    EXPECT_NEAR(lhs_sparse, lhs_dense,  1e-12);
    EXPECT_NEAR(rhs_sparse, rhs_dense,  1e-12);
}
