#include "engine/core/telemetry/schema.hpp"

#include <cstdint>

#include <gtest/gtest.h>

namespace telemetry = engine::core::telemetry;

TEST(TelemetrySchema, MetricKindToString)
{
    EXPECT_EQ("counter", telemetry::to_string(telemetry::MetricKind::Counter));
    EXPECT_EQ("gauge", telemetry::to_string(telemetry::MetricKind::Gauge));
    EXPECT_EQ("histogram", telemetry::to_string(telemetry::MetricKind::Histogram));
}

TEST(TelemetrySchema, MetricUnitToString)
{
    EXPECT_EQ("none", telemetry::to_string(telemetry::MetricUnit::None));
    EXPECT_EQ("count", telemetry::to_string(telemetry::MetricUnit::Count));
    EXPECT_EQ("milliseconds", telemetry::to_string(telemetry::MetricUnit::Milliseconds));
    EXPECT_EQ("seconds", telemetry::to_string(telemetry::MetricUnit::Seconds));
    EXPECT_EQ("bytes", telemetry::to_string(telemetry::MetricUnit::Bytes));
    EXPECT_EQ("percentage", telemetry::to_string(telemetry::MetricUnit::Percentage));
}

TEST(TelemetrySchema, MetricValueConversions)
{
    telemetry::MetricValue integral{std::int64_t{42}};
    EXPECT_TRUE(telemetry::is_integral(integral));
    EXPECT_EQ(42, telemetry::as_int(integral));
    EXPECT_DOUBLE_EQ(42.0, telemetry::as_double(integral));

    telemetry::MetricValue floating{3.5};
    EXPECT_FALSE(telemetry::is_integral(floating));
    EXPECT_EQ(3, telemetry::as_int(floating));
    EXPECT_DOUBLE_EQ(3.5, telemetry::as_double(floating));
}