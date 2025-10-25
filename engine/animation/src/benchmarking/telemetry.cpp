#include "engine/animation/benchmarking/telemetry.hpp"

#include <algorithm>
#include <unordered_map>

namespace engine::animation::benchmarking {
namespace {
    using Map = std::unordered_map<std::string, double>;

    [[nodiscard]] std::vector<AggregatedTelemetry> to_sorted_vector(const Map& map)
    {
        std::vector<AggregatedTelemetry> totals;
        totals.reserve(map.size());
        for (const auto& [label, duration] : map)
        {
            totals.push_back(AggregatedTelemetry{label, duration});
        }

        std::sort(totals.begin(), totals.end(), [](const AggregatedTelemetry& lhs, const AggregatedTelemetry& rhs) {
            return lhs.label < rhs.label;
        });

        return totals;
    }

    [[nodiscard]] Map aggregate(std::span<const DispatchTelemetry> dispatches, auto&& key_selector)
    {
        Map totals;
        for (const auto& dispatch : dispatches)
        {
            const std::string key = std::string(key_selector(dispatch));
            totals[key] += dispatch.duration_ms;
        }
        return totals;
    }

    [[nodiscard]] std::string_view normalise(std::string_view value, std::string_view fallback) noexcept
    {
        if (value.empty())
        {
            return fallback;
        }
        return value;
    }
} // namespace

std::vector<AggregatedTelemetry> aggregate_category_totals(std::span<const DispatchTelemetry> dispatches)
{
    const auto map = aggregate(dispatches, [](const DispatchTelemetry& dispatch) noexcept {
        return canonical_category(dispatch.category);
    });
    return to_sorted_vector(map);
}

std::vector<AggregatedTelemetry> aggregate_queue_totals(std::span<const DispatchTelemetry> dispatches)
{
    const auto map = aggregate(dispatches, [](const DispatchTelemetry& dispatch) noexcept {
        return canonical_queue(dispatch.queue);
    });
    return to_sorted_vector(map);
}

std::string_view canonical_category(std::string_view category) noexcept
{
    return normalise(category, "uncategorised");
}

std::string_view canonical_queue(std::string_view queue) noexcept
{
    return normalise(queue, "unspecified");
}

} // namespace engine::animation::benchmarking
