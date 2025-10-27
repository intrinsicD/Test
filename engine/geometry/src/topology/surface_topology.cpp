#include "engine/geometry/topology/surface_topology.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
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

        struct EdgeAccumulator
        {
            std::uint32_t v0{0};
            std::uint32_t v1{0};
            std::array<math::vec3, 2> normals{};
            std::array<bool, 2> has_valid_normal{false, false};
            std::uint32_t face_count{0};
            bool non_manifold{false};
        };

        [[nodiscard]] math::vec3 ComputeFaceNormal(const std::vector<math::vec3>& positions,
                                                   std::uint32_t i0,
                                                   std::uint32_t i1,
                                                   std::uint32_t i2,
                                                   bool& valid) noexcept
        {
            const math::vec3& p0 = positions[i0];
            const math::vec3& p1 = positions[i1];
            const math::vec3& p2 = positions[i2];

            const math::vec3 e0 = p1 - p0;
            const math::vec3 e1 = p2 - p0;
            auto normal = math::cross(e0, e1);
            const float length = math::length(normal);
            if (length <= std::numeric_limits<float>::epsilon())
            {
                valid = false;
                return math::vec3{0.0F, 0.0F, 0.0F};
            }
            valid = true;
            return normal / length;
        }

        void AccumulateEdge(std::unordered_map<EdgeKey, std::size_t, EdgeKeyHash>& lookup,
                            std::vector<EdgeAccumulator>& edges,
                            std::uint32_t v0,
                            std::uint32_t v1,
                            const math::vec3& normal,
                            bool normal_valid)
        {
            if (v0 > v1)
            {
                std::swap(v0, v1);
            }

            const EdgeKey key{v0, v1};
            const auto [it, inserted] = lookup.try_emplace(key, edges.size());
            if (inserted)
            {
                edges.push_back(EdgeAccumulator{.v0 = v0, .v1 = v1});
            }

            EdgeAccumulator& accumulator = edges[it->second];
            if (accumulator.face_count < 2)
            {
                accumulator.normals[accumulator.face_count] = normal;
                accumulator.has_valid_normal[accumulator.face_count] = normal_valid;
            }
            else
            {
                accumulator.non_manifold = true;
            }

            ++accumulator.face_count;
        }
    } // namespace

    SurfaceTopologySummary AnalyzeSurfaceTopology(const SurfaceMesh& mesh, float crease_angle_radians) noexcept
    {
        SurfaceTopologySummary summary{};

        const auto& positions = !mesh.positions.empty() ? mesh.positions : mesh.rest_positions;
        if (positions.empty())
        {
            return summary;
        }

        summary.vertices.resize(positions.size());

        if (mesh.indices.size() < 3 || (mesh.indices.size() % 3) != 0)
        {
            return summary;
        }

        const std::size_t triangle_count = mesh.indices.size() / 3;
        std::vector<EdgeAccumulator> edges;
        edges.reserve(triangle_count * 3);
        std::unordered_map<EdgeKey, std::size_t, EdgeKeyHash> lookup;
        lookup.reserve(triangle_count * 2);

        for (std::size_t triangle = 0; triangle < triangle_count; ++triangle)
        {
            const std::uint32_t i0 = mesh.indices[3 * triangle + 0];
            const std::uint32_t i1 = mesh.indices[3 * triangle + 1];
            const std::uint32_t i2 = mesh.indices[3 * triangle + 2];

            if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size())
            {
                continue;
            }

            bool normal_valid = false;
            const math::vec3 normal = ComputeFaceNormal(positions, i0, i1, i2, normal_valid);

            AccumulateEdge(lookup, edges, i0, i1, normal, normal_valid);
            AccumulateEdge(lookup, edges, i1, i2, normal, normal_valid);
            AccumulateEdge(lookup, edges, i2, i0, normal, normal_valid);
        }

        summary.edges.reserve(edges.size());
        const float threshold = std::max(0.0F, crease_angle_radians);

        for (const EdgeAccumulator& accumulator : edges)
        {
            SurfaceEdgeTag tag{};
            tag.v0 = accumulator.v0;
            tag.v1 = accumulator.v1;
            tag.is_non_manifold = accumulator.non_manifold || accumulator.face_count > 2;

            if (accumulator.face_count <= 1)
            {
                tag.is_boundary = true;
                tag.dihedral_angle = 0.0F;
            }
            else if (accumulator.face_count == 2 && accumulator.has_valid_normal[0] && accumulator.has_valid_normal[1])
            {
                const float dot = math::dot(accumulator.normals[0], accumulator.normals[1]);
                const float clamped = std::clamp(dot, -1.0F, 1.0F);
                tag.dihedral_angle = std::acos(clamped);
                tag.is_crease = tag.dihedral_angle >= threshold;
            }
            else
            {
                tag.dihedral_angle = 0.0F;
                if (accumulator.face_count == 2)
                {
                    tag.is_non_manifold = true;
                }
            }

            summary.edges.push_back(tag);

            auto& v0_tag = summary.vertices[tag.v0];
            auto& v1_tag = summary.vertices[tag.v1];

            if (tag.is_boundary)
            {
                v0_tag.is_boundary = true;
                v1_tag.is_boundary = true;
            }

            if (tag.is_boundary || tag.is_crease || tag.is_non_manifold)
            {
                v0_tag.is_feature = true;
                v1_tag.is_feature = true;
            }
        }

        return summary;
    }
} // namespace engine::geometry