#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace engine::animation::benchmarking {

struct DispatchTelemetry
{
    std::string name;
    std::string category;
    std::string queue;
    double duration_ms{0.0};
};

struct AggregatedTelemetry
{
    std::string label;
    double duration_ms{0.0};
};

[[nodiscard]] std::vector<AggregatedTelemetry> aggregate_category_totals(std::span<const DispatchTelemetry> dispatches);

[[nodiscard]] std::vector<AggregatedTelemetry> aggregate_queue_totals(std::span<const DispatchTelemetry> dispatches);

[[nodiscard]] std::string_view canonical_category(std::string_view category) noexcept;

[[nodiscard]] std::string_view canonical_queue(std::string_view queue) noexcept;

} // namespace engine::animation::benchmarking
