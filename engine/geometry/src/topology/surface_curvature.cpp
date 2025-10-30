#include "engine/geometry/topology/surface_curvature.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <unordered_map>
#include <utility>

#include "engine/geometry/api.hpp"
#include "engine/math/vector.hpp"

namespace engine::geometry
{
    namespace
    {
        using EdgeKey = std::pair<std::uint32_t, std::uint32_t>;

        struct EdgeKeyHash
        {
            [[nodiscard]] std::size_t operator()(const EdgeKey& key) const noexcept
            {
                constexpr auto shift = static_cast<std::size_t>(32);
                return (static_cast<std::size_t>(key.first) << shift) ^ static_cast<std::size_t>(key.second);
            }
        };

        [[nodiscard]] float safe_angle(const math::vec3& u, const math::vec3& v) noexcept
        {
            const math::vec3 cross = math::cross(u, v);
            const float cross_norm = math::length(cross);
            if (cross_norm <= std::numeric_limits<float>::epsilon())
            {
                return 0.0F;
            }

            const float dot = math::dot(u, v);
            return std::atan2(cross_norm, dot);
        }

        [[nodiscard]] float safe_cotangent(const math::vec3& u, const math::vec3& v) noexcept
        {
            const math::vec3 cross = math::cross(u, v);
            const float cross_norm = math::length(cross);
            if (cross_norm <= std::numeric_limits<float>::epsilon())
            {
                return 0.0F;
            }

            const float dot = math::dot(u, v);
            return dot / cross_norm;
        }
    } // namespace

    SurfaceCurvatureResult ComputeSurfaceCurvature(const SurfaceMesh& mesh) noexcept
    {
        SurfaceCurvatureResult result{};

        const auto& positions = mesh.positions.empty() ? mesh.rest_positions : mesh.positions;
        const std::size_t vertex_count = positions.size();
        result.mean_curvature.assign(vertex_count, 0.0F);
        result.gaussian_curvature.assign(vertex_count, 0.0F);

        if (vertex_count == 0 || mesh.indices.size() < 3 || (mesh.indices.size() % 3) != 0)
        {
            return result;
        }

        std::vector<float> mixed_area(vertex_count, 0.0F);
        std::vector<float> angle_sum(vertex_count, 0.0F);
        std::vector<math::vec3> mean_vectors(vertex_count, math::vec3{0.0F});
        std::unordered_map<EdgeKey, std::uint8_t, EdgeKeyHash> edge_face_counts;
        edge_face_counts.reserve(mesh.indices.size());

        auto register_edge = [&edge_face_counts](std::uint32_t a, std::uint32_t b) {
            if (a > b)
            {
                std::swap(a, b);
            }

            const EdgeKey key{a, b};
            const auto [it, inserted] = edge_face_counts.try_emplace(key, static_cast<std::uint8_t>(0));
            (void)inserted;
            if (it->second < std::numeric_limits<std::uint8_t>::max())
            {
                ++it->second;
            }
        };

        auto accumulate_edge = [&mean_vectors, &positions](std::uint32_t i, std::uint32_t j, float weight) {
            if (!std::isfinite(weight) || std::abs(weight) <= std::numeric_limits<float>::epsilon())
            {
                return;
            }

            const math::vec3 diff = positions[i] - positions[j];
            mean_vectors[i] += diff * weight;
            mean_vectors[j] -= diff * weight;
        };

        const std::size_t triangle_count = mesh.indices.size() / 3;
        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle)
        {
            const std::uint32_t i0 = mesh.indices[3 * triangle + 0];
            const std::uint32_t i1 = mesh.indices[3 * triangle + 1];
            const std::uint32_t i2 = mesh.indices[3 * triangle + 2];

            if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
            {
                continue;
            }

            const math::vec3& p0 = positions[i0];
            const math::vec3& p1 = positions[i1];
            const math::vec3& p2 = positions[i2];

            const math::vec3 e01 = p1 - p0;
            const math::vec3 e02 = p2 - p0;
            const math::vec3 e10 = p0 - p1;
            const math::vec3 e12 = p2 - p1;
            const math::vec3 e20 = p0 - p2;
            const math::vec3 e21 = p1 - p2;

            const math::vec3 normal = math::cross(e01, e02);
            const float double_area = math::length(normal);
            if (double_area <= std::numeric_limits<float>::epsilon())
            {
                continue;
            }

            register_edge(i0, i1);
            register_edge(i1, i2);
            register_edge(i2, i0);

            const float area = 0.5F * double_area;
            const float area_third = area / 3.0F;
            mixed_area[i0] += area_third;
            mixed_area[i1] += area_third;
            mixed_area[i2] += area_third;

            angle_sum[i0] += safe_angle(e01, e02);
            angle_sum[i1] += safe_angle(e12, e10);
            angle_sum[i2] += safe_angle(e20, e21);

            const float cot_alpha = safe_cotangent(e01, e02);
            const float cot_beta = safe_cotangent(e12, e10);
            const float cot_gamma = safe_cotangent(e20, e21);

            accumulate_edge(i1, i2, cot_alpha);
            accumulate_edge(i2, i0, cot_beta);
            accumulate_edge(i0, i1, cot_gamma);
        }

        std::vector<bool> is_boundary(vertex_count, false);
        for (const auto& [edge, count] : edge_face_counts)
        {
            if (count <= 1)
            {
                is_boundary[edge.first] = true;
                is_boundary[edge.second] = true;
            }
        }

        const float two_pi = 2.0F * std::numbers::pi_v<float>;
        for (std::size_t vertex = 0; vertex < vertex_count; ++vertex)
        {
            const float area = mixed_area[vertex];
            if (area <= std::numeric_limits<float>::epsilon())
            {
                result.mean_curvature[vertex] = 0.0F;
                result.gaussian_curvature[vertex] = 0.0F;
                continue;
            }

            if (!is_boundary[vertex])
            {
                const float gaussian = (two_pi - angle_sum[vertex]) / area;
                result.gaussian_curvature[vertex] = std::isfinite(gaussian) ? gaussian : 0.0F;
            }
            else
            {
                result.gaussian_curvature[vertex] = 0.0F;
            }

            const float mean = 0.5F * math::length(mean_vectors[vertex]) / area;
            result.mean_curvature[vertex] = std::isfinite(mean) ? mean : 0.0F;
        }

        return result;
    }
} // namespace engine::geometry
