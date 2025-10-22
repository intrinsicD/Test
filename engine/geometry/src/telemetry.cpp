#include "engine/geometry/telemetry.hpp"

namespace engine::geometry
{
    GeometrySpatialTelemetry& GeometrySpatialTelemetry::instance() noexcept
    {
        static GeometrySpatialTelemetry telemetry;
        return telemetry;
    }

    void GeometrySpatialTelemetry::record_invocation(GeometrySpatialQueryOperation operation, std::uint64_t result_count) noexcept
    {
        const auto index = geometry_spatial_query_operation_index(operation);

        invocations_[index].fetch_add(1U, std::memory_order_relaxed);
        total_results_[index].fetch_add(result_count, std::memory_order_relaxed);
        last_results_[index].store(result_count, std::memory_order_relaxed);
        auto observed = max_results_[index].load(std::memory_order_relaxed);
        while (observed < result_count &&
               !max_results_[index].compare_exchange_weak(
                   observed,
                   result_count,
                   std::memory_order_relaxed,
                   std::memory_order_relaxed))
        {
        }
    }

    GeometrySpatialTelemetrySnapshot GeometrySpatialTelemetry::snapshot() const noexcept
    {
        GeometrySpatialTelemetrySnapshot snapshot{};

        for (std::size_t i = 0; i < geometry_spatial_query_operation_count(); ++i)
        {
            auto& operation_snapshot = snapshot.operations[i];
            operation_snapshot.invocations = invocations_[i].load(std::memory_order_relaxed);
            operation_snapshot.total_results = total_results_[i].load(std::memory_order_relaxed);
            operation_snapshot.last_results = last_results_[i].load(std::memory_order_relaxed);
            operation_snapshot.max_results = max_results_[i].load(std::memory_order_relaxed);
        }

        return snapshot;
    }

    void GeometrySpatialTelemetry::reset_for_testing() noexcept
    {
        for (std::size_t i = 0; i < geometry_spatial_query_operation_count(); ++i)
        {
            invocations_[i].store(0U, std::memory_order_relaxed);
            total_results_[i].store(0U, std::memory_order_relaxed);
            last_results_[i].store(0U, std::memory_order_relaxed);
            max_results_[i].store(0U, std::memory_order_relaxed);
        }
    }
} // namespace engine::geometry

