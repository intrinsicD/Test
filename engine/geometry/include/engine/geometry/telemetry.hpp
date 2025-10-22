#pragma once

#include "engine/geometry/api.hpp"

#include <array>
#include <atomic>
#include <cstdint>

namespace engine::geometry
{
    enum class GeometrySpatialQueryOperation : std::uint8_t
    {
        octree_build = 0U,
        octree_query_aabb,
        octree_query_sphere,
        octree_query_ray,
        octree_query_segment,
        octree_query_knn,
        octree_query_nearest,
        count
    };

    [[nodiscard]] constexpr std::size_t geometry_spatial_query_operation_index(GeometrySpatialQueryOperation operation) noexcept
    {
        return static_cast<std::size_t>(operation);
    }

    [[nodiscard]] constexpr std::size_t geometry_spatial_query_operation_count() noexcept
    {
        return geometry_spatial_query_operation_index(GeometrySpatialQueryOperation::count);
    }

    struct GeometrySpatialQueryOperationSnapshot
    {
        std::uint64_t invocations{0};
        std::uint64_t total_results{0};
        std::uint64_t last_results{0};
        std::uint64_t max_results{0};
    };

    struct GeometrySpatialTelemetrySnapshot
    {
        std::array<GeometrySpatialQueryOperationSnapshot, geometry_spatial_query_operation_count()> operations{};

        [[nodiscard]] const GeometrySpatialQueryOperationSnapshot& operation(GeometrySpatialQueryOperation op) const noexcept
        {
            return operations[geometry_spatial_query_operation_index(op)];
        }
    };

    class ENGINE_GEOMETRY_API GeometrySpatialTelemetry
    {
    public:
        static GeometrySpatialTelemetry& instance() noexcept;

        void record_invocation(GeometrySpatialQueryOperation operation, std::uint64_t result_count) noexcept;

        [[nodiscard]] GeometrySpatialTelemetrySnapshot snapshot() const noexcept;

        void reset_for_testing() noexcept;

    private:
        GeometrySpatialTelemetry() = default;

        using MetricArray = std::array<std::atomic<std::uint64_t>, geometry_spatial_query_operation_count()>;

        MetricArray invocations_{};
        MetricArray total_results_{};
        MetricArray last_results_{};
        MetricArray max_results_{};
    };
} // namespace engine::geometry

