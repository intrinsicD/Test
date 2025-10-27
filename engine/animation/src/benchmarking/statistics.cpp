#include "engine/animation/benchmarking/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace engine::animation::benchmarking
{
    namespace
    {
        [[nodiscard]] double compute_mean(std::span<const double> values) noexcept
        {
            if (values.empty())
            {
                return 0.0;
            }

            const double sum = std::accumulate(values.begin(), values.end(), 0.0);
            return sum / static_cast<double>(values.size());
        }

        [[nodiscard]] double compute_stddev(std::span<const double> values, double mean) noexcept
        {
            if (values.size() <= 1U || mean == 0.0)
            {
                return 0.0;
            }

            double variance = 0.0;
            for (const double value : values)
            {
                const double delta = value - mean;
                variance += delta * delta;
            }
            variance /= static_cast<double>(values.size());
            return std::sqrt(variance);
        }
    } // namespace

    FrameTimingSummary compute_frame_timing_summary(std::span<const double> frame_durations_ms) noexcept
    {
        FrameTimingSummary summary{};
        summary.samples = frame_durations_ms.size();

        if (frame_durations_ms.empty())
        {
            return summary;
        }

        summary.total_ms = std::accumulate(frame_durations_ms.begin(), frame_durations_ms.end(), 0.0);
        summary.min_ms = *std::min_element(frame_durations_ms.begin(), frame_durations_ms.end());
        summary.max_ms = *std::max_element(frame_durations_ms.begin(), frame_durations_ms.end());
        summary.mean_ms = compute_mean(frame_durations_ms);
        summary.stddev_ms = compute_stddev(frame_durations_ms, summary.mean_ms);
        return summary;
    }
} // namespace engine::animation::benchmarking