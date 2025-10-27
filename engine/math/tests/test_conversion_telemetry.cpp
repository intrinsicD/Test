#include <gtest/gtest.h>

#include "engine/math/matrix.hpp"
#include "engine/math/telemetry/conversion_telemetry.hpp"
#include "engine/math/vector.hpp"

namespace math = engine::math;
namespace telemetry = engine::math::telemetry;

TEST(ConversionTelemetry, RecordsVectorMetrics)
{
    auto& telemetry_system = telemetry::ConversionTelemetry::instance();
    telemetry_system.reset_for_testing();

    math::Vector < double, 3 > original{1.0, 2.0, -4.0};
    math::Vector < double, 3 > converted = original;
    converted[1] = 2.0005; // 5e-4 absolute error on component 1.

    telemetry::RecordVectorRoundTrip(original, converted);

    const telemetry::ConversionTelemetrySnapshot snapshot = telemetry_system.snapshot();
    ASSERT_EQ(snapshot.vectors.size(), 1U);

    const telemetry::ConversionVectorEntry& entry = snapshot.vectors.front();
    EXPECT_EQ(entry.dimension, 3U);
    EXPECT_EQ(entry.sample_count, 1U);
    EXPECT_NEAR(entry.max_abs_error, 5e-4, 1e-9);
    EXPECT_NEAR(entry.last_abs_error, 5e-4, 1e-9);
    EXPECT_NEAR(entry.mean_abs_error, 5e-4, 1e-9);
    EXPECT_NEAR(entry.max_relative_error, 5e-4 / 2.0, 1e-9);
    EXPECT_NEAR(entry.last_relative_error, 5e-4 / 2.0, 1e-9);
    EXPECT_NEAR(entry.mean_relative_error, 5e-4 / 2.0, 1e-9);
}

TEST(ConversionTelemetry, AggregatesVectorSamples)
{
    auto& telemetry_system = telemetry::ConversionTelemetry::instance();
    telemetry_system.reset_for_testing();

    math::Vector < float, 2 > original{1.0F, -3.0F};

    math::Vector < float, 2 > converted_first = original;
    converted_first[0] = 1.05F; // 0.05 absolute, relative 0.05
    telemetry::RecordVectorRoundTrip(original, converted_first);

    math::Vector < float, 2 > converted_second = original;
    converted_second[1] = -2.4F; // 0.6 absolute, relative 0.2
    telemetry::RecordVectorRoundTrip(original, converted_second);

    const telemetry::ConversionTelemetrySnapshot snapshot = telemetry_system.snapshot();
    ASSERT_EQ(snapshot.vectors.size(), 1U);

    const telemetry::ConversionVectorEntry& entry = snapshot.vectors.front();
    EXPECT_EQ(entry.sample_count, 2U);
    EXPECT_NEAR(entry.max_abs_error, 0.6, 1e-6);
    EXPECT_NEAR(entry.max_relative_error, 0.2, 1e-6);
    EXPECT_NEAR(entry.last_abs_error, 0.6, 1e-6);
    EXPECT_NEAR(entry.last_relative_error, 0.2, 1e-6);
    EXPECT_NEAR(entry.mean_abs_error, (0.05 + 0.6) / 2.0, 1e-6);
    EXPECT_NEAR(entry.mean_relative_error, (0.05 + 0.2) / 2.0, 1e-6);
}

TEST(ConversionTelemetry, RecordsMatrixMetrics)
{
    auto& telemetry_system = telemetry::ConversionTelemetry::instance();
    telemetry_system.reset_for_testing();

    math::Matrix < double, 3, 3 > original = math::identity_matrix<double, 3>();
    math::Matrix < double, 3, 3 > converted = original;
    converted[0][0] = 0.9; // 0.1 absolute error relative to 1.0.

    telemetry::RecordMatrixRoundTrip(original, converted);

    const telemetry::ConversionTelemetrySnapshot snapshot = telemetry_system.snapshot();
    ASSERT_EQ(snapshot.matrices.size(), 1U);

    const telemetry::ConversionMatrixEntry& entry = snapshot.matrices.front();
    EXPECT_EQ(entry.rows, 3U);
    EXPECT_EQ(entry.columns, 3U);
    EXPECT_EQ(entry.sample_count, 1U);
    EXPECT_NEAR(entry.max_abs_error, 0.1, 1e-9);
    EXPECT_NEAR(entry.last_abs_error, 0.1, 1e-9);
    EXPECT_NEAR(entry.mean_abs_error, 0.1, 1e-9);
    EXPECT_NEAR(entry.max_relative_error, 0.1, 1e-9);
    EXPECT_NEAR(entry.last_relative_error, 0.1, 1e-9);
    EXPECT_NEAR(entry.mean_relative_error, 0.1, 1e-9);
}

TEST(ConversionTelemetry, MatrixAggregationMaintainsMax)
{
    auto& telemetry_system = telemetry::ConversionTelemetry::instance();
    telemetry_system.reset_for_testing();

    math::Matrix < float, 2, 2 > original = math::identity_matrix<float, 2>();

    math::Matrix < float, 2, 2 > converted_first = original;
    converted_first[0][1] = 0.02F; // small error
    telemetry::RecordMatrixRoundTrip(original, converted_first);

    math::Matrix < float, 2, 2 > converted_second = original;
    converted_second[1][0] = -0.5F; // larger error
    telemetry::RecordMatrixRoundTrip(original, converted_second);

    const telemetry::ConversionTelemetrySnapshot snapshot = telemetry_system.snapshot();
    ASSERT_EQ(snapshot.matrices.size(), 1U);

    const telemetry::ConversionMatrixEntry& entry = snapshot.matrices.front();
    EXPECT_EQ(entry.sample_count, 2U);
    EXPECT_NEAR(entry.max_abs_error, 0.5, 1e-6);
    EXPECT_NEAR(entry.last_abs_error, 0.5, 1e-6);
    EXPECT_NEAR(entry.mean_abs_error, (0.02 + 0.5) / 2.0, 1e-6);
    EXPECT_NEAR(entry.max_relative_error, 0.5, 1e-6);
    EXPECT_NEAR(entry.mean_relative_error, (0.02 + 0.5) / 2.0, 1e-6);
}

TEST(ConversionTelemetry, HandlesZeroOriginalValues)
{
    auto& telemetry_system = telemetry::ConversionTelemetry::instance();
    telemetry_system.reset_for_testing();

    math::Matrix < double, 2, 2 > original{}; // all zeros
    math::Matrix < double, 2, 2 > converted = original;
    converted[0][1] = 0.25;

    telemetry::RecordMatrixRoundTrip(original, converted);

    const telemetry::ConversionTelemetrySnapshot snapshot = telemetry_system.snapshot();
    ASSERT_EQ(snapshot.matrices.size(), 1U);

    const telemetry::ConversionMatrixEntry& entry = snapshot.matrices.front();
    EXPECT_EQ(entry.sample_count, 1U);
    EXPECT_NEAR(entry.max_abs_error, 0.25, 1e-9);
    EXPECT_GT(entry.max_relative_error, 1e6); // Fallback denominator keeps error finite but large.
}