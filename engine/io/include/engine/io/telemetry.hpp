#pragma once

#include "engine/io/errors.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace engine::io
{
    enum class GeometryIoOperation : std::uint8_t
    {
        detect = 0U,
        read_mesh,
        write_mesh,
        read_point_cloud,
        write_point_cloud,
        read_graph,
        write_graph,
        count
    };

    [[nodiscard]] constexpr std::size_t geometry_io_operation_index(GeometryIoOperation operation) noexcept
    {
        return static_cast<std::size_t>(operation);
    }

    [[nodiscard]] constexpr std::size_t geometry_io_operation_count() noexcept
    {
        return geometry_io_operation_index(GeometryIoOperation::count);
    }

    [[nodiscard]] std::string_view to_string(GeometryIoOperation operation) noexcept;

    [[nodiscard]] constexpr std::size_t geometry_io_error_index(GeometryIoError error) noexcept
    {
        return static_cast<std::size_t>(error) - 1U;
    }

    [[nodiscard]] constexpr std::size_t geometry_io_error_count() noexcept
    {
        return geometry_io_error_index(GeometryIoError::plugin_missing) + 1U;
    }

    struct GeometryIoOperationSnapshot
    {
        std::uint64_t attempts{0};
        std::uint64_t successes{0};
        std::array<std::uint64_t, geometry_io_error_count()> failures_by_error{};
    };

    struct GeometryIoTelemetrySnapshot
    {
        std::array<GeometryIoOperationSnapshot, geometry_io_operation_count()> operations{};

        [[nodiscard]] const GeometryIoOperationSnapshot& operation(GeometryIoOperation op) const noexcept
        {
            return operations[geometry_io_operation_index(op)];
        }
    };

    class GeometryIoTelemetry
    {
    public:
        static GeometryIoTelemetry& instance() noexcept;

        void record_attempt(GeometryIoOperation operation) noexcept;
        void record_success(GeometryIoOperation operation) noexcept;
        void record_failure(GeometryIoOperation operation, GeometryIoError error) noexcept;

        [[nodiscard]] GeometryIoTelemetrySnapshot snapshot() const noexcept;

        void reset_for_testing() noexcept;

    private:
        GeometryIoTelemetry() = default;

        using FailureArray = std::array<std::atomic<std::uint64_t>, geometry_io_error_count()>;

        std::array<std::atomic<std::uint64_t>, geometry_io_operation_count()> attempts_{};
        std::array<std::atomic<std::uint64_t>, geometry_io_operation_count()> successes_{};
        std::array<FailureArray, geometry_io_operation_count()> failures_{};
    };

    [[nodiscard]] constexpr std::array<GeometryIoError, geometry_io_error_count()> geometry_io_error_codes() noexcept
    {
        return {
            GeometryIoError::file_not_found,
            GeometryIoError::io_failure,
            GeometryIoError::invalid_argument,
            GeometryIoError::unsupported_format,
            GeometryIoError::plugin_missing
        };
    }
} // namespace engine::io