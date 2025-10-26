#include "engine/geometry/remesh/telemetry.hpp"

#include <algorithm>
#include <cmath>

namespace engine::geometry
{
    namespace
    {
        template <typename T>
        void update_max(std::atomic<T>& target, T value) noexcept
        {
            T observed = target.load(std::memory_order_relaxed);
            while (observed < value &&
                   !target.compare_exchange_weak(
                       observed,
                       value,
                       std::memory_order_relaxed,
                       std::memory_order_relaxed))
            {
            }
        }
    } // namespace

    RemeshTelemetry& RemeshTelemetry::instance() noexcept
    {
        static RemeshTelemetry telemetry{};
        return telemetry;
    }

    void RemeshTelemetry::record_invocation(RemeshingMode mode,
                                            std::uint32_t iterations,
                                            std::uint64_t split_count,
                                            std::uint64_t collapse_count,
                                            std::uint64_t vertex_count,
                                            double duration_ms,
                                            const RemeshStatistics& statistics,
                                            std::uint64_t surface_deviation_sample_count,
                                            const std::optional<std::string>& job_label) noexcept
    {
        const auto index = remesh_telemetry_mode_index(mode);

        invocations_[index].fetch_add(1U, std::memory_order_relaxed);
        total_iterations_[index].fetch_add(iterations, std::memory_order_relaxed);
        last_iterations_[index].store(iterations, std::memory_order_relaxed);

        total_splits_[index].fetch_add(split_count, std::memory_order_relaxed);
        last_splits_[index].store(split_count, std::memory_order_relaxed);

        total_collapses_[index].fetch_add(collapse_count, std::memory_order_relaxed);
        last_collapses_[index].store(collapse_count, std::memory_order_relaxed);

        last_vertex_count_[index].store(vertex_count, std::memory_order_relaxed);
        update_max(max_vertex_count_[index], vertex_count);

        const double clamped_duration = std::max(duration_ms, 0.0);
        last_duration_ms_[index].store(clamped_duration, std::memory_order_relaxed);
        update_max(max_duration_ms_[index], clamped_duration);

        last_surface_deviation_sample_count_[index].store(surface_deviation_sample_count, std::memory_order_relaxed);
        if (surface_deviation_sample_count > 0U)
        {
            const auto sanitize = [](float value) -> double {
                if (!std::isfinite(value))
                {
                    return 0.0;
                }
                return static_cast<double>(std::max(value, 0.0f));
            };

            const double sample_count = static_cast<double>(surface_deviation_sample_count);

            const double max_surface_deviation = sanitize(statistics.max_surface_deviation);
            last_max_surface_deviation_[index].store(max_surface_deviation, std::memory_order_relaxed);
            update_max(max_surface_deviation_[index], max_surface_deviation);
            total_max_surface_deviation_[index].fetch_add(max_surface_deviation, std::memory_order_relaxed);
            surface_deviation_invocations_[index].fetch_add(1U, std::memory_order_relaxed);

            const double mean_surface_deviation = sanitize(statistics.mean_surface_deviation);
            last_mean_surface_deviation_[index].store(mean_surface_deviation, std::memory_order_relaxed);
            total_weighted_mean_surface_deviation_[index].fetch_add(
                mean_surface_deviation * sample_count,
                std::memory_order_relaxed);

            const double rms_surface_deviation = sanitize(statistics.rms_surface_deviation);
            last_rms_surface_deviation_[index].store(rms_surface_deviation, std::memory_order_relaxed);
            const double weighted_squared = rms_surface_deviation * rms_surface_deviation * sample_count;
            total_weighted_squared_surface_deviation_[index].fetch_add(weighted_squared, std::memory_order_relaxed);

            total_surface_deviation_sample_count_[index].fetch_add(
                surface_deviation_sample_count,
                std::memory_order_relaxed);
        }
        else
        {
            last_max_surface_deviation_[index].store(0.0, std::memory_order_relaxed);
            last_mean_surface_deviation_[index].store(0.0, std::memory_order_relaxed);
            last_rms_surface_deviation_[index].store(0.0, std::memory_order_relaxed);
        }

        {
            std::scoped_lock lock(job_label_mutex_);
            if (job_label.has_value())
            {
                last_job_labels_[index] = job_label.value();
            }
            else
            {
                last_job_labels_[index].clear();
            }
        }
    }

    RemeshTelemetrySnapshot RemeshTelemetry::snapshot() const noexcept
    {
        RemeshTelemetrySnapshot snapshot{};

        for (std::size_t i = 0; i < remesh_telemetry_mode_count(); ++i)
        {
            auto& operation = snapshot.operations[i];
            operation.invocations = invocations_[i].load(std::memory_order_relaxed);
            operation.total_iterations = total_iterations_[i].load(std::memory_order_relaxed);
            operation.last_iterations = last_iterations_[i].load(std::memory_order_relaxed);
            operation.total_splits = total_splits_[i].load(std::memory_order_relaxed);
            operation.last_splits = last_splits_[i].load(std::memory_order_relaxed);
            operation.total_collapses = total_collapses_[i].load(std::memory_order_relaxed);
            operation.last_collapses = last_collapses_[i].load(std::memory_order_relaxed);
            operation.last_vertex_count = last_vertex_count_[i].load(std::memory_order_relaxed);
            operation.max_vertex_count = max_vertex_count_[i].load(std::memory_order_relaxed);
            operation.last_duration_ms = last_duration_ms_[i].load(std::memory_order_relaxed);
            operation.max_duration_ms = max_duration_ms_[i].load(std::memory_order_relaxed);
            operation.surface_deviation_invocations =
                surface_deviation_invocations_[i].load(std::memory_order_relaxed);
            operation.last_max_surface_deviation =
                last_max_surface_deviation_[i].load(std::memory_order_relaxed);
            operation.max_surface_deviation = max_surface_deviation_[i].load(std::memory_order_relaxed);
            const double total_max = total_max_surface_deviation_[i].load(std::memory_order_relaxed);
            if (operation.surface_deviation_invocations > 0U)
            {
                operation.average_max_surface_deviation =
                    total_max / static_cast<double>(operation.surface_deviation_invocations);
            }
            else
            {
                operation.average_max_surface_deviation = 0.0;
            }

            operation.last_mean_surface_deviation =
                last_mean_surface_deviation_[i].load(std::memory_order_relaxed);
            operation.last_rms_surface_deviation =
                last_rms_surface_deviation_[i].load(std::memory_order_relaxed);
            operation.last_surface_deviation_sample_count =
                last_surface_deviation_sample_count_[i].load(std::memory_order_relaxed);
            operation.total_surface_deviation_sample_count =
                total_surface_deviation_sample_count_[i].load(std::memory_order_relaxed);

            if (operation.total_surface_deviation_sample_count > 0U)
            {
                const double total_weighted_mean =
                    total_weighted_mean_surface_deviation_[i].load(std::memory_order_relaxed);
                operation.average_mean_surface_deviation =
                    total_weighted_mean / static_cast<double>(operation.total_surface_deviation_sample_count);

                const double total_weighted_squared =
                    total_weighted_squared_surface_deviation_[i].load(std::memory_order_relaxed);
                const double mean_squared =
                    total_weighted_squared / static_cast<double>(operation.total_surface_deviation_sample_count);
                operation.average_rms_surface_deviation = mean_squared > 0.0 ? std::sqrt(mean_squared) : 0.0;
            }
            else
            {
                operation.average_mean_surface_deviation = 0.0;
                operation.average_rms_surface_deviation = 0.0;
            }
        }

        {
            std::scoped_lock lock(job_label_mutex_);
            for (std::size_t i = 0; i < remesh_telemetry_mode_count(); ++i)
            {
                snapshot.operations[i].last_job_label = last_job_labels_[i];
            }
        }

        return snapshot;
    }

    void RemeshTelemetry::reset_for_testing() noexcept
    {
        for (std::size_t i = 0; i < remesh_telemetry_mode_count(); ++i)
        {
            invocations_[i].store(0U, std::memory_order_relaxed);
            total_iterations_[i].store(0U, std::memory_order_relaxed);
            last_iterations_[i].store(0U, std::memory_order_relaxed);
            total_splits_[i].store(0U, std::memory_order_relaxed);
            last_splits_[i].store(0U, std::memory_order_relaxed);
            total_collapses_[i].store(0U, std::memory_order_relaxed);
            last_collapses_[i].store(0U, std::memory_order_relaxed);
            last_vertex_count_[i].store(0U, std::memory_order_relaxed);
            max_vertex_count_[i].store(0U, std::memory_order_relaxed);
            last_duration_ms_[i].store(0.0, std::memory_order_relaxed);
            max_duration_ms_[i].store(0.0, std::memory_order_relaxed);
            surface_deviation_invocations_[i].store(0U, std::memory_order_relaxed);
            last_max_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            max_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            total_max_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            last_mean_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            total_weighted_mean_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            last_rms_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            total_weighted_squared_surface_deviation_[i].store(0.0, std::memory_order_relaxed);
            last_surface_deviation_sample_count_[i].store(0U, std::memory_order_relaxed);
            total_surface_deviation_sample_count_[i].store(0U, std::memory_order_relaxed);
        }

        std::scoped_lock lock(job_label_mutex_);
        for (auto& label : last_job_labels_)
        {
            label.clear();
        }
    }
} // namespace engine::geometry

