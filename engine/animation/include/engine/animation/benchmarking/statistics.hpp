#pragma once

#include <cstddef>
#include <span>

namespace engine::animation::benchmarking
{
    struct FrameTimingSummary
    {
        std::size_t samples{0U};
        double total_ms{0.0};
        double mean_ms{0.0};
        double min_ms{0.0};
        double max_ms{0.0};
        double stddev_ms{0.0};
    };

    [[nodiscard]] FrameTimingSummary compute_frame_timing_summary(std::span<const double> frame_durations_ms) noexcept;
} // namespace engine::animation::benchmarking