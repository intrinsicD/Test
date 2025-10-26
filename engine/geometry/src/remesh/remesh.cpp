#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/remesh/deviation.hpp"
#include "engine/geometry/remesh/telemetry.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <exception>
#include <limits>
#include <numbers>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/math/math.hpp"
#include "engine/math/utils/utils.hpp"
#include "engine/geometry/mesh/halfedge_mesh.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"
#include "engine/geometry/topology/surface_topology.hpp"

namespace engine::geometry
{
    namespace
    {
        constexpr float kEpsilon = 1e-6F;
        constexpr std::uint32_t kInvalidIsland = std::numeric_limits<std::uint32_t>::max();
        constexpr char kRestPositionPropertyName[] = "v:rest_position";

        struct RemeshOperationCounters
        {
            std::uint64_t split_count{0};
            std::uint64_t collapse_count{0};
        };

        [[nodiscard]] RemeshValidationResult make_target_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_target_configuration, std::move(message))};
        }

        [[nodiscard]] RemeshValidationResult make_attribute_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_attribute_policy, std::move(message))};
        }

        [[nodiscard]] RemeshValidationResult make_parameterization_error(std::string message)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_parameterization, std::move(message))};
        }

        [[nodiscard]] bool is_positive_finite(float value) noexcept
        {
            return std::isfinite(value) && value > 0.0f;
        }

        [[nodiscard]] bool is_non_negative_finite(float value) noexcept
        {
            return std::isfinite(value) && value >= 0.0f;
        }

        [[nodiscard]] std::uint64_t make_edge_key(std::uint32_t a, std::uint32_t b) noexcept
        {
            if (a > b)
            {
                std::swap(a, b);
            }
            return (static_cast<std::uint64_t>(a) << 32U) | static_cast<std::uint64_t>(b);
        }

        [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> decode_edge_key(std::uint64_t key) noexcept
        {
            const std::uint32_t a = static_cast<std::uint32_t>(key >> 32U);
            const std::uint32_t b = static_cast<std::uint32_t>(key & 0xFFFFFFFFU);
            return {a, b};
        }

        [[nodiscard]] float triangle_area(const math::vec3& a, const math::vec3& b, const math::vec3& c) noexcept
        {
            return 0.5F * math::length(math::cross(b - a, c - a));
        }

        [[nodiscard]] float triangle_uv_area(const math::vec2& a, const math::vec2& b, const math::vec2& c) noexcept
        {
            const math::vec2 ab = b - a;
            const math::vec2 ac = c - a;
            const float ab_x = ab[0];
            const float ab_y = ab[1];
            const float ac_x = ac[0];
            const float ac_y = ac[1];
            return 0.5F * std::fabs(ab_x * ac_y - ab_y * ac_x);
        }

        [[nodiscard]] std::unordered_set<std::uint64_t> build_protected_edge_set(
            const SurfaceTopologySummary& summary,
            const RemeshRequest& request)
        {
            std::unordered_set<std::uint64_t> protected_edges{};

            if (!request.feature_preservation.lock_boundary_edges &&
                !request.feature_preservation.lock_feature_edges)
            {
                return protected_edges;
            }

            protected_edges.reserve(summary.edges.size());
            for (const SurfaceEdgeTag& tag : summary.edges)
            {
                const bool protect_boundary = request.feature_preservation.lock_boundary_edges && tag.is_boundary;
                const bool protect_feature = request.feature_preservation.lock_feature_edges &&
                                             (tag.is_crease || tag.is_non_manifold);

                if (protect_boundary || protect_feature)
                {
                    protected_edges.insert(make_edge_key(tag.v0, tag.v1));
                }
            }

            return protected_edges;
        }

        [[nodiscard]] std::vector<math::vec3> compute_vertex_normals(const MeshInterface& interface)
        {
            std::vector<math::vec3> normals(interface.vertices_size(), math::vec3{0.0F});

            for (auto face : interface.faces())
            {
                if (interface.is_deleted(face) || interface.is_boundary(face))
                {
                    continue;
                }

                const HalfedgeHandle h0 = interface.halfedge(face);
                if (!h0.is_valid())
                {
                    continue;
                }

                const HalfedgeHandle h1 = interface.next_halfedge(h0);
                const HalfedgeHandle h2 = interface.next_halfedge(h1);

                const VertexHandle v0 = interface.to_vertex(h0);
                const VertexHandle v1 = interface.to_vertex(h1);
                const VertexHandle v2 = interface.to_vertex(h2);

                const math::vec3& p0 = interface.position(v0);
                const math::vec3& p1 = interface.position(v1);
                const math::vec3& p2 = interface.position(v2);

                const math::vec3 weighted_normal = math::cross(p1 - p0, p2 - p0);
                if (math::dot(weighted_normal, weighted_normal) <= std::numeric_limits<float>::epsilon())
                {
                    continue;
                }

                normals[v0.index()] += weighted_normal;
                normals[v1.index()] += weighted_normal;
                normals[v2.index()] += weighted_normal;
            }

            for (auto vertex : interface.vertices())
            {
                if (interface.is_deleted(vertex))
                {
                    continue;
                }

                math::vec3& normal = normals[vertex.index()];
                const float length_sq = math::dot(normal, normal);
                if (length_sq > std::numeric_limits<float>::epsilon())
                {
                    normal /= std::sqrt(length_sq);
                }
                else
                {
                    normal = math::vec3{0.0F, 0.0F, 0.0F};
                }
            }

            return normals;
        }

        struct ParameterizationIsland
        {
            std::vector<std::uint32_t> vertices{};
            math::vec2 translation{0.0F, 0.0F};
            float scale{1.0F};
        };

        struct IslandBounds
        {
            math::vec2 min{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
            math::vec2 max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
            float width{0.0F};
            float height{0.0F};
        };

        [[nodiscard]] std::vector<ParameterizationIsland> collect_parameterization_islands(
            const SurfaceMesh& mesh) noexcept
        {
            std::vector<ParameterizationIsland> islands{};

            if (mesh.texture_coordinates.empty() || mesh.indices.size() < 3U)
            {
                return islands;
            }

            const std::size_t vertex_count = mesh.positions.size();
            const std::size_t uv_count = mesh.texture_coordinates.size();
            if (vertex_count == 0U || uv_count == 0U)
            {
                return islands;
            }

            std::vector<std::vector<std::uint32_t>> adjacency(vertex_count);
            std::vector<bool> used(vertex_count, false);

            for (std::size_t triangle = 0U; triangle + 2U < mesh.indices.size(); triangle += 3U)
            {
                const std::uint32_t i0 = mesh.indices[triangle + 0U];
                const std::uint32_t i1 = mesh.indices[triangle + 1U];
                const std::uint32_t i2 = mesh.indices[triangle + 2U];

                if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
                {
                    continue;
                }

                if (i0 >= uv_count || i1 >= uv_count || i2 >= uv_count)
                {
                    continue;
                }

                used[i0] = true;
                used[i1] = true;
                used[i2] = true;

                adjacency[i0].push_back(i1);
                adjacency[i0].push_back(i2);
                adjacency[i1].push_back(i0);
                adjacency[i1].push_back(i2);
                adjacency[i2].push_back(i0);
                adjacency[i2].push_back(i1);
            }

            std::vector<bool> visited(vertex_count, false);
            std::vector<std::uint32_t> stack;
            stack.reserve(vertex_count);

            for (std::size_t vertex = 0U; vertex < vertex_count; ++vertex)
            {
                if (!used[vertex] || visited[vertex])
                {
                    continue;
                }

                ParameterizationIsland island{};
                stack.clear();
                stack.push_back(static_cast<std::uint32_t>(vertex));
                visited[vertex] = true;

                while (!stack.empty())
                {
                    const std::uint32_t current = stack.back();
                    stack.pop_back();

                    island.vertices.push_back(current);

                    for (const std::uint32_t neighbour : adjacency[current])
                    {
                        if (neighbour >= vertex_count)
                        {
                            continue;
                        }
                        if (!used[neighbour] || visited[neighbour])
                        {
                            continue;
                        }

                        visited[neighbour] = true;
                        stack.push_back(neighbour);
                    }
                }

                islands.push_back(std::move(island));
            }

            return islands;
        }

        [[nodiscard]] IslandBounds compute_island_bounds(const SurfaceMesh& mesh,
                                                         const ParameterizationIsland& island) noexcept
        {
            IslandBounds bounds{};
            const std::size_t uv_count = mesh.texture_coordinates.size();

            for (const std::uint32_t vertex : island.vertices)
            {
                if (vertex >= uv_count)
                {
                    continue;
                }

                const math::vec2& uv = mesh.texture_coordinates[vertex];
                bounds.min[0] = std::min(bounds.min[0], uv[0]);
                bounds.min[1] = std::min(bounds.min[1], uv[1]);
                bounds.max[0] = std::max(bounds.max[0], uv[0]);
                bounds.max[1] = std::max(bounds.max[1], uv[1]);
            }

            if (!std::isfinite(bounds.min[0]))
            {
                bounds.min = math::vec2{0.0F, 0.0F};
                bounds.max = math::vec2{0.0F, 0.0F};
            }

            bounds.width = std::max(bounds.max[0] - bounds.min[0], 0.0F);
            bounds.height = std::max(bounds.max[1] - bounds.min[1], 0.0F);
            return bounds;
        }

        void scale_texture_coordinates(SurfaceMesh& mesh, float scale) noexcept
        {
            if (!std::isfinite(scale) || std::abs(scale - 1.0F) <= kEpsilon)
            {
                return;
            }

            for (math::vec2& uv : mesh.texture_coordinates)
            {
                uv *= scale;
            }
        }

        void apply_global_scale(SurfaceMesh& mesh,
                                std::vector<ParameterizationIsland>& islands,
                                float scale) noexcept
        {
            if (!std::isfinite(scale) || std::abs(scale - 1.0F) <= kEpsilon)
            {
                return;
            }

            scale_texture_coordinates(mesh, scale);
            for (auto& island : islands)
            {
                island.translation *= scale;
                island.scale *= scale;
            }
        }

        void repack_parameterization_islands(SurfaceMesh& mesh,
                                             float gutter_width,
                                             std::vector<ParameterizationIsland>& islands) noexcept
        {
            if (islands.empty())
            {
                return;
            }

            struct PackingInfo
            {
                std::size_t island_index{0U};
                IslandBounds bounds{};
                float padded_width{0.0F};
                float padded_height{0.0F};
            };

            std::vector<PackingInfo> packing;
            packing.reserve(islands.size());

            const float gutter = std::max(gutter_width, 0.0F);
            const float half_gutter = gutter * 0.5F;

            float total_area = 0.0F;
            float max_width = 0.0F;

            for (std::size_t index = 0U; index < islands.size(); ++index)
            {
                const IslandBounds bounds = compute_island_bounds(mesh, islands[index]);
                PackingInfo info{};
                info.island_index = index;
                info.bounds = bounds;
                info.padded_width = bounds.width + gutter;
                info.padded_height = bounds.height + gutter;
                packing.push_back(info);

                total_area += info.padded_width * info.padded_height;
                max_width = std::max(max_width, info.padded_width);
            }

            if (total_area <= kEpsilon)
            {
                return;
            }

            std::sort(packing.begin(), packing.end(), [](const PackingInfo& a, const PackingInfo& b) {
                return a.padded_height > b.padded_height;
            });

            const float shelf_limit = std::max(std::sqrt(total_area), max_width);
            float current_x = 0.0F;
            float current_y = 0.0F;
            float shelf_height = 0.0F;
            float pack_width = 0.0F;

            for (const PackingInfo& info : packing)
            {
                if (current_x > 0.0F && current_x + info.padded_width > shelf_limit)
                {
                    current_y += shelf_height;
                    current_x = 0.0F;
                    shelf_height = 0.0F;
                }

                const float offset_x = current_x + half_gutter - info.bounds.min[0];
                const float offset_y = current_y + half_gutter - info.bounds.min[1];

                ParameterizationIsland& island = islands[info.island_index];
                for (const std::uint32_t vertex : island.vertices)
                {
                    if (vertex >= mesh.texture_coordinates.size())
                    {
                        continue;
                    }

                    math::vec2& uv = mesh.texture_coordinates[vertex];
                    uv[0] += offset_x;
                    uv[1] += offset_y;
                }

                island.translation += math::vec2{offset_x, offset_y};

                current_x += info.padded_width;
                shelf_height = std::max(shelf_height, info.padded_height);
                pack_width = std::max(pack_width, current_x);
            }

            const float pack_height = current_y + shelf_height;
            const float max_extent = std::max(pack_width, pack_height);
            if (max_extent <= std::numeric_limits<float>::epsilon())
            {
                return;
            }

            const float normalization_scale = 1.0F / max_extent;
            apply_global_scale(mesh, islands, normalization_scale);
        }

        [[nodiscard]] ParameterizationSummary compute_parameterization_summary(
            const SurfaceMesh& mesh,
            const std::vector<ParameterizationIsland>& islands) noexcept
        {
            ParameterizationSummary summary{};
            summary.chart_count = static_cast<std::uint32_t>(islands.size());

            const std::size_t uv_count = mesh.texture_coordinates.size();
            if (uv_count == 0U || mesh.indices.size() < 3U)
            {
                return summary;
            }

            const std::size_t position_count = mesh.positions.size();
            if (position_count == 0U)
            {
                return summary;
            }

            std::vector<std::uint32_t> vertex_to_island(uv_count, kInvalidIsland);
            for (std::uint32_t island_index = 0; island_index < islands.size(); ++island_index)
            {
                for (const std::uint32_t vertex : islands[island_index].vertices)
                {
                    if (vertex < uv_count)
                    {
                        vertex_to_island[vertex] = island_index;
                    }
                }
            }

            std::vector<std::unordered_map<std::uint64_t, std::uint32_t>> island_edge_counts(islands.size());
            for (auto& edge_counts : island_edge_counts)
            {
                edge_counts.reserve(32U);
            }

            float total_area = 0.0F;
            float weighted_density = 0.0F;
            float max_density = 0.0F;

            for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
            {
                const std::uint32_t i0 = mesh.indices[i];
                const std::uint32_t i1 = mesh.indices[i + 1U];
                const std::uint32_t i2 = mesh.indices[i + 2U];

                if (i0 >= position_count || i1 >= position_count || i2 >= position_count)
                {
                    continue;
                }

                const float world_area = triangle_area(mesh.positions[i0], mesh.positions[i1], mesh.positions[i2]);
                if (world_area <= kEpsilon)
                {
                    continue;
                }

                if (i0 >= uv_count || i1 >= uv_count || i2 >= uv_count)
                {
                    continue;
                }

                const float uv_area =
                    triangle_uv_area(mesh.texture_coordinates[i0], mesh.texture_coordinates[i1],
                                     mesh.texture_coordinates[i2]);

                const float density = uv_area <= kEpsilon ? 0.0F : std::sqrt(std::max(uv_area / world_area, 0.0F));

                weighted_density += density * world_area;
                total_area += world_area;
                max_density = std::max(max_density, density);

                const std::uint32_t island_index = vertex_to_island[i0];
                if (island_index == kInvalidIsland || vertex_to_island[i1] != island_index ||
                    vertex_to_island[i2] != island_index)
                {
                    continue;
                }

                auto& edge_counts = island_edge_counts[island_index];
                const std::array<std::pair<std::uint32_t, std::uint32_t>, 3> edges{{
                    {i0, i1},
                    {i1, i2},
                    {i2, i0},
                }};

                for (const auto& [a, b] : edges)
                {
                    if (a == b)
                    {
                        continue;
                    }

                    ++edge_counts[make_edge_key(a, b)];
                }
            }

            if (total_area > kEpsilon)
            {
                summary.average_stretch = weighted_density / total_area;
                summary.texel_density = summary.average_stretch;
            }

            summary.max_stretch = max_density;
            summary.charts.reserve(islands.size());

            math::vec2 atlas_min{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
            math::vec2 atlas_max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
            float total_chart_area = 0.0F;
            float total_seam_length = 0.0F;

            for (std::size_t island_index = 0; island_index < islands.size(); ++island_index)
            {
                const auto& island = islands[island_index];
                const IslandBounds bounds = compute_island_bounds(mesh, island);
                ParameterizationChart chart{};
                chart.min_uv = bounds.min;
                chart.max_uv = bounds.max;
                chart.translation = island.translation;
                chart.scale = island.scale;
                chart.area = std::max(bounds.width * bounds.height, 0.0F);

                atlas_min[0] = std::min(atlas_min[0], bounds.min[0]);
                atlas_min[1] = std::min(atlas_min[1], bounds.min[1]);
                atlas_max[0] = std::max(atlas_max[0], bounds.max[0]);
                atlas_max[1] = std::max(atlas_max[1], bounds.max[1]);

                float island_seam_length = 0.0F;
                if (island_index < island_edge_counts.size())
                {
                    const auto& edges = island_edge_counts[island_index];
                    for (const auto& [key, count] : edges)
                    {
                        if (count != 1U)
                        {
                            continue;
                        }

                        const auto [u, v] = decode_edge_key(key);
                        if (u >= uv_count || v >= uv_count)
                        {
                            continue;
                        }

                        island_seam_length += math::length(mesh.texture_coordinates[u] - mesh.texture_coordinates[v]);
                    }
                }

                chart.boundary_length = island_seam_length;
                total_chart_area += chart.area;
                total_seam_length += island_seam_length;

                summary.charts.push_back(chart);
            }

            summary.total_chart_area = total_chart_area;
            summary.total_seam_length = total_seam_length;

            if (std::isfinite(atlas_min[0]) && std::isfinite(atlas_min[1]) && std::isfinite(atlas_max[0]) &&
                std::isfinite(atlas_max[1]))
            {
                const float atlas_width = std::max(atlas_max[0] - atlas_min[0], 0.0F);
                const float atlas_height = std::max(atlas_max[1] - atlas_min[1], 0.0F);
                summary.atlas_area = atlas_width * atlas_height;
                if (summary.atlas_area > kEpsilon)
                {
                    const float ratio = total_chart_area / summary.atlas_area;
                    summary.fill_ratio = std::clamp(ratio, 0.0F, 1.0F);
                }
            }

            return summary;
        }

        [[nodiscard]] ParameterizationSummary finalize_parameterization(
            SurfaceMesh& mesh,
            const ParameterizationPolicy& policy) noexcept
        {
            std::vector<ParameterizationIsland> islands = collect_parameterization_islands(mesh);
            if (islands.empty())
            {
                return ParameterizationSummary{};
            }

            const bool force_repack =
                (policy.mode == ParameterizationMode::kReuseExisting) && !policy.allow_chart_reuse;
            if (policy.repack_islands || force_repack)
            {
                repack_parameterization_islands(mesh, policy.gutter_width, islands);
            }

            ParameterizationSummary summary = compute_parameterization_summary(mesh, islands);

            if (policy.target_texel_density > kEpsilon && summary.texel_density > kEpsilon)
            {
                const float target_scale = policy.target_texel_density / summary.texel_density;
                apply_global_scale(mesh, islands, target_scale);
                summary = compute_parameterization_summary(mesh, islands);
            }

            return summary;
        }

        void assign_interpolated_uv(VertexProperty<math::vec2>& texture_coordinates,
                                     VertexHandle target,
                                     const math::vec2& a,
                                     const math::vec2& b) noexcept
        {
            texture_coordinates[target] = (a + b) * 0.5F;
        }

        void update_collapse_uv(VertexProperty<math::vec2>& texture_coordinates,
                                VertexHandle keep,
                                VertexHandle remove) noexcept
        {
            const math::vec2& uv_keep = texture_coordinates[keep];
            const math::vec2& uv_remove = texture_coordinates[remove];
            texture_coordinates[keep] = (uv_keep + uv_remove) * 0.5F;
        }

        void assign_interpolated_rest_position(VertexProperty<math::vec3>& rest_positions,
                                               VertexHandle target,
                                               const math::vec3& a,
                                               const math::vec3& b) noexcept
        {
            rest_positions[target] = (a + b) * 0.5F;
        }

        void update_collapse_rest_position(VertexProperty<math::vec3>& rest_positions,
                                           VertexHandle keep,
                                           VertexHandle remove) noexcept
        {
            const math::vec3& keep_rest = rest_positions[keep];
            const math::vec3& remove_rest = rest_positions[remove];
            rest_positions[keep] = (keep_rest + remove_rest) * 0.5F;
        }

        struct ParameterizationAnchors
        {
            std::uint32_t primary{0U};
            std::uint32_t secondary{1U};
        };

        [[nodiscard]] std::optional<ParameterizationAnchors> select_parameterization_anchors(
            const SurfaceMesh& mesh,
            const SurfaceTopologySummary& topology_summary) noexcept
        {
            const std::size_t vertex_count = mesh.positions.size();
            if (vertex_count < 2U)
            {
                return std::nullopt;
            }

            const auto squared_distance = [&](std::uint32_t a, std::uint32_t b) noexcept
            {
                const math::vec3 delta = mesh.positions[b] - mesh.positions[a];
                return math::dot(delta, delta);
            };

            std::vector<std::uint32_t> boundary_indices{};
            boundary_indices.reserve(topology_summary.vertices.size());
            for (std::uint32_t index = 0U; index < topology_summary.vertices.size() && index < vertex_count; ++index)
            {
                if (topology_summary.vertices[index].is_boundary)
                {
                    boundary_indices.push_back(index);
                }
            }

            ParameterizationAnchors anchors{};
            if (!boundary_indices.empty())
            {
                anchors.primary = boundary_indices.front();
            }
            else
            {
                anchors.primary = 0U;
            }

            float best_distance = -1.0F;
            auto consider_candidate = [&](std::uint32_t candidate)
            {
                if (candidate >= vertex_count || candidate == anchors.primary)
                {
                    return;
                }
                const float distance = squared_distance(anchors.primary, candidate);
                if (distance > best_distance)
                {
                    best_distance = distance;
                    anchors.secondary = candidate;
                }
            };

            for (std::uint32_t candidate : boundary_indices)
            {
                consider_candidate(candidate);
            }

            if (best_distance <= 0.0F)
            {
                for (std::uint32_t candidate = 0U; candidate < vertex_count; ++candidate)
                {
                    consider_candidate(candidate);
                }
            }

            if (anchors.secondary == anchors.primary)
            {
                if (vertex_count < 2U)
                {
                    return std::nullopt;
                }
                anchors.secondary = (anchors.primary + 1U) % static_cast<std::uint32_t>(vertex_count);
            }

            if (anchors.secondary == anchors.primary || anchors.secondary >= vertex_count)
            {
                return std::nullopt;
            }

            return anchors;
        }

        [[nodiscard]] std::optional<std::vector<double>> solve_dense_linear_system(std::vector<double> matrix,
                                                                                   std::vector<double> rhs,
                                                                                   std::size_t order) noexcept
        {
            if (order == 0U || matrix.size() != order * order || rhs.size() != order)
            {
                return std::nullopt;
            }

            for (std::size_t pivot = 0U; pivot < order; ++pivot)
            {
                std::size_t best_row = pivot;
                double best_value = std::abs(matrix[pivot * order + pivot]);
                for (std::size_t candidate = pivot + 1U; candidate < order; ++candidate)
                {
                    const double value = std::abs(matrix[candidate * order + pivot]);
                    if (value > best_value)
                    {
                        best_value = value;
                        best_row = candidate;
                    }
                }

                if (best_value <= 1e-12)
                {
                    return std::nullopt;
                }

                if (best_row != pivot)
                {
                    for (std::size_t column = 0U; column < order; ++column)
                    {
                        std::swap(matrix[pivot * order + column], matrix[best_row * order + column]);
                    }
                    std::swap(rhs[pivot], rhs[best_row]);
                }

                const double pivot_value = matrix[pivot * order + pivot];
                for (std::size_t column = pivot; column < order; ++column)
                {
                    matrix[pivot * order + column] /= pivot_value;
                }
                rhs[pivot] /= pivot_value;

                for (std::size_t row = 0U; row < order; ++row)
                {
                    if (row == pivot)
                    {
                        continue;
                    }

                    const double factor = matrix[row * order + pivot];
                    if (std::abs(factor) <= 1e-12)
                    {
                        continue;
                    }

                    for (std::size_t column = pivot; column < order; ++column)
                    {
                        matrix[row * order + column] -= factor * matrix[pivot * order + column];
                    }
                    rhs[row] -= factor * rhs[pivot];
                }
            }

            return rhs;
        }

        [[nodiscard]] double compute_corner_angle(const math::vec3& vertex,
                                                  const math::vec3& adjacent0,
                                                  const math::vec3& adjacent1) noexcept
        {
            const math::vec3 edge0 = adjacent0 - vertex;
            const math::vec3 edge1 = adjacent1 - vertex;
            const float length0 = math::length(edge0);
            const float length1 = math::length(edge1);
            if (length0 <= std::numeric_limits<float>::epsilon() ||
                length1 <= std::numeric_limits<float>::epsilon())
            {
                return std::numbers::pi_v<double> / 3.0;
            }

            const float dot = math::dot(edge0 / length0, edge1 / length1);
            const float clamped = std::clamp(dot, -1.0F, 1.0F);
            return static_cast<double>(std::acos(clamped));
        }

        struct ConstraintRow
        {
            std::vector<std::pair<std::size_t, double>> entries{};
            double target{0.0};
        };

        [[nodiscard]] double dot_rows(const ConstraintRow& a, const ConstraintRow& b) noexcept
        {
            double sum = 0.0;
            std::size_t index_a = 0U;
            std::size_t index_b = 0U;

            while (index_a < a.entries.size() && index_b < b.entries.size())
            {
                const auto& entry_a = a.entries[index_a];
                const auto& entry_b = b.entries[index_b];
                if (entry_a.first == entry_b.first)
                {
                    sum += entry_a.second * entry_b.second;
                    ++index_a;
                    ++index_b;
                }
                else if (entry_a.first < entry_b.first)
                {
                    ++index_a;
                }
                else
                {
                    ++index_b;
                }
            }

            return sum;
        }

        [[nodiscard]] RemeshResult<ParameterizationSummary> generate_planar_parameterization(
            SurfaceMesh& mesh,
            const ParameterizationPolicy& policy) noexcept
        {
            const std::size_t vertex_count = mesh.positions.size();
            mesh.texture_coordinates.assign(vertex_count, math::vec2{0.0F, 0.0F});

            if (vertex_count == 0U)
            {
                return RemeshResult<ParameterizationSummary>{ParameterizationSummary{}};
            }

            const math::vec3 origin = mesh.positions.front();
            math::vec3 axis_x{1.0F, 0.0F, 0.0F};
            for (std::size_t index = 1U; index < vertex_count; ++index)
            {
                axis_x = mesh.positions[index] - origin;
                const float length_sq = math::dot(axis_x, axis_x);
                if (length_sq > std::numeric_limits<float>::epsilon())
                {
                    axis_x /= std::sqrt(length_sq);
                    break;
                }
            }

            math::vec3 normal{0.0F, 1.0F, 0.0F};
            for (std::size_t index = 2U; index < vertex_count; ++index)
            {
                const math::vec3 candidate = math::cross(axis_x, mesh.positions[index] - origin);
                const float length_sq = math::dot(candidate, candidate);
                if (length_sq > std::numeric_limits<float>::epsilon())
                {
                    normal = candidate / std::sqrt(length_sq);
                    break;
                }
            }

            math::vec3 axis_y = math::cross(normal, axis_x);
            const float axis_y_length_sq = math::dot(axis_y, axis_y);
            if (axis_y_length_sq > std::numeric_limits<float>::epsilon())
            {
                axis_y /= std::sqrt(axis_y_length_sq);
            }
            else
            {
                axis_y = math::vec3{0.0F, 0.0F, 1.0F};
            }
            for (std::size_t index = 0U; index < vertex_count; ++index)
            {
                const math::vec3 relative = mesh.positions[index] - origin;
                mesh.texture_coordinates[index] = math::vec2{math::dot(relative, axis_x), math::dot(relative, axis_y)};
            }

            const ParameterizationSummary summary = finalize_parameterization(mesh, policy);
            return RemeshResult<ParameterizationSummary>{summary};
        }

        [[nodiscard]] RemeshResult<ParameterizationSummary> generate_lscm_parameterization(
            SurfaceMesh& mesh,
            const ParameterizationPolicy& policy) noexcept
        {
            const std::size_t vertex_count = mesh.positions.size();
            if (vertex_count < 2U || mesh.indices.size() < 3U)
            {
                mesh.texture_coordinates.assign(vertex_count, math::vec2{0.0F, 0.0F});
                return RemeshResult<ParameterizationSummary>{ParameterizationSummary{}};
            }

            const SurfaceTopologySummary topology_summary = AnalyzeSurfaceTopology(mesh);
            const std::optional<ParameterizationAnchors> anchors =
                select_parameterization_anchors(mesh, topology_summary);
            if (!anchors.has_value())
            {
                return generate_planar_parameterization(mesh, policy);
            }

            std::vector<std::complex<double>> complex_matrix(vertex_count * vertex_count, std::complex<double>{0.0, 0.0});

            const auto accumulate_entry = [&](std::uint32_t a,
                                              std::uint32_t b,
                                              const std::complex<double>& contribution)
            {
                complex_matrix[static_cast<std::size_t>(a) * vertex_count + b] += contribution;
            };

            for (std::size_t triangle = 0U; triangle + 2U < mesh.indices.size(); triangle += 3U)
            {
                const std::uint32_t i0 = mesh.indices[triangle + 0U];
                const std::uint32_t i1 = mesh.indices[triangle + 1U];
                const std::uint32_t i2 = mesh.indices[triangle + 2U];

                if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
                {
                    continue;
                }

                const math::vec3& p0 = mesh.positions[i0];
                const math::vec3& p1 = mesh.positions[i1];
                const math::vec3& p2 = mesh.positions[i2];

                const math::vec3 e1 = p1 - p0;
                const math::vec3 e2 = p2 - p0;
                const math::vec3 normal = math::cross(e1, e2);
                const float normal_length_sq = math::dot(normal, normal);
                if (normal_length_sq <= std::numeric_limits<float>::epsilon())
                {
                    continue;
                }

                math::vec3 x_axis = e1;
                const float e1_length_sq = math::dot(x_axis, x_axis);
                if (e1_length_sq <= std::numeric_limits<float>::epsilon())
                {
                    continue;
                }
                x_axis /= std::sqrt(e1_length_sq);
                const math::vec3 z_axis = normal / std::sqrt(normal_length_sq);
                const math::vec3 y_axis = math::normalize(math::cross(z_axis, x_axis));

                const std::complex<double> z0{0.0, 0.0};
                const std::complex<double> z1{math::dot(e1, x_axis), math::dot(e1, y_axis)};
                const std::complex<double> z2{math::dot(e2, x_axis), math::dot(e2, y_axis)};

                const std::complex<double> z10 = z1 - z0;
                const std::complex<double> z20 = z2 - z0;
                const double area = 0.5 * std::imag(std::conj(z10) * z20);
                if (area <= std::numeric_limits<double>::epsilon())
                {
                    continue;
                }

                const double inv_area = 1.0 / (2.0 * area);
                const std::complex<double> c0 = (z1 - z2) * inv_area;
                const std::complex<double> c1 = (z2 - z0) * inv_area;
                const std::complex<double> c2 = (z0 - z1) * inv_area;

                const std::array<std::uint32_t, 3U> indices{i0, i1, i2};
                const std::array<std::complex<double>, 3U> coefficients{c0, c1, c2};

                for (std::size_t a = 0U; a < coefficients.size(); ++a)
                {
                    for (std::size_t b = 0U; b < coefficients.size(); ++b)
                    {
                        const std::complex<double> contribution = area * coefficients[a] * std::conj(coefficients[b]);
                        accumulate_entry(indices[a], indices[b], contribution);
                    }
                }
            }

            const std::size_t order = vertex_count * 2U;
            std::vector<double> matrix(order * order, 0.0);
            std::vector<double> rhs(order, 0.0);

            for (std::size_t row = 0U; row < vertex_count; ++row)
            {
                for (std::size_t column = 0U; column < vertex_count; ++column)
                {
                    const std::complex<double>& value = complex_matrix[row * vertex_count + column];
                    const double real = value.real();
                    const double imag = value.imag();

                    const std::size_t u_row = row * 2U;
                    const std::size_t v_row = u_row + 1U;
                    const std::size_t u_col = column * 2U;
                    const std::size_t v_col = u_col + 1U;

                    matrix[u_row * order + u_col] += real;
                    matrix[u_row * order + v_col] -= imag;
                    matrix[v_row * order + u_col] += imag;
                    matrix[v_row * order + v_col] += real;
                }
            }

            const auto apply_anchor = [&](std::uint32_t vertex, double u_value, double v_value)
            {
                const std::size_t u_index = static_cast<std::size_t>(vertex) * 2U;
                const std::size_t v_index = u_index + 1U;

                for (std::size_t row_index = 0U; row_index < order; ++row_index)
                {
                    rhs[row_index] -= matrix[row_index * order + u_index] * u_value;
                    rhs[row_index] -= matrix[row_index * order + v_index] * v_value;
                }

                for (std::size_t column = 0U; column < order; ++column)
                {
                    matrix[u_index * order + column] = 0.0;
                    matrix[v_index * order + column] = 0.0;
                }
                for (std::size_t row_index = 0U; row_index < order; ++row_index)
                {
                    matrix[row_index * order + u_index] = 0.0;
                    matrix[row_index * order + v_index] = 0.0;
                }

                matrix[u_index * order + u_index] = 1.0;
                matrix[v_index * order + v_index] = 1.0;
                rhs[u_index] = u_value;
                rhs[v_index] = v_value;
            };

            apply_anchor(anchors->primary, 0.0, 0.0);
            apply_anchor(anchors->secondary, 1.0, 0.0);

            const std::optional<std::vector<double>> solution = solve_dense_linear_system(matrix, rhs, order);
            if (!solution.has_value())
            {
                return generate_planar_parameterization(mesh, policy);
            }

            mesh.texture_coordinates.resize(vertex_count, math::vec2{0.0F, 0.0F});
            for (std::size_t index = 0U; index < vertex_count; ++index)
            {
                const double u = solution.value()[index * 2U + 0U];
                const double v = solution.value()[index * 2U + 1U];
                mesh.texture_coordinates[index] = math::vec2{static_cast<float>(u), static_cast<float>(v)};
            }

            return RemeshResult<ParameterizationSummary>{finalize_parameterization(mesh, policy)};
        }

        [[nodiscard]] RemeshResult<ParameterizationSummary> generate_abfpp_parameterization(
            SurfaceMesh& mesh,
            const ParameterizationPolicy& policy) noexcept
        {
            const std::size_t vertex_count = mesh.positions.size();
            const std::size_t triangle_count = mesh.indices.size() / 3U;

            if (vertex_count < 2U || triangle_count == 0U)
            {
                mesh.texture_coordinates.assign(vertex_count, math::vec2{0.0F, 0.0F});
                return RemeshResult<ParameterizationSummary>{ParameterizationSummary{}};
            }

            const SurfaceTopologySummary topology_summary = AnalyzeSurfaceTopology(mesh);

            std::vector<double> original_angles(triangle_count * 3U, std::numbers::pi_v<double> / 3.0);
            std::vector<std::vector<std::size_t>> vertex_angle_indices(vertex_count);
            std::unordered_map<std::uint64_t, std::vector<std::size_t>> edge_to_faces;
            edge_to_faces.reserve(triangle_count * 3U);

            std::size_t seed_face = triangle_count;
            math::vec3 reference_normal{0.0F, 0.0F, 0.0F};

            for (std::size_t face_index = 0U; face_index < triangle_count; ++face_index)
            {
                const std::uint32_t i0 = mesh.indices[face_index * 3U + 0U];
                const std::uint32_t i1 = mesh.indices[face_index * 3U + 1U];
                const std::uint32_t i2 = mesh.indices[face_index * 3U + 2U];

                if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
                {
                    return generate_planar_parameterization(mesh, policy);
                }

                const math::vec3& p0 = mesh.positions[i0];
                const math::vec3& p1 = mesh.positions[i1];
                const math::vec3& p2 = mesh.positions[i2];

                const math::vec3 normal = math::cross(p1 - p0, p2 - p0);
                const float normal_length_sq = math::dot(normal, normal);
                if (seed_face == triangle_count && normal_length_sq > std::numeric_limits<float>::epsilon())
                {
                    seed_face = face_index;
                    reference_normal = normal / std::sqrt(normal_length_sq);
                }

                const double angle0 = compute_corner_angle(p0, p1, p2);
                const double angle1 = compute_corner_angle(p1, p2, p0);
                const double angle2 = compute_corner_angle(p2, p0, p1);

                original_angles[face_index * 3U + 0U] = angle0;
                original_angles[face_index * 3U + 1U] = angle1;
                original_angles[face_index * 3U + 2U] = angle2;

                vertex_angle_indices[i0].push_back(face_index * 3U + 0U);
                vertex_angle_indices[i1].push_back(face_index * 3U + 1U);
                vertex_angle_indices[i2].push_back(face_index * 3U + 2U);

                edge_to_faces[make_edge_key(i0, i1)].push_back(face_index);
                edge_to_faces[make_edge_key(i1, i2)].push_back(face_index);
                edge_to_faces[make_edge_key(i2, i0)].push_back(face_index);
            }

            if (seed_face == triangle_count)
            {
                return generate_planar_parameterization(mesh, policy);
            }

            std::vector<ConstraintRow> constraints;
            constraints.reserve(triangle_count + vertex_count);
            const double pi = std::numbers::pi_v<double>;

            for (std::size_t face_index = 0U; face_index < triangle_count; ++face_index)
            {
                ConstraintRow row{};
                row.entries.emplace_back(face_index * 3U + 0U, 1.0);
                row.entries.emplace_back(face_index * 3U + 1U, 1.0);
                row.entries.emplace_back(face_index * 3U + 2U, 1.0);
                row.target = pi;
                constraints.push_back(std::move(row));
            }

            for (std::size_t vertex = 0U; vertex < vertex_count; ++vertex)
            {
                const auto& angles = vertex_angle_indices[vertex];
                if (angles.empty())
                {
                    continue;
                }

                ConstraintRow row{};
                row.entries.reserve(angles.size());
                for (const std::size_t index : angles)
                {
                    row.entries.emplace_back(index, 1.0);
                }

                const bool is_boundary = vertex < topology_summary.vertices.size() &&
                                         topology_summary.vertices[vertex].is_boundary;
                row.target = is_boundary ? pi : 2.0 * pi;
                constraints.push_back(std::move(row));
            }

            for (ConstraintRow& row : constraints)
            {
                std::sort(row.entries.begin(), row.entries.end(),
                          [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
            }

            const auto solve_constraints = [&](std::vector<ConstraintRow>& rows)
                -> std::optional<std::vector<double>>
            {
                const std::size_t constraint_count = rows.size();
                if (constraint_count == 0U)
                {
                    return std::vector<double>{};
                }

                std::vector<double> residual(constraint_count, 0.0);
                for (std::size_t row_index = 0U; row_index < constraint_count; ++row_index)
                {
                    double sum = 0.0;
                    for (const auto& entry : rows[row_index].entries)
                    {
                        sum += entry.second * original_angles[entry.first];
                    }
                    residual[row_index] = sum - rows[row_index].target;
                }

                std::vector<double> matrix(constraint_count * constraint_count, 0.0);
                for (std::size_t row = 0U; row < constraint_count; ++row)
                {
                    for (std::size_t column = row; column < constraint_count; ++column)
                    {
                        const double value = dot_rows(rows[row], rows[column]);
                        matrix[row * constraint_count + column] = value;
                        if (row != column)
                        {
                            matrix[column * constraint_count + row] = value;
                        }
                    }
                }

                return solve_dense_linear_system(std::move(matrix), std::move(residual), constraint_count);
            };

            std::optional<std::vector<double>> lambda = solve_constraints(constraints);

            if (!lambda.has_value() && constraints.size() > triangle_count)
            {
                constraints.erase(constraints.begin() + static_cast<std::ptrdiff_t>(triangle_count));
                lambda = solve_constraints(constraints);
            }

            if (!lambda.has_value())
            {
                return generate_planar_parameterization(mesh, policy);
            }

            std::vector<double> adjusted_angles = original_angles;
            const auto& multipliers = lambda.value();
            for (std::size_t row_index = 0U; row_index < constraints.size(); ++row_index)
            {
                const double multiplier = multipliers[row_index];
                if (multiplier == 0.0)
                {
                    continue;
                }

                for (const auto& entry : constraints[row_index].entries)
                {
                    adjusted_angles[entry.first] -= entry.second * multiplier;
                }
            }

            for (double& angle : adjusted_angles)
            {
                if (!std::isfinite(angle))
                {
                    return generate_planar_parameterization(mesh, policy);
                }
                angle = std::clamp(angle, 1e-4, std::numbers::pi_v<double> - 1e-4);
            }

            std::vector<math::vec2> generated(vertex_count, math::vec2{0.0F, 0.0F});
            std::vector<bool> assigned(vertex_count, false);
            std::vector<char> face_processed(triangle_count, 0);
            std::vector<char> face_enqueued(triangle_count, 0);
            std::queue<std::size_t> queue;

            const std::uint32_t seed_v0 = mesh.indices[seed_face * 3U + 0U];
            const std::uint32_t seed_v1 = mesh.indices[seed_face * 3U + 1U];
            const std::uint32_t seed_v2 = mesh.indices[seed_face * 3U + 2U];

            const math::vec3& seed_p0 = mesh.positions[seed_v0];
            const math::vec3& seed_p1 = mesh.positions[seed_v1];
            const math::vec3& seed_p2 = mesh.positions[seed_v2];

            const double seed_len01 = static_cast<double>(math::length(seed_p1 - seed_p0));
            if (seed_len01 <= std::numeric_limits<double>::epsilon())
            {
                return generate_planar_parameterization(mesh, policy);
            }

            const double seed_angle0 = adjusted_angles[seed_face * 3U + 0U];
            const double seed_angle1 = adjusted_angles[seed_face * 3U + 1U];
            const double seed_angle2 = adjusted_angles[seed_face * 3U + 2U];
            const double sin_seed_angle2 = std::sin(seed_angle2);
            if (std::abs(sin_seed_angle2) <= 1e-8)
            {
                return generate_planar_parameterization(mesh, policy);
            }

            const double seed_scale = seed_len01 / sin_seed_angle2;
            const double length_v0v2 = std::sin(seed_angle1) * seed_scale;

            generated[seed_v0] = math::vec2{0.0F, 0.0F};
            generated[seed_v1] = math::vec2{static_cast<float>(seed_len01), 0.0F};
            generated[seed_v2] =
                math::vec2{static_cast<float>(length_v0v2 * std::cos(seed_angle0)),
                           static_cast<float>(length_v0v2 * std::sin(seed_angle0))};
            assigned[seed_v0] = true;
            assigned[seed_v1] = true;
            assigned[seed_v2] = true;
            face_processed[seed_face] = 1;

            const auto enqueue_neighbors = [&](std::size_t face_index)
            {
                const std::array<std::uint32_t, 3U> vertices{
                    mesh.indices[face_index * 3U + 0U],
                    mesh.indices[face_index * 3U + 1U],
                    mesh.indices[face_index * 3U + 2U],
                };

                for (std::size_t edge = 0U; edge < 3U; ++edge)
                {
                    const std::uint32_t a = vertices[edge];
                    const std::uint32_t b = vertices[(edge + 1U) % 3U];
                    const std::uint64_t key = make_edge_key(a, b);
                    const auto it = edge_to_faces.find(key);
                    if (it == edge_to_faces.end())
                    {
                        continue;
                    }

                    for (const std::size_t neighbor : it->second)
                    {
                        if (neighbor == face_index || face_processed[neighbor])
                        {
                            continue;
                        }
                        if (!face_enqueued[neighbor])
                        {
                            queue.push(neighbor);
                            face_enqueued[neighbor] = 1;
                        }
                    }
                }
            };

            enqueue_neighbors(seed_face);

            std::size_t guard = 0U;
            const std::size_t guard_limit = std::max<std::size_t>(triangle_count * 8U, 64U);

            while (!queue.empty() && guard < guard_limit)
            {
                const std::size_t face_index = queue.front();
                queue.pop();
                face_enqueued[face_index] = 0;
                ++guard;

                if (face_processed[face_index])
                {
                    continue;
                }

                const std::array<std::uint32_t, 3U> vertices{
                    mesh.indices[face_index * 3U + 0U],
                    mesh.indices[face_index * 3U + 1U],
                    mesh.indices[face_index * 3U + 2U],
                };

                std::array<bool, 3U> vertex_assigned{
                    assigned[vertices[0]], assigned[vertices[1]], assigned[vertices[2]]};

                const std::size_t assigned_count = static_cast<std::size_t>(vertex_assigned[0]) +
                                                    static_cast<std::size_t>(vertex_assigned[1]) +
                                                    static_cast<std::size_t>(vertex_assigned[2]);

                if (assigned_count < 2U)
                {
                    queue.push(face_index);
                    face_enqueued[face_index] = 1;
                    continue;
                }

                if (assigned_count == 3U)
                {
                    face_processed[face_index] = 1;
                    enqueue_neighbors(face_index);
                    continue;
                }

                int unassigned_corner = -1;
                for (int corner = 0; corner < 3; ++corner)
                {
                    if (!vertex_assigned[corner])
                    {
                        unassigned_corner = corner;
                        break;
                    }
                }

                if (unassigned_corner < 0)
                {
                    face_processed[face_index] = 1;
                    enqueue_neighbors(face_index);
                    continue;
                }

                const int corner_a = (unassigned_corner + 1) % 3;
                const int corner_b = (unassigned_corner + 2) % 3;

                const std::uint32_t vertex_u = vertices[static_cast<std::size_t>(unassigned_corner)];
                const std::uint32_t vertex_a = vertices[static_cast<std::size_t>(corner_a)];
                const std::uint32_t vertex_b = vertices[static_cast<std::size_t>(corner_b)];

                const math::vec2& base_a = generated[vertex_a];
                const math::vec2& base_b = generated[vertex_b];
                const math::vec2 base_vector = base_b - base_a;
                const double base_length = static_cast<double>(math::length(base_vector));
                if (base_length <= std::numeric_limits<double>::epsilon())
                {
                    return generate_planar_parameterization(mesh, policy);
                }

                const double angle_u = adjusted_angles[face_index * 3U + static_cast<std::size_t>(unassigned_corner)];
                const double angle_a = adjusted_angles[face_index * 3U + static_cast<std::size_t>(corner_a)];
                const double angle_b = adjusted_angles[face_index * 3U + static_cast<std::size_t>(corner_b)];

                const double sin_angle_u = std::sin(angle_u);
                if (std::abs(sin_angle_u) <= 1e-8)
                {
                    return generate_planar_parameterization(mesh, policy);
                }

                const double scale = base_length / sin_angle_u;
                const double radius_a = std::sin(angle_b) * scale;
                const double radius_b = std::sin(angle_a) * scale;

                math::vec2 dir_unit = base_vector / static_cast<float>(base_length);
                math::vec2 perp_dir{-dir_unit[1], dir_unit[0]};

                const math::vec3 normal = math::cross(mesh.positions[vertex_b] - mesh.positions[vertex_a],
                                                       mesh.positions[vertex_u] - mesh.positions[vertex_a]);
                const double orientation = static_cast<double>(math::dot(normal, reference_normal));
                if (orientation < 0.0)
                {
                    perp_dir *= -1.0F;
                }

                const double d = base_length;
                const double numerator = (radius_a * radius_a - radius_b * radius_b + d * d);
                const double a = numerator / (2.0 * d);
                const double h_sq = std::max(radius_a * radius_a - a * a, 0.0);
                const double h = std::sqrt(h_sq);

                const math::vec2 point2 = base_a + dir_unit * static_cast<float>(a);
                const math::vec2 new_position = point2 + perp_dir * static_cast<float>(h);

                generated[vertex_u] = new_position;
                assigned[vertex_u] = true;
                face_processed[face_index] = 1;
                enqueue_neighbors(face_index);
            }

            if (!std::all_of(assigned.begin(), assigned.end(), [](bool value) { return value; }))
            {
                return generate_planar_parameterization(mesh, policy);
            }

            mesh.texture_coordinates = generated;

            return RemeshResult<ParameterizationSummary>{finalize_parameterization(mesh, policy)};
        }
    } // namespace

    RemeshValidationResult ValidateRemeshRequest(const RemeshRequest& request) noexcept
    {
        if (request.input_mesh == nullptr)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "remesh request missing input mesh")};
        }

        if (request.input_mesh->positions.empty())
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "input mesh must contain vertices")};
        }

        if (!request.input_mesh->indices.empty() && request.input_mesh->indices.size() % 3 != 0)
        {
            return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                            "triangle index buffer must be a multiple of three")};
        }

        for (const std::uint32_t index : request.input_mesh->indices)
        {
            if (index >= request.input_mesh->positions.size())
            {
                return RemeshValidationResult{make_remesh_error(RemeshError::invalid_input_mesh,
                                                                "triangle index out of range")};
            }
        }

        if (request.max_iterations == 0U)
        {
            return make_target_error("max_iterations must be greater than zero");
        }

        if (!is_positive_finite(request.relaxation_factor) || request.relaxation_factor > 1.0f)
        {
            return make_target_error("relaxation_factor must be within (0, 1]");
        }

        if (!is_non_negative_finite(request.tangential_smoothing_weight) ||
            request.tangential_smoothing_weight > 1.0f)
        {
            return make_target_error("tangential_smoothing_weight must be within [0, 1]");
        }

        bool has_primary_target = false;
        if (request.targets.target_edge_length.has_value())
        {
            const float value = request.targets.target_edge_length.value();
            if (!is_positive_finite(value))
            {
                return make_target_error("target_edge_length must be positive and finite");
            }
            has_primary_target = true;
        }

        if (request.targets.relative_edge_scale.has_value())
        {
            const float value = request.targets.relative_edge_scale.value();
            if (!is_positive_finite(value))
            {
                return make_target_error("relative_edge_scale must be positive and finite");
            }
            has_primary_target = true;
        }

        bool has_adaptive_budget = false;
        if (request.targets.maximum_normal_deviation_degrees.has_value())
        {
            const float value = request.targets.maximum_normal_deviation_degrees.value();
            if (!std::isfinite(value) || value <= 0.0f || value > 180.0f)
            {
                return make_target_error("maximum_normal_deviation_degrees must lie within (0, 180]");
            }
            has_adaptive_budget = true;
        }

        if (request.targets.maximum_surface_deviation.has_value())
        {
            const float value = request.targets.maximum_surface_deviation.value();
            if (!is_non_negative_finite(value))
            {
                return make_target_error("maximum_surface_deviation must be non-negative and finite");
            }
            has_adaptive_budget = true;
        }

        switch (request.mode)
        {
        case RemeshingMode::kUniform:
            if (!has_primary_target)
            {
                return make_target_error("uniform remeshing requires an edge length target");
            }
            break;
        case RemeshingMode::kFeaturePreserving:
            if (!has_primary_target)
            {
                return make_target_error("feature-preserving remeshing requires an edge length target");
            }
            if (!std::isfinite(request.feature_preservation.minimum_feature_angle_degrees) ||
                request.feature_preservation.minimum_feature_angle_degrees <= 0.0f ||
                request.feature_preservation.minimum_feature_angle_degrees >= 180.0f)
            {
                return make_target_error("minimum_feature_angle_degrees must lie within (0, 180)");
            }
            break;
        case RemeshingMode::kAdaptive:
            if (!has_primary_target && !has_adaptive_budget)
            {
                return make_target_error("adaptive remeshing requires either edge targets or error budgets");
            }
            if (!has_adaptive_budget)
            {
                return make_target_error("adaptive remeshing requires an error budget");
            }
            break;
        }

        if (request.attribute_policy.positions != AttributeTransferMode::kPreserve)
        {
            return make_attribute_error("position attributes must be preserved during remeshing");
        }

        if (request.attribute_policy.skinning_weights == AttributeTransferMode::kDrop)
        {
            return make_attribute_error("skinning_weights cannot be dropped during remeshing");
        }

        if (request.parameterization.mode == ParameterizationMode::kReuseExisting &&
            request.attribute_policy.texture_coordinates == AttributeTransferMode::kDrop)
        {
            return make_attribute_error(
                "texture coordinates cannot be dropped when parameterisation reuses existing data");
        }

        if (!is_non_negative_finite(request.parameterization.gutter_width))
        {
            return make_parameterization_error("gutter_width must be non-negative and finite");
        }

        switch (request.parameterization.mode)
        {
        case ParameterizationMode::kNone:
            break;
        case ParameterizationMode::kReuseExisting:
            if (!is_non_negative_finite(request.parameterization.target_texel_density))
            {
                return make_parameterization_error(
                    "target_texel_density must be non-negative when reusing parameterisation");
            }
            break;
        case ParameterizationMode::kGenerateLscm:
        case ParameterizationMode::kGenerateAbfpp:
            if (!is_positive_finite(request.parameterization.target_texel_density))
            {
                return make_parameterization_error(
                    "target_texel_density must be positive when generating parameterisation");
            }
            break;
        }

        return RemeshValidationResult{};
    }

    MeshEdgeStatistics ComputeMeshEdgeStatistics(const SurfaceMesh& mesh) noexcept
    {
        MeshEdgeStatistics statistics{};

        if (mesh.positions.size() < 2U || mesh.indices.empty())
        {
            statistics.min_edge_length = 0.0f;
            return statistics;
        }

        std::unordered_set<std::uint64_t> unique_edges{};
        unique_edges.reserve(mesh.indices.size());

        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const std::uint32_t a = mesh.indices[i];
            const std::uint32_t b = mesh.indices[i + 1];
            const std::uint32_t c = mesh.indices[i + 2];

            const std::array<std::pair<std::uint32_t, std::uint32_t>, 3> edges{{
                {a, b},
                {b, c},
                {c, a},
            }};

            for (const auto& [u, v] : edges)
            {
                if (u == v)
                {
                    continue;
                }

                const std::uint64_t key = make_edge_key(u, v);
                if (!unique_edges.insert(key).second)
                {
                    continue;
                }

                const math::vec3& p0 = mesh.positions[u];
                const math::vec3& p1 = mesh.positions[v];
                const float length = math::length(p1 - p0);

                statistics.total_edge_length += length;
                statistics.max_edge_length = std::max(statistics.max_edge_length, length);
                statistics.min_edge_length = std::min(statistics.min_edge_length, length);
                ++statistics.edge_count;
            }
        }

        if (statistics.edge_count == 0U)
        {
            statistics.min_edge_length = 0.0f;
            statistics.max_edge_length = 0.0f;
            statistics.total_edge_length = 0.0f;
        }

        return statistics;
    }

    RemeshResult<ResolvedRemeshingTargets> ResolveRemeshingTargets(const RemeshRequest& request) noexcept
    {
        if (const RemeshValidationResult validation = ValidateRemeshRequest(request); !validation.has_value())
        {
            return RemeshResult<ResolvedRemeshingTargets>{validation.error()};
        }

        ResolvedRemeshingTargets resolved{};
        resolved.edge_statistics = ComputeMeshEdgeStatistics(*request.input_mesh);

        if (resolved.edge_statistics.edge_count == 0U)
        {
            resolved.edge_statistics.min_edge_length = 0.0f;
        }

        if (request.targets.maximum_normal_deviation_degrees.has_value())
        {
            resolved.maximum_normal_deviation_degrees = request.targets.maximum_normal_deviation_degrees;
        }

        if (request.targets.maximum_surface_deviation.has_value())
        {
            resolved.maximum_surface_deviation = request.targets.maximum_surface_deviation;
        }

        if (request.targets.target_edge_length.has_value())
        {
            resolved.target_edge_length = request.targets.target_edge_length;
        }

        if (request.targets.relative_edge_scale.has_value())
        {
            const float mean_length = resolved.edge_statistics.mean_edge_length();
            if (mean_length <= std::numeric_limits<float>::min())
            {
                return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                    RemeshError::invalid_target_configuration,
                    "relative_edge_scale requires a mesh with non-degenerate edges")};
            }

            const float derived_edge_length =
                mean_length * request.targets.relative_edge_scale.value();

            if (resolved.target_edge_length.has_value())
            {
                if (!math::utils::nearly_equal(resolved.target_edge_length.value(), derived_edge_length, 1e-4f))
                {
                    return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                        RemeshError::invalid_target_configuration,
                        "absolute and relative edge targets disagree")};
                }
            }
            else
            {
                resolved.target_edge_length = derived_edge_length;
            }
        }

        if (!resolved.target_edge_length.has_value() && request.mode != RemeshingMode::kAdaptive)
        {
            // Validation should have caught this already, but provide a defensive guard for callers
            // that bypass ValidateRemeshRequest.
            return RemeshResult<ResolvedRemeshingTargets>{make_remesh_error(
                RemeshError::invalid_target_configuration,
                "remeshing mode requires an absolute edge length target")};
        }

        return RemeshResult<ResolvedRemeshingTargets>{resolved};
    }

    namespace
    {
        constexpr float kDefaultSplitThreshold = 4.0F / 3.0F;
        constexpr float kDefaultCollapseThreshold = 4.0F / 5.0F;

        [[nodiscard]] std::vector<bool> initialise_locked_vertices(const SurfaceTopologySummary& summary,
                                                                   MeshInterface& interface,
                                                                   const RemeshRequest& request)
        {
            std::vector<bool> locked(interface.vertices_size(), false);

            const bool lock_boundaries = request.feature_preservation.lock_boundary_edges;
            const bool lock_features = request.feature_preservation.lock_feature_edges;

            for (std::size_t index = 0; index < summary.vertices.size() && index < locked.size(); ++index)
            {
                const SurfaceVertexTag& tag = summary.vertices[index];
                if ((lock_boundaries && tag.is_boundary) || (lock_features && tag.is_feature))
                {
                    locked[index] = true;
                }
            }

            return locked;
        }

        [[nodiscard]] bool is_locked_vertex(const std::vector<bool>& locked, VertexHandle vertex) noexcept
        {
            const std::size_t index = vertex.index();
            if (index >= locked.size())
            {
                return false;
            }
            return locked[index];
        }

        void ensure_vertex_capacity(std::vector<bool>& locked, const MeshInterface& interface)
        {
            if (locked.size() < interface.vertices_size())
            {
                locked.resize(interface.vertices_size(), false);
            }
        }

        void laplacian_relaxation(MeshInterface& interface,
                                  const std::vector<bool>& locked,
                                  float factor,
                                  const std::vector<math::vec3>* vertex_normals,
                                  VertexProperty<math::vec3>* rest_positions)
        {
            if (factor <= 0.0F)
            {
                return;
            }

            std::vector<math::vec3> updated(interface.vertices_size(), math::vec3{0.0F});
            std::vector<bool> has_update(interface.vertices_size(), false);

            for (auto vertex : interface.vertices())
            {
                if (interface.is_deleted(vertex))
                {
                    continue;
                }

                if (is_locked_vertex(locked, vertex))
                {
                    continue;
                }

                auto neighbors = interface.vertices(vertex);
                if (!neighbors)
                {
                    continue;
                }

                const auto start = neighbors;
                math::vec3 sum{0.0F};
                std::size_t count = 0U;
                do
                {
                    const VertexHandle neighbor = *neighbors;
                    if (!interface.is_deleted(neighbor))
                    {
                        sum += interface.position(neighbor);
                        ++count;
                    }
                }
                while (++neighbors != start);

                if (count == 0U)
                {
                    continue;
                }

                const math::vec3 current = interface.position(vertex);
                const math::vec3 average = sum / static_cast<float>(count);
                math::vec3 displacement = average - current;

                if (vertex_normals != nullptr && vertex.index() < vertex_normals->size())
                {
                    const math::vec3& normal = (*vertex_normals)[vertex.index()];
                    const float normal_length_sq = math::dot(normal, normal);
                    if (normal_length_sq > std::numeric_limits<float>::epsilon())
                    {
                        displacement -= math::dot(displacement, normal) * normal;
                    }
                }

                const math::vec3 next_position = current + factor * displacement;
                updated[vertex.index()] = next_position;
                (void)rest_positions;
                has_update[vertex.index()] = true;
            }

            for (auto vertex : interface.vertices())
            {
                if (interface.is_deleted(vertex))
                {
                    continue;
                }

                const std::size_t index = vertex.index();
                if (index >= has_update.size() || !has_update[index])
                {
                    continue;
                }

                interface.position(vertex) = updated[index];
                if (rest_positions != nullptr)
                {
                    (*rest_positions)[vertex] = updated[index];
                }
            }
        }

        [[nodiscard]] float compute_edge_length(const MeshInterface& interface, EdgeHandle edge) noexcept
        {
            const HalfedgeHandle h0 = interface.halfedge(edge, 0);
            const HalfedgeHandle h1 = interface.halfedge(edge, 1);
            const math::vec3& p0 = interface.position(interface.to_vertex(h0));
            const math::vec3& p1 = interface.position(interface.to_vertex(h1));
            return math::length(p1 - p0);
        }

        [[nodiscard]] std::optional<math::vec3> compute_face_unit_normal(const MeshInterface& interface,
                                                                         FaceHandle face) noexcept
        {
            if (!face.is_valid() || interface.is_deleted(face))
            {
                return std::nullopt;
            }

            auto vertices = interface.vertices(face);
            if (!vertices)
            {
                return std::nullopt;
            }

            const auto start = vertices;
            std::array<math::vec3, 3> points{};
            std::size_t count = 0U;

            do
            {
                const VertexHandle vertex = *vertices;
                points[count++] = interface.position(vertex);
            }
            while (++vertices != start && count < points.size());

            if (count < 3U)
            {
                return std::nullopt;
            }

            const math::vec3 e0 = points[1] - points[0];
            const math::vec3 e1 = points[2] - points[0];
            const math::vec3 normal = math::cross(e0, e1);
            const float length_sq = math::dot(normal, normal);
            if (length_sq <= std::numeric_limits<float>::epsilon())
            {
                return std::nullopt;
            }

            return normal / std::sqrt(length_sq);
        }

        [[nodiscard]] float compute_edge_dihedral_degrees(const MeshInterface& interface, EdgeHandle edge) noexcept
        {
            const HalfedgeHandle h0 = interface.halfedge(edge, 0);
            const HalfedgeHandle h1 = interface.halfedge(edge, 1);

            const FaceHandle f0 = interface.face(h0);
            const FaceHandle f1 = interface.face(h1);

            const std::optional<math::vec3> n0 = compute_face_unit_normal(interface, f0);
            const std::optional<math::vec3> n1 = compute_face_unit_normal(interface, f1);

            if (!n0.has_value() || !n1.has_value())
            {
                return 0.0F;
            }

            const float dot_value = math::dot(n0.value(), n1.value());
            const float clamped = std::clamp(dot_value, -1.0F, 1.0F);
            const float angle_radians = std::acos(clamped);
            return angle_radians * (180.0F / std::numbers::pi_v<float>);
        }

        bool can_collapse_edge(const RemeshRequest& request,
                               const std::vector<bool>& locked,
                               MeshInterface& interface,
                               EdgeHandle edge,
                               HalfedgeHandle& collapse_halfedge,
                               VertexHandle& keep_vertex,
                               VertexHandle& remove_vertex,
                               const std::unordered_set<std::uint64_t>* protected_edges)
        {
            if (interface.is_deleted(edge))
            {
                return false;
            }

            if (request.feature_preservation.lock_boundary_edges && interface.is_boundary(edge))
            {
                return false;
            }

            HalfedgeHandle candidate = interface.halfedge(edge, 0);
            HalfedgeHandle opposite = interface.opposite_halfedge(candidate);

            VertexHandle keep = interface.to_vertex(candidate);
            VertexHandle remove = interface.to_vertex(opposite);

            if (protected_edges != nullptr)
            {
                const std::uint64_t key = make_edge_key(keep.index(), remove.index());
                if (protected_edges->find(key) != protected_edges->end())
                {
                    return false;
                }
            }

            if (is_locked_vertex(locked, remove) && !is_locked_vertex(locked, keep))
            {
                candidate = interface.halfedge(edge, 1);
                opposite = interface.opposite_halfedge(candidate);
                keep = interface.to_vertex(candidate);
                remove = interface.to_vertex(opposite);

                if (protected_edges != nullptr)
                {
                    const std::uint64_t key = make_edge_key(keep.index(), remove.index());
                    if (protected_edges->find(key) != protected_edges->end())
                    {
                        return false;
                    }
                }
            }

            if (is_locked_vertex(locked, keep) || is_locked_vertex(locked, remove))
            {
                return false;
            }

            if (!interface.is_collapse_ok(candidate))
            {
                return false;
            }

            collapse_halfedge = candidate;
            keep_vertex = keep;
            remove_vertex = remove;
            return true;
        }

        std::uint32_t execute_uniform_remesh(const RemeshRequest& request,
                                             MeshInterface& interface,
                                             std::vector<bool>& locked,
                                             float target_length,
                                             float split_threshold,
                                             float collapse_threshold,
                                             const std::unordered_set<std::uint64_t>* protected_edges,
                                             bool tangential_smoothing,
                                             VertexProperty<math::vec3>* rest_positions,
                                             VertexProperty<math::vec2>* texture_coordinates,
                                             AttributeTransferMode texture_mode,
                                             RemeshOperationCounters* counters)
        {
            std::uint32_t performed_iterations = 0U;
            const float smoothing_factor = request.relaxation_factor * request.tangential_smoothing_weight;
            const bool resample_texture = texture_coordinates != nullptr &&
                                          texture_mode != AttributeTransferMode::kDrop;
            const bool update_rest_positions = rest_positions != nullptr;

            for (; performed_iterations < request.max_iterations; ++performed_iterations)
            {
                std::vector<EdgeHandle> edges_to_split;
                std::vector<EdgeHandle> edges_to_collapse;

                for (auto edge : interface.edges())
                {
                    if (interface.is_deleted(edge))
                    {
                        continue;
                    }

                    const HalfedgeHandle h0 = interface.halfedge(edge, 0);
                    const HalfedgeHandle h1 = interface.halfedge(edge, 1);
                    const math::vec3& p0 = interface.position(interface.to_vertex(h0));
                    const math::vec3& p1 = interface.position(interface.to_vertex(h1));
                    const float length = math::length(p0 - p1);

                    if (!std::isfinite(length) || length <= std::numeric_limits<float>::epsilon())
                    {
                        continue;
                    }

                    if (length > target_length * split_threshold)
                    {
                        edges_to_split.push_back(edge);
                    }
                    else if (length < target_length * collapse_threshold)
                    {
                        if (protected_edges != nullptr)
                        {
                            const std::uint64_t key =
                                make_edge_key(interface.to_vertex(h0).index(), interface.to_vertex(h1).index());
                            if (protected_edges->find(key) != protected_edges->end())
                            {
                                continue;
                            }
                        }
                        edges_to_collapse.push_back(edge);
                    }
                }

                bool iteration_changed = false;

                for (EdgeHandle edge : edges_to_collapse)
                {
                    if (interface.is_deleted(edge))
                    {
                        continue;
                    }

                    const HalfedgeHandle h0 = interface.halfedge(edge, 0);
                    const HalfedgeHandle h1 = interface.halfedge(edge, 1);
                    const math::vec3& p0 = interface.position(interface.to_vertex(h0));
                    const math::vec3& p1 = interface.position(interface.to_vertex(h1));
                    const float length = math::length(p0 - p1);

                    if (!(length < target_length * collapse_threshold))
                    {
                        continue;
                    }

                    HalfedgeHandle collapse_halfedge{};
                    VertexHandle keep_vertex{};
                    VertexHandle remove_vertex{};

                    if (!can_collapse_edge(request,
                                           locked,
                                           interface,
                                           edge,
                                           collapse_halfedge,
                                           keep_vertex,
                                           remove_vertex,
                                           protected_edges))
                    {
                        continue;
                    }

                    if (update_rest_positions)
                    {
                        update_collapse_rest_position(*rest_positions, keep_vertex, remove_vertex);
                    }

                    if (resample_texture)
                    {
                        update_collapse_uv(*texture_coordinates, keep_vertex, remove_vertex);
                    }

                    interface.position(keep_vertex) = (interface.position(keep_vertex) + interface.position(remove_vertex)) * 0.5F;
                    interface.collapse(collapse_halfedge);
                    if (counters != nullptr)
                    {
                        counters->collapse_count += 1U;
                    }
                    iteration_changed = true;
                }

                ensure_vertex_capacity(locked, interface);

                for (EdgeHandle edge : edges_to_split)
                {
                    if (interface.is_deleted(edge))
                    {
                        continue;
                    }

                    const HalfedgeHandle h0 = interface.halfedge(edge, 0);
                    const HalfedgeHandle h1 = interface.halfedge(edge, 1);
                    const math::vec3& p0 = interface.position(interface.to_vertex(h0));
                    const math::vec3& p1 = interface.position(interface.to_vertex(h1));
                    const float length = math::length(p0 - p1);

                    if (!(length > target_length * split_threshold))
                    {
                        continue;
                    }

                    const std::size_t previous_vertices = interface.vertices_size();
                    const auto new_halfedge = interface.split(edge, (p0 + p1) * 0.5F);
                    (void)new_halfedge;
                    ensure_vertex_capacity(locked, interface);

                    if (interface.vertices_size() == previous_vertices + 1)
                    {
                        VertexHandle new_vertex(static_cast<std::uint32_t>(interface.vertices_size() - 1));
                        bool lock_new = false;
                        if (request.feature_preservation.lock_boundary_edges && interface.is_boundary(new_vertex))
                        {
                            lock_new = true;
                        }
                        if (!lock_new && request.feature_preservation.lock_feature_edges)
                        {
                            const bool v0_locked = is_locked_vertex(locked, interface.to_vertex(h0));
                            const bool v1_locked = is_locked_vertex(locked, interface.to_vertex(h1));
                            lock_new = v0_locked && v1_locked;
                        }
                        locked[new_vertex.index()] = lock_new;

                        if (update_rest_positions)
                        {
                            const math::vec3& rest0 = (*rest_positions)[interface.to_vertex(h0)];
                            const math::vec3& rest1 = (*rest_positions)[interface.to_vertex(h1)];
                            assign_interpolated_rest_position(*rest_positions, new_vertex, rest0, rest1);
                        }

                        if (resample_texture)
                        {
                            const math::vec2& uv0 = (*texture_coordinates)[interface.to_vertex(h0)];
                            const math::vec2& uv1 = (*texture_coordinates)[interface.to_vertex(h1)];
                            assign_interpolated_uv(*texture_coordinates, new_vertex, uv0, uv1);
                        }

                        if (counters != nullptr)
                        {
                            counters->split_count += 1U;
                        }
                    }

                    iteration_changed = true;
                }

                if (!iteration_changed)
                {
                    break;
                }

                ensure_vertex_capacity(locked, interface);
                const std::vector<math::vec3>* normals_ptr = nullptr;
                std::vector<math::vec3> normals{};
                if (tangential_smoothing && smoothing_factor > 0.0F)
                {
                    normals = compute_vertex_normals(interface);
                    normals_ptr = &normals;
                }

                laplacian_relaxation(interface, locked, smoothing_factor, normals_ptr, rest_positions);
            }

            return performed_iterations;
        }

        struct AdaptiveRemeshThresholds
        {
            float base_target_length{1.0F};
            float split_length{1.0F};
            float collapse_length{1.0F};
            std::optional<float> max_normal_deviation_degrees{};
            std::optional<float> max_surface_deviation{};
        };

        [[nodiscard]] AdaptiveRemeshThresholds make_adaptive_thresholds(const RemeshRequest& request,
                                                                         const ResolvedRemeshingTargets& resolved)
        {
            AdaptiveRemeshThresholds thresholds{};

            float base = resolved.target_edge_length.value_or(resolved.edge_statistics.mean_edge_length());
            if (!std::isfinite(base) || base <= std::numeric_limits<float>::epsilon())
            {
                base = resolved.edge_statistics.max_edge_length;
            }
            if (!std::isfinite(base) || base <= std::numeric_limits<float>::epsilon())
            {
                base = 1.0F;
            }

            thresholds.base_target_length = base;
            thresholds.split_length = base * kDefaultSplitThreshold;
            thresholds.collapse_length = base * kDefaultCollapseThreshold;

            if (resolved.maximum_surface_deviation.has_value())
            {
                const float deviation = std::max(0.0F, resolved.maximum_surface_deviation.value());
                thresholds.max_surface_deviation = deviation;
                thresholds.split_length = std::min(thresholds.split_length, base + deviation);
            }

            if (thresholds.split_length <= thresholds.collapse_length)
            {
                thresholds.split_length = thresholds.collapse_length + thresholds.base_target_length * 0.25F;
            }

            thresholds.max_normal_deviation_degrees = resolved.maximum_normal_deviation_degrees;

            return thresholds;
        }

        [[nodiscard]] bool should_split_edge(const AdaptiveRemeshThresholds& thresholds,
                                             float length,
                                             float dihedral_degrees) noexcept
        {
            if (length > thresholds.split_length)
            {
                return true;
            }

            if (thresholds.max_surface_deviation.has_value())
            {
                const float deviation = thresholds.max_surface_deviation.value();
                if (deviation > 0.0F && (length - thresholds.base_target_length) > deviation)
                {
                    return true;
                }
            }

            if (thresholds.max_normal_deviation_degrees.has_value())
            {
                const float allowed = thresholds.max_normal_deviation_degrees.value();
                if (allowed > 0.0F && dihedral_degrees > allowed && length > thresholds.base_target_length * 0.5F)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool should_collapse_edge(const AdaptiveRemeshThresholds& thresholds,
                                                float length,
                                                float dihedral_degrees) noexcept
        {
            bool collapse = length < thresholds.collapse_length;

            if (!collapse && thresholds.max_surface_deviation.has_value())
            {
                const float deviation = thresholds.max_surface_deviation.value();
                if (deviation > 0.0F && (thresholds.base_target_length - length) > deviation)
                {
                    collapse = true;
                }
            }

            if (!collapse)
            {
                return false;
            }

            if (thresholds.max_normal_deviation_degrees.has_value())
            {
                const float allowed = thresholds.max_normal_deviation_degrees.value();
                if (allowed > 0.0F && dihedral_degrees > allowed * 0.75F)
                {
                    return false;
                }
            }

            return true;
        }

        std::uint32_t execute_adaptive_remesh(const RemeshRequest& request,
                                              MeshInterface& interface,
                                              std::vector<bool>& locked,
                                              const AdaptiveRemeshThresholds& thresholds,
                                              const std::unordered_set<std::uint64_t>* protected_edges,
                                              VertexProperty<math::vec3>* rest_positions,
                                              VertexProperty<math::vec2>* texture_coordinates,
                                              AttributeTransferMode texture_mode,
                                              RemeshOperationCounters* counters)
        {
            std::uint32_t performed_iterations = 0U;
            const float smoothing_factor = request.relaxation_factor * request.tangential_smoothing_weight;
            const bool tangential_smoothing = request.tangential_smoothing_weight > 0.0F;
            const bool resample_texture = texture_coordinates != nullptr &&
                                          texture_mode != AttributeTransferMode::kDrop;
            const bool update_rest_positions = rest_positions != nullptr;

            for (; performed_iterations < request.max_iterations; ++performed_iterations)
            {
                std::vector<EdgeHandle> edges_to_split;
                std::vector<EdgeHandle> edges_to_collapse;

                for (auto edge : interface.edges())
                {
                    if (interface.is_deleted(edge))
                    {
                        continue;
                    }

                    const float length = compute_edge_length(interface, edge);
                    const float dihedral = compute_edge_dihedral_degrees(interface, edge);

                    if (should_split_edge(thresholds, length, dihedral))
                    {
                        edges_to_split.push_back(edge);
                        continue;
                    }

                    if (should_collapse_edge(thresholds, length, dihedral))
                    {
                        edges_to_collapse.push_back(edge);
                    }
                }

                bool iteration_changed = false;

                for (EdgeHandle edge : edges_to_collapse)
                {
                    HalfedgeHandle collapse_halfedge{};
                    VertexHandle keep_vertex{};
                    VertexHandle remove_vertex{};

                    if (!can_collapse_edge(request,
                                           locked,
                                           interface,
                                           edge,
                                           collapse_halfedge,
                                           keep_vertex,
                                           remove_vertex,
                                           protected_edges))
                    {
                        continue;
                    }

                    const math::vec3 blended =
                        (interface.position(keep_vertex) + interface.position(remove_vertex)) * 0.5F;
                    interface.position(keep_vertex) = blended;

                    if (update_rest_positions)
                    {
                        update_collapse_rest_position(*rest_positions, keep_vertex, remove_vertex);
                    }

                    if (resample_texture)
                    {
                        update_collapse_uv(*texture_coordinates, keep_vertex, remove_vertex);
                    }

                    interface.collapse(collapse_halfedge);
                    if (counters != nullptr)
                    {
                        counters->collapse_count += 1U;
                    }
                    iteration_changed = true;
                }

                ensure_vertex_capacity(locked, interface);

                for (EdgeHandle edge : edges_to_split)
                {
                    if (interface.is_deleted(edge))
                    {
                        continue;
                    }

                    const float length = compute_edge_length(interface, edge);
                    const float dihedral = compute_edge_dihedral_degrees(interface, edge);
                    if (!should_split_edge(thresholds, length, dihedral))
                    {
                        continue;
                    }

                    const HalfedgeHandle h0 = interface.halfedge(edge, 0);
                    const HalfedgeHandle h1 = interface.halfedge(edge, 1);
                    const math::vec3& p0 = interface.position(interface.to_vertex(h0));
                    const math::vec3& p1 = interface.position(interface.to_vertex(h1));

                    const std::size_t previous_vertices = interface.vertices_size();
                    const auto new_halfedge = interface.split(edge, (p0 + p1) * 0.5F);
                    (void)new_halfedge;
                    ensure_vertex_capacity(locked, interface);

                    if (interface.vertices_size() == previous_vertices + 1)
                    {
                        VertexHandle new_vertex(static_cast<std::uint32_t>(interface.vertices_size() - 1));
                        bool lock_new = false;
                        if (request.feature_preservation.lock_boundary_edges && interface.is_boundary(new_vertex))
                        {
                            lock_new = true;
                        }
                        if (!lock_new && request.feature_preservation.lock_feature_edges)
                        {
                            const bool v0_locked = is_locked_vertex(locked, interface.to_vertex(h0));
                            const bool v1_locked = is_locked_vertex(locked, interface.to_vertex(h1));
                            lock_new = v0_locked && v1_locked;
                        }
                        locked[new_vertex.index()] = lock_new;

                        if (update_rest_positions)
                        {
                            const math::vec3& rest0 = (*rest_positions)[interface.to_vertex(h0)];
                            const math::vec3& rest1 = (*rest_positions)[interface.to_vertex(h1)];
                            assign_interpolated_rest_position(*rest_positions, new_vertex, rest0, rest1);
                        }

                        if (resample_texture)
                        {
                            const math::vec2& uv0 = (*texture_coordinates)[interface.to_vertex(h0)];
                            const math::vec2& uv1 = (*texture_coordinates)[interface.to_vertex(h1)];
                            assign_interpolated_uv(*texture_coordinates, new_vertex, uv0, uv1);
                        }

                        if (counters != nullptr)
                        {
                            counters->split_count += 1U;
                        }
                    }

                    iteration_changed = true;
                }

                if (!iteration_changed)
                {
                    break;
                }

                ensure_vertex_capacity(locked, interface);

                if (smoothing_factor > 0.0F)
                {
                    if (tangential_smoothing)
                    {
                        std::vector<math::vec3> normals = compute_vertex_normals(interface);
                        laplacian_relaxation(interface, locked, smoothing_factor, &normals, rest_positions);
                    }
                    else
                    {
                        laplacian_relaxation(interface, locked, smoothing_factor, nullptr, rest_positions);
                    }
                }
            }

            return performed_iterations;
        }
    } // namespace

    RemeshResult<RemeshOutput> Remesh(const RemeshRequest& request) noexcept
    {
        if (const RemeshValidationResult validation = ValidateRemeshRequest(request); !validation.has_value())
        {
            return RemeshResult<RemeshOutput>{validation.error()};
        }

        const auto start_time = std::chrono::steady_clock::now();

        auto resolved_targets_result = ResolveRemeshingTargets(request);
        if (!resolved_targets_result.has_value())
        {
            return RemeshResult<RemeshOutput>{resolved_targets_result.error()};
        }

        const ResolvedRemeshingTargets& resolved_targets = resolved_targets_result.value();
        if (request.mode != RemeshingMode::kAdaptive && !resolved_targets.target_edge_length.has_value())
        {
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::invalid_target_configuration,
                                                                "remeshing requires a target edge length")};
        }

        const SurfaceTopologySummary topology_summary = AnalyzeSurfaceTopology(
            *request.input_mesh, math::radians(request.feature_preservation.minimum_feature_angle_degrees));
        const std::unordered_set<std::uint64_t> protected_edges =
            build_protected_edge_set(topology_summary, request);

        Mesh mesh{};
        try
        {
            mesh::build_halfedge_from_surface_mesh(*request.input_mesh, mesh.interface);
        }
        catch (const std::exception& error)
        {
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::invalid_input_mesh, error.what())};
        }

        VertexProperty<math::vec3> rest_positions{};
        VertexProperty<math::vec3>* rest_positions_ptr = nullptr;
        VertexProperty<math::vec2> texture_coordinates{};
        VertexProperty<math::vec2>* texture_coordinates_ptr = nullptr;
        const AttributeTransferMode texture_transfer_mode = request.attribute_policy.texture_coordinates;
        if (mesh.interface.has_vertex_property(kRestPositionPropertyName))
        {
            rest_positions = mesh.interface.get_vertex_property<math::vec3>(kRestPositionPropertyName);
            rest_positions_ptr = &rest_positions;
        }

        if (mesh.interface.has_vertex_property("v:texcoord"))
        {
            texture_coordinates = mesh.interface.get_vertex_property<math::vec2>("v:texcoord");
            texture_coordinates_ptr = &texture_coordinates;
            if (texture_transfer_mode == AttributeTransferMode::kDrop)
            {
                mesh.interface.remove_vertex_property(texture_coordinates);
                texture_coordinates.reset();
                texture_coordinates_ptr = nullptr;
            }
        }

        if (!mesh.interface.is_triangle_mesh())
        {
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::invalid_input_mesh,
                                                                "uniform remeshing currently supports triangle meshes only")};
        }

        std::vector<bool> locked = initialise_locked_vertices(topology_summary, mesh.interface, request);

        const bool tangential_requested = request.tangential_smoothing_weight > 0.0F;
        const bool use_tangential_smoothing =
            (request.mode == RemeshingMode::kFeaturePreserving) ||
            (request.mode == RemeshingMode::kAdaptive && tangential_requested);
        const std::unordered_set<std::uint64_t>* protected_edge_ptr =
            (!protected_edges.empty() && use_tangential_smoothing) ? &protected_edges : nullptr;

        std::uint32_t iterations = 0U;
        RemeshOperationCounters counters{};
        std::optional<AdaptiveRemeshThresholds> adaptive_thresholds{};

        switch (request.mode)
        {
        case RemeshingMode::kUniform:
        case RemeshingMode::kFeaturePreserving:
        {
            const float target_length = resolved_targets.target_edge_length.value();
            iterations = execute_uniform_remesh(request,
                                                mesh.interface,
                                                locked,
                                                target_length,
                                                kDefaultSplitThreshold,
                                                kDefaultCollapseThreshold,
                                                protected_edge_ptr,
                                                use_tangential_smoothing,
                                                rest_positions_ptr,
                                                texture_coordinates_ptr,
                                                texture_transfer_mode,
                                                &counters);
            break;
        }
        case RemeshingMode::kAdaptive:
        {
            adaptive_thresholds = make_adaptive_thresholds(request, resolved_targets);
            iterations = execute_adaptive_remesh(request,
                                                 mesh.interface,
                                                 locked,
                                                 adaptive_thresholds.value(),
                                                 protected_edge_ptr,
                                                 rest_positions_ptr,
                                                 texture_coordinates_ptr,
                                                 texture_transfer_mode,
                                                 &counters);
            break;
        }
        }

        if (rest_positions_ptr != nullptr)
        {
            for (auto vertex : mesh.interface.vertices())
            {
                if (mesh.interface.is_deleted(vertex))
                {
                    continue;
                }

                (*rest_positions_ptr)[vertex] = mesh.interface.position(vertex);
            }
        }

        mesh.interface.garbage_collection();

        RemeshOutput output{};
        try
        {
            output.mesh = mesh::build_surface_mesh_from_halfedge(mesh.interface);
        }
        catch (const std::exception& error)
        {
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::invalid_input_mesh, error.what())};
        }

        output.statistics.iteration_count = iterations;
        output.statistics.split_count = counters.split_count;
        output.statistics.collapse_count = counters.collapse_count;
        const MeshEdgeStatistics stats = ComputeMeshEdgeStatistics(output.mesh);
        output.statistics.max_edge_length = stats.max_edge_length;
        output.statistics.min_edge_length = stats.min_edge_length;

        if (stats.edge_count > 0U)
        {
            float target = resolved_targets.target_edge_length.value_or(stats.mean_edge_length());
            if (request.mode == RemeshingMode::kAdaptive && adaptive_thresholds.has_value())
            {
                target = adaptive_thresholds->base_target_length;
            }

            const float max_over_target = math::utils::abs(stats.max_edge_length - target);
            const float max_under_target = math::utils::abs(stats.min_edge_length - target);
            output.statistics.max_error = std::max(max_over_target, max_under_target);
        }
        else
        {
            output.statistics.max_error = 0.0F;
        }

        std::uint64_t surface_deviation_sample_count = 0U;

        if (request.input_mesh != nullptr)
        {
            const SurfaceDeviationMetrics deviation_metrics =
                ComputeSurfaceDeviationMetrics(*request.input_mesh, output.mesh);
            output.statistics.max_surface_deviation = deviation_metrics.max_distance;
            output.statistics.mean_surface_deviation = deviation_metrics.mean_distance;
            output.statistics.rms_surface_deviation = deviation_metrics.rms_distance;
            surface_deviation_sample_count = static_cast<std::uint64_t>(deviation_metrics.sample_count);
        }
        else
        {
            output.statistics.max_surface_deviation = 0.0F;
            output.statistics.mean_surface_deviation = 0.0F;
            output.statistics.rms_surface_deviation = 0.0F;
        }
        switch (request.parameterization.mode)
        {
        case ParameterizationMode::kNone:
            output.parameterization = ParameterizationSummary{};
            break;
        case ParameterizationMode::kReuseExisting:
        {
            if (output.mesh.texture_coordinates.empty())
            {
                output.parameterization = ParameterizationSummary{};
                break;
            }

            output.parameterization = finalize_parameterization(output.mesh, request.parameterization);
            break;
        }
        case ParameterizationMode::kGenerateLscm:
        {
            auto parameterization_result = generate_lscm_parameterization(output.mesh, request.parameterization);
            if (!parameterization_result.has_value())
            {
                return RemeshResult<RemeshOutput>{parameterization_result.error()};
            }
            output.parameterization = parameterization_result.value();
            break;
        }
        case ParameterizationMode::kGenerateAbfpp:
        {
            auto parameterization_result = generate_abfpp_parameterization(output.mesh, request.parameterization);
            if (!parameterization_result.has_value())
            {
                return RemeshResult<RemeshOutput>{parameterization_result.error()};
            }
            output.parameterization = parameterization_result.value();
            break;
        }
        }

        const double duration_ms = std::chrono::duration<double, std::milli>(
                                        std::chrono::steady_clock::now() - start_time)
                                        .count();
        output.statistics.duration_ms = duration_ms;

        RemeshTelemetry::instance().record_invocation(request.mode,
                                                      iterations,
                                                      counters.split_count,
                                                      counters.collapse_count,
                                                      static_cast<std::uint64_t>(output.mesh.positions.size()),
                                                      duration_ms,
                                                      output.statistics,
                                                      surface_deviation_sample_count,
                                                      request.job_label);

        return RemeshResult<RemeshOutput>{output};
    }
} // namespace engine::geometry

