#include "engine/io/telemetry.hpp"

#include <array>

namespace engine::io
{
    namespace
    {
        constexpr std::array<std::string_view, geometry_io_operation_count()> kOperationNames{
            "detect_geometry_file",
            "read_mesh",
            "write_mesh",
            "read_point_cloud",
            "write_point_cloud",
            "read_graph",
            "write_graph"};
    } // namespace

    GeometryIoTelemetry& GeometryIoTelemetry::instance() noexcept
    {
        static GeometryIoTelemetry telemetry;
        return telemetry;
    }

    void GeometryIoTelemetry::record_attempt(GeometryIoOperation operation) noexcept
    {
        attempts_[geometry_io_operation_index(operation)].fetch_add(1U, std::memory_order_relaxed);
    }

    void GeometryIoTelemetry::record_success(GeometryIoOperation operation) noexcept
    {
        successes_[geometry_io_operation_index(operation)].fetch_add(1U, std::memory_order_relaxed);
    }

    void GeometryIoTelemetry::record_failure(GeometryIoOperation operation, GeometryIoError error) noexcept
    {
        const auto op_index = geometry_io_operation_index(operation);
        const auto error_index = geometry_io_error_index(error);
        failures_[op_index][error_index].fetch_add(1U, std::memory_order_relaxed);
    }

    GeometryIoTelemetrySnapshot GeometryIoTelemetry::snapshot() const noexcept
    {
        GeometryIoTelemetrySnapshot snapshot{};
        for (std::size_t op_index = 0; op_index < geometry_io_operation_count(); ++op_index)
        {
            GeometryIoOperationSnapshot entry{};
            entry.attempts = attempts_[op_index].load(std::memory_order_relaxed);
            entry.successes = successes_[op_index].load(std::memory_order_relaxed);
            for (std::size_t error_index = 0; error_index < geometry_io_error_count(); ++error_index)
            {
                entry.failures_by_error[error_index] =
                    failures_[op_index][error_index].load(std::memory_order_relaxed);
            }
            snapshot.operations[op_index] = entry;
        }
        return snapshot;
    }

    void GeometryIoTelemetry::reset_for_testing() noexcept
    {
        for (auto& value : attempts_)
        {
            value.store(0U, std::memory_order_relaxed);
        }
        for (auto& value : successes_)
        {
            value.store(0U, std::memory_order_relaxed);
        }
        for (auto& failure_array : failures_)
        {
            for (auto& value : failure_array)
            {
                value.store(0U, std::memory_order_relaxed);
            }
        }
    }

    std::string_view to_string(GeometryIoOperation operation) noexcept
    {
        const auto index = geometry_io_operation_index(operation);
        if (index < kOperationNames.size())
        {
            return kOperationNames[index];
        }
        return "unknown";
    }
} // namespace engine::io

