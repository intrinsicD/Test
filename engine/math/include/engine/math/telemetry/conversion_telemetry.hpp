#pragma once

#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace engine::math::telemetry
{
    namespace detail
    {
        // Minimum magnitude used to avoid division by zero in relative error calculations.
        inline constexpr double kMinimumMagnitude = 1e-12;

        [[nodiscard]] inline double safe_denominator(double value) noexcept
        {
            const double magnitude = std::abs(value);
            return magnitude < kMinimumMagnitude ? kMinimumMagnitude : magnitude;
        }

        [[nodiscard]] inline std::uint64_t pack_matrix_key(std::size_t rows, std::size_t columns) noexcept
        {
            return (static_cast<std::uint64_t>(rows) << 32U) | static_cast<std::uint64_t>(columns);
        }

        struct VectorMetrics
        {
            std::uint64_t sample_count{0};
            double total_abs_error{0.0};
            double total_relative_error{0.0};
            double max_abs_error{0.0};
            double max_relative_error{0.0};
            double last_abs_error{0.0};
            double last_relative_error{0.0};
        };

        struct MatrixMetrics
        {
            std::uint64_t sample_count{0};
            double total_abs_error{0.0};
            double total_relative_error{0.0};
            double max_abs_error{0.0};
            double max_relative_error{0.0};
            double last_abs_error{0.0};
            double last_relative_error{0.0};
        };
    } // namespace detail

    struct ConversionVectorEntry
    {
        std::size_t dimension{0};
        std::uint64_t sample_count{0};
        double max_abs_error{0.0};
        double max_relative_error{0.0};
        double mean_abs_error{0.0};
        double mean_relative_error{0.0};
        double last_abs_error{0.0};
        double last_relative_error{0.0};
    };

    struct ConversionMatrixEntry
    {
        std::size_t rows{0};
        std::size_t columns{0};
        std::uint64_t sample_count{0};
        double max_abs_error{0.0};
        double max_relative_error{0.0};
        double mean_abs_error{0.0};
        double mean_relative_error{0.0};
        double last_abs_error{0.0};
        double last_relative_error{0.0};
    };

    struct ConversionTelemetrySnapshot
    {
        std::vector<ConversionVectorEntry> vectors{};
        std::vector<ConversionMatrixEntry> matrices{};
    };

    class ConversionTelemetry
    {
    public:
        [[nodiscard]] static ConversionTelemetry& instance() noexcept
        {
            static ConversionTelemetry telemetry{};
            return telemetry;
        }

        void record_vector(std::size_t dimension, double max_abs_error, double max_relative_error) noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            detail::VectorMetrics& metrics = vector_metrics_[dimension];
            metrics.sample_count += 1U;
            metrics.total_abs_error += max_abs_error;
            metrics.total_relative_error += max_relative_error;
            metrics.max_abs_error = std::max(metrics.max_abs_error, max_abs_error);
            metrics.max_relative_error = std::max(metrics.max_relative_error, max_relative_error);
            metrics.last_abs_error = max_abs_error;
            metrics.last_relative_error = max_relative_error;
        }

        void record_matrix(std::size_t rows, std::size_t columns, double max_abs_error, double max_relative_error) noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            detail::MatrixMetrics& metrics = matrix_metrics_[detail::pack_matrix_key(rows, columns)];
            metrics.sample_count += 1U;
            metrics.total_abs_error += max_abs_error;
            metrics.total_relative_error += max_relative_error;
            metrics.max_abs_error = std::max(metrics.max_abs_error, max_abs_error);
            metrics.max_relative_error = std::max(metrics.max_relative_error, max_relative_error);
            metrics.last_abs_error = max_abs_error;
            metrics.last_relative_error = max_relative_error;
        }

        [[nodiscard]] ConversionTelemetrySnapshot snapshot() const noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ConversionTelemetrySnapshot snapshot{};
            snapshot.vectors.reserve(vector_metrics_.size());
            for (const auto& [dimension, metrics] : vector_metrics_)
            {
                ConversionVectorEntry entry{};
                entry.dimension = dimension;
                entry.sample_count = metrics.sample_count;
                entry.max_abs_error = metrics.max_abs_error;
                entry.max_relative_error = metrics.max_relative_error;
                entry.last_abs_error = metrics.last_abs_error;
                entry.last_relative_error = metrics.last_relative_error;
                if (metrics.sample_count > 0U)
                {
                    const double count = static_cast<double>(metrics.sample_count);
                    entry.mean_abs_error = metrics.total_abs_error / count;
                    entry.mean_relative_error = metrics.total_relative_error / count;
                }
                snapshot.vectors.push_back(entry);
            }

            std::sort(snapshot.vectors.begin(), snapshot.vectors.end(), [](const ConversionVectorEntry& lhs, const ConversionVectorEntry& rhs) {
                return lhs.dimension < rhs.dimension;
            });

            snapshot.matrices.reserve(matrix_metrics_.size());
            for (const auto& [key, metrics] : matrix_metrics_)
            {
                ConversionMatrixEntry entry{};
                entry.rows = static_cast<std::size_t>(key >> 32U);
                entry.columns = static_cast<std::size_t>(key & 0xFFFFFFFFULL);
                entry.sample_count = metrics.sample_count;
                entry.max_abs_error = metrics.max_abs_error;
                entry.max_relative_error = metrics.max_relative_error;
                entry.last_abs_error = metrics.last_abs_error;
                entry.last_relative_error = metrics.last_relative_error;
                if (metrics.sample_count > 0U)
                {
                    const double count = static_cast<double>(metrics.sample_count);
                    entry.mean_abs_error = metrics.total_abs_error / count;
                    entry.mean_relative_error = metrics.total_relative_error / count;
                }
                snapshot.matrices.push_back(entry);
            }

            std::sort(snapshot.matrices.begin(), snapshot.matrices.end(), [](const ConversionMatrixEntry& lhs, const ConversionMatrixEntry& rhs) {
                if (lhs.rows == rhs.rows)
                {
                    return lhs.columns < rhs.columns;
                }
                return lhs.rows < rhs.rows;
            });

            return snapshot;
        }

        void reset_for_testing() noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            vector_metrics_.clear();
            matrix_metrics_.clear();
        }

    private:
        ConversionTelemetry() = default;

        mutable std::mutex mutex_{};
        std::unordered_map<std::size_t, detail::VectorMetrics> vector_metrics_{};
        std::unordered_map<std::uint64_t, detail::MatrixMetrics> matrix_metrics_{};
    };

    template <typename T, std::size_t N>
    inline void RecordVectorRoundTrip(const Vector<T, N>& original, const Vector<T, N>& round_trip) noexcept
    {
        double max_abs_error = 0.0;
        double max_relative_error = 0.0;

        for (std::size_t i = 0; i < N; ++i)
        {
            const double original_component = static_cast<double>(original[i]);
            const double converted_component = static_cast<double>(round_trip[i]);
            const double absolute_error = std::abs(original_component - converted_component);
            max_abs_error = std::max(max_abs_error, absolute_error);
            const double denominator = detail::safe_denominator(original_component);
            const double relative_error = absolute_error / denominator;
            max_relative_error = std::max(max_relative_error, relative_error);
        }

        ConversionTelemetry::instance().record_vector(N, max_abs_error, max_relative_error);
    }

    template <typename T, std::size_t Rows, std::size_t Columns>
    inline void RecordMatrixRoundTrip(const Matrix<T, Rows, Columns>& original, const Matrix<T, Rows, Columns>& round_trip) noexcept
    {
        double max_abs_error = 0.0;
        double max_relative_error = 0.0;

        // For matrices, normalize relative error by the global maximum magnitude in the original matrix
        // to avoid blowing up relative errors for zero-valued elements (e.g., off-diagonals in identity matrices).
        double global_scale = 0.0;
        for (std::size_t column = 0; column < Columns; ++column)
        {
            for (std::size_t row = 0; row < Rows; ++row)
            {
                const double original_value = static_cast<double>(original[row][column]);
                global_scale = std::max(global_scale, std::abs(original_value));
            }
        }
        const double denominator = std::max(global_scale, detail::kMinimumMagnitude);

        for (std::size_t column = 0; column < Columns; ++column)
        {
            for (std::size_t row = 0; row < Rows; ++row)
            {
                const double original_value = static_cast<double>(original[row][column]);
                const double converted_value = static_cast<double>(round_trip[row][column]);
                const double absolute_error = std::abs(original_value - converted_value);
                max_abs_error = std::max(max_abs_error, absolute_error);
                const double relative_error = absolute_error / denominator;
                max_relative_error = std::max(max_relative_error, relative_error);
            }
        }

        ConversionTelemetry::instance().record_matrix(Rows, Columns, max_abs_error, max_relative_error);
    }
} // namespace engine::math::telemetry
