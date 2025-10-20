#include "engine/core/telemetry/schema.hpp"

namespace engine::core::telemetry {

    std::string_view to_string(MetricKind kind) noexcept
    {
        switch (kind)
        {
        case MetricKind::Counter:
            return "counter";
        case MetricKind::Gauge:
            return "gauge";
        case MetricKind::Histogram:
            return "histogram";
        }

        return "unknown";
    }

    std::string_view to_string(MetricUnit unit) noexcept
    {
        switch (unit)
        {
        case MetricUnit::None:
            return "none";
        case MetricUnit::Count:
            return "count";
        case MetricUnit::Milliseconds:
            return "milliseconds";
        case MetricUnit::Seconds:
            return "seconds";
        case MetricUnit::Bytes:
            return "bytes";
        case MetricUnit::Percentage:
            return "percentage";
        }

        return "unknown";
    }

    bool is_integral(const MetricValue& value) noexcept
    {
        return std::holds_alternative<std::int64_t>(value);
    }

    double as_double(const MetricValue& value) noexcept
    {
        if (std::holds_alternative<double>(value))
        {
            return std::get<double>(value);
        }
        return static_cast<double>(std::get<std::int64_t>(value));
    }

    std::int64_t as_int(const MetricValue& value) noexcept
    {
        if (std::holds_alternative<std::int64_t>(value))
        {
            return std::get<std::int64_t>(value);
        }
        return static_cast<std::int64_t>(std::get<double>(value));
    }

}  // namespace engine::core::telemetry

