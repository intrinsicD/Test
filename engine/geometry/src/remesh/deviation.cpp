#include "engine/geometry/remesh/deviation.hpp"

#include "engine/geometry/kdtree/kdtree.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/shapes/triangle.hpp"
#include "engine/math/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace engine::geometry
{
    namespace
    {
        constexpr std::size_t kTriangleSamplesPerQuery = 32U;

        struct WeightedAccumulator
        {
            double sum{0.0};
            double sum_sq{0.0};
            std::size_t count{0U};
            float max_distance{0.0F};

            void add(float distance) noexcept
            {
                if (!std::isfinite(distance))
                {
                    return;
                }

                const double magnitude = static_cast<double>(std::fabs(distance));
                sum += magnitude;
                sum_sq += magnitude * magnitude;
                ++count;
                max_distance = std::max(max_distance, static_cast<float>(magnitude));
            }
        };

        class TriangleAcceleration
        {
        public:
            bool build(const SurfaceMesh& mesh)
            {
                triangles_.clear();
                centroid_storage_.clear();

                const std::size_t index_count = mesh.indices.size();
                if (index_count < 3U || (index_count % 3U) != 0U || mesh.positions.empty())
                {
                    has_tree_ = false;
                    return false;
                }

                const std::size_t triangle_count = index_count / 3U;
                triangles_.resize(triangle_count);

                centroid_storage_.resize(triangle_count);
                auto centroids = centroid_storage_.add<math::vec3>("n:triangle_centroids", math::vec3{0.0F});

                for (std::size_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
                {
                    const std::size_t base_index = triangle_index * 3U;
                    const std::uint32_t i0 = mesh.indices[base_index];
                    const std::uint32_t i1 = mesh.indices[base_index + 1U];
                    const std::uint32_t i2 = mesh.indices[base_index + 2U];

                    if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size())
                    {
                        // Skip invalid triangles by collapsing them to zero area.
                        triangles_[triangle_index] = Triangle{};
                        centroids[triangle_index] = math::vec3{0.0F};
                        continue;
                    }

                    Triangle triangle{mesh.positions[i0], mesh.positions[i1], mesh.positions[i2]};
                    triangles_[triangle_index] = triangle;
                    centroids[triangle_index] = Centroid(triangle);
                }

                has_tree_ = tree_.build(centroids, 16U, 32U);
                return !triangles_.empty();
            }

            [[nodiscard]] float closest_distance(const math::vec3& point,
                                                  std::vector<std::size_t>& candidates) const noexcept
            {
                if (triangles_.empty())
                {
                    return 0.0F;
                }

                double best_sq = std::numeric_limits<double>::infinity();

                if (has_tree_)
                {
                    candidates.clear();
                    tree_.query_knn(point, kTriangleSamplesPerQuery, candidates);
                    for (const std::size_t index : candidates)
                    {
                        if (index >= triangles_.size())
                        {
                            continue;
                        }
                        const double distance_sq = SquaredDistance(triangles_[index], point);
                        best_sq = std::min(best_sq, distance_sq);
                    }
                }

                if (!std::isfinite(best_sq))
                {
                    for (const Triangle& triangle : triangles_)
                    {
                        const double distance_sq = SquaredDistance(triangle, point);
                        best_sq = std::min(best_sq, distance_sq);
                    }
                }

                if (!std::isfinite(best_sq))
                {
                    return 0.0F;
                }

                return static_cast<float>(std::sqrt(std::max(best_sq, 0.0)));
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return triangles_.empty();
            }

        private:
            std::vector<Triangle> triangles_{};
            PropertySet centroid_storage_{};
            KdTree tree_{};
            bool has_tree_{false};
        };

        void accumulate_samples(const SurfaceMesh& source,
                                const TriangleAcceleration& target,
                                WeightedAccumulator& accumulator,
                                std::vector<std::size_t>& buffer) noexcept
        {
            if (target.empty())
            {
                return;
            }

            auto accumulate_point = [&](const math::vec3& point)
            {
                const float distance = target.closest_distance(point, buffer);
                accumulator.add(distance);
            };

            for (const math::vec3& position : source.positions)
            {
                accumulate_point(position);
            }

            const std::size_t index_count = source.indices.size();
            if (index_count < 3U)
            {
                return;
            }

            for (std::size_t index = 0; index + 2U < index_count; index += 3U)
            {
                const std::uint32_t i0 = source.indices[index];
                const std::uint32_t i1 = source.indices[index + 1U];
                const std::uint32_t i2 = source.indices[index + 2U];

                if (i0 >= source.positions.size() || i1 >= source.positions.size() || i2 >= source.positions.size())
                {
                    continue;
                }

                const math::vec3& p0 = source.positions[i0];
                const math::vec3& p1 = source.positions[i1];
                const math::vec3& p2 = source.positions[i2];

                const math::vec3 centroid = (p0 + p1 + p2) / 3.0F;
                accumulate_point(centroid);
                accumulate_point((p0 + p1) * 0.5F);
                accumulate_point((p1 + p2) * 0.5F);
                accumulate_point((p2 + p0) * 0.5F);
            }
        }
    } // namespace

    SurfaceDeviationMetrics ComputeSurfaceDeviationMetrics(const SurfaceMesh& reference,
                                                           const SurfaceMesh& candidate) noexcept
    {
        SurfaceDeviationMetrics metrics{};

        const std::size_t reference_triangles = reference.indices.size() / 3U;
        const std::size_t candidate_triangles = candidate.indices.size() / 3U;
        if (reference_triangles == 0U || candidate_triangles == 0U)
        {
            return metrics;
        }

        TriangleAcceleration reference_acceleration;
        TriangleAcceleration candidate_acceleration;
        if (!reference_acceleration.build(reference) || !candidate_acceleration.build(candidate))
        {
            return metrics;
        }

        WeightedAccumulator reference_accumulator{};
        WeightedAccumulator candidate_accumulator{};
        std::vector<std::size_t> candidate_buffer;
        candidate_buffer.reserve(kTriangleSamplesPerQuery);

        accumulate_samples(reference, candidate_acceleration, reference_accumulator, candidate_buffer);
        accumulate_samples(candidate, reference_acceleration, candidate_accumulator, candidate_buffer);

        metrics.reference_to_candidate_max = reference_accumulator.max_distance;
        metrics.candidate_to_reference_max = candidate_accumulator.max_distance;
        metrics.max_distance = std::max(reference_accumulator.max_distance, candidate_accumulator.max_distance);

        const double total_samples = static_cast<double>(reference_accumulator.count + candidate_accumulator.count);
        metrics.sample_count = reference_accumulator.count + candidate_accumulator.count;
        if (total_samples > 0.0)
        {
            metrics.mean_distance = static_cast<float>(
                (reference_accumulator.sum + candidate_accumulator.sum) / total_samples);
            metrics.rms_distance = static_cast<float>(
                std::sqrt((reference_accumulator.sum_sq + candidate_accumulator.sum_sq) / total_samples));
        }

        return metrics;
    }
} // namespace engine::geometry

