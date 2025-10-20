#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace engine::core::telemetry {

    enum class MetricKind
    {
        Counter,
        Gauge,
        Histogram,
    };

    enum class MetricUnit
    {
        None,
        Count,
        Milliseconds,
        Seconds,
        Bytes,
        Percentage,
    };

    struct Label
    {
        std::string key{};
        std::string value{};
    };

    struct MetricDescriptor
    {
        std::string name{};
        MetricKind kind{MetricKind::Gauge};
        MetricUnit unit{MetricUnit::None};
        std::string description{};
        std::vector<Label> labels{};
    };

    using MetricValue = std::variant<std::int64_t, double>;

    struct MetricSample
    {
        std::size_t descriptor_index{0U};
        MetricValue value{std::int64_t{0}};
    };

    struct MetricSet
    {
        std::vector<MetricDescriptor> descriptors{};
        std::vector<MetricSample> samples{};
    };

    [[nodiscard]] std::string_view to_string(MetricKind kind) noexcept;
    [[nodiscard]] std::string_view to_string(MetricUnit unit) noexcept;

    [[nodiscard]] bool is_integral(const MetricValue& value) noexcept;
    [[nodiscard]] double as_double(const MetricValue& value) noexcept;
    [[nodiscard]] std::int64_t as_int(const MetricValue& value) noexcept;

}  // namespace engine::core::telemetry

