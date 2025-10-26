#include "engine/geometry/remesh/telemetry.hpp"

#include <algorithm>

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
        }

        std::scoped_lock lock(job_label_mutex_);
        for (auto& label : last_job_labels_)
        {
            label.clear();
        }
    }
} // namespace engine::geometry

