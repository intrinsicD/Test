#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/export.hpp"
#include "engine/geometry/remesh/remesh.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

namespace engine::geometry
{
    [[nodiscard]] constexpr std::size_t remesh_telemetry_mode_index(RemeshingMode mode) noexcept
    {
        return static_cast<std::size_t>(mode);
    }

    [[nodiscard]] constexpr std::size_t remesh_telemetry_mode_count() noexcept
    {
        return static_cast<std::size_t>(RemeshingMode::kAdaptive) + 1U;
    }

    struct RemeshTelemetryOperationSnapshot
    {
        std::uint64_t invocations{0};
        std::uint64_t total_iterations{0};
        std::uint64_t last_iterations{0};
        std::uint64_t total_splits{0};
        std::uint64_t last_splits{0};
        std::uint64_t total_collapses{0};
        std::uint64_t last_collapses{0};
        std::uint64_t last_vertex_count{0};
        std::uint64_t max_vertex_count{0};
        double last_duration_ms{0.0};
        double max_duration_ms{0.0};
        std::uint64_t surface_deviation_invocations{0};
        double last_max_surface_deviation{0.0};
        double max_surface_deviation{0.0};
        double average_max_surface_deviation{0.0};
        double last_mean_surface_deviation{0.0};
        double average_mean_surface_deviation{0.0};
        double last_rms_surface_deviation{0.0};
        double average_rms_surface_deviation{0.0};
        std::uint64_t last_surface_deviation_sample_count{0};
        std::uint64_t total_surface_deviation_sample_count{0};
        std::string last_job_label{};
    };

    struct RemeshTelemetrySnapshot
    {
        std::array<RemeshTelemetryOperationSnapshot, remesh_telemetry_mode_count()> operations{};

        [[nodiscard]] const RemeshTelemetryOperationSnapshot& operation(RemeshingMode mode) const noexcept
        {
            return operations[remesh_telemetry_mode_index(mode)];
        }
    };

    class ENGINE_GEOMETRY_API RemeshTelemetry
    {
    public:
        static RemeshTelemetry& instance() noexcept;

        void record_invocation(RemeshingMode mode,
                               std::uint32_t iterations,
                               std::uint64_t split_count,
                               std::uint64_t collapse_count,
                               std::uint64_t vertex_count,
                               double duration_ms,
                               const RemeshStatistics& statistics,
                               std::uint64_t surface_deviation_sample_count,
                               const std::optional<std::string>& job_label) noexcept;

        [[nodiscard]] RemeshTelemetrySnapshot snapshot() const noexcept;

        void reset_for_testing() noexcept;

    private:
        RemeshTelemetry() = default;

        using CounterArray = std::array<std::atomic<std::uint64_t>, remesh_telemetry_mode_count()>;
        using GaugeArray = std::array<std::atomic<double>, remesh_telemetry_mode_count()>;

        CounterArray invocations_{};
        CounterArray total_iterations_{};
        CounterArray last_iterations_{};
        CounterArray total_splits_{};
        CounterArray last_splits_{};
        CounterArray total_collapses_{};
        CounterArray last_collapses_{};
        CounterArray last_vertex_count_{};
        CounterArray max_vertex_count_{};
        GaugeArray last_duration_ms_{};
        GaugeArray max_duration_ms_{};
        CounterArray surface_deviation_invocations_{};
        GaugeArray last_max_surface_deviation_{};
        GaugeArray max_surface_deviation_{};
        GaugeArray total_max_surface_deviation_{};
        GaugeArray last_mean_surface_deviation_{};
        GaugeArray total_weighted_mean_surface_deviation_{};
        GaugeArray last_rms_surface_deviation_{};
        GaugeArray total_weighted_squared_surface_deviation_{};
        CounterArray last_surface_deviation_sample_count_{};
        CounterArray total_surface_deviation_sample_count_{};

        mutable std::mutex job_label_mutex_{};
        std::array<std::string, remesh_telemetry_mode_count()> last_job_labels_{};
    };
} // namespace engine::geometry