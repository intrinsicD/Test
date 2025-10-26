#include "engine/geometry/remesh/remesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
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

        [[nodiscard]] ParameterizationSummary compute_parameterization_summary(const SurfaceMesh& mesh) noexcept
        {
            ParameterizationSummary summary{};

            if (mesh.texture_coordinates.empty() || mesh.indices.size() < 3U)
            {
                return summary;
            }

            float total_area = 0.0F;
            float weighted_density = 0.0F;
            float max_density = 0.0F;

            for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
            {
                const std::uint32_t i0 = mesh.indices[i];
                const std::uint32_t i1 = mesh.indices[i + 1U];
                const std::uint32_t i2 = mesh.indices[i + 2U];

                if (i0 >= mesh.positions.size() || i1 >= mesh.positions.size() || i2 >= mesh.positions.size())
                {
                    continue;
                }

                const float world_area = triangle_area(mesh.positions[i0], mesh.positions[i1], mesh.positions[i2]);
                if (world_area <= kEpsilon)
                {
                    continue;
                }

                if (i0 >= mesh.texture_coordinates.size() || i1 >= mesh.texture_coordinates.size() ||
                    i2 >= mesh.texture_coordinates.size())
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
            }

            if (total_area > kEpsilon)
            {
                summary.average_stretch = weighted_density / total_area;
                summary.texel_density = summary.average_stretch;
            }

            summary.max_stretch = max_density;
            summary.chart_count = mesh.texture_coordinates.empty() ? 0U : 1U;
            return summary;
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
                                  const std::vector<math::vec3>* vertex_normals)
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

                updated[vertex.index()] = current + factor * displacement;
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
                                             VertexProperty<math::vec2>* texture_coordinates,
                                             AttributeTransferMode texture_mode)
        {
            std::uint32_t performed_iterations = 0U;
            const float smoothing_factor = request.relaxation_factor * request.tangential_smoothing_weight;
            const bool resample_texture = texture_coordinates != nullptr &&
                                          texture_mode != AttributeTransferMode::kDrop;

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

                    if (resample_texture)
                    {
                        update_collapse_uv(*texture_coordinates, keep_vertex, remove_vertex);
                    }

                    interface.position(keep_vertex) = (interface.position(keep_vertex) + interface.position(remove_vertex)) * 0.5F;
                    interface.collapse(collapse_halfedge);
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

                        if (resample_texture)
                        {
                            const math::vec2& uv0 = (*texture_coordinates)[interface.to_vertex(h0)];
                            const math::vec2& uv1 = (*texture_coordinates)[interface.to_vertex(h1)];
                            assign_interpolated_uv(*texture_coordinates, new_vertex, uv0, uv1);
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

                laplacian_relaxation(interface, locked, smoothing_factor, normals_ptr);
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
                                              VertexProperty<math::vec2>* texture_coordinates,
                                              AttributeTransferMode texture_mode)
        {
            std::uint32_t performed_iterations = 0U;
            const float smoothing_factor = request.relaxation_factor * request.tangential_smoothing_weight;
            const bool tangential_smoothing = request.tangential_smoothing_weight > 0.0F;
            const bool resample_texture = texture_coordinates != nullptr &&
                                          texture_mode != AttributeTransferMode::kDrop;

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

                    if (resample_texture)
                    {
                        update_collapse_uv(*texture_coordinates, keep_vertex, remove_vertex);
                    }

                    interface.collapse(collapse_halfedge);
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

                        if (resample_texture)
                        {
                            const math::vec2& uv0 = (*texture_coordinates)[interface.to_vertex(h0)];
                            const math::vec2& uv1 = (*texture_coordinates)[interface.to_vertex(h1)];
                            assign_interpolated_uv(*texture_coordinates, new_vertex, uv0, uv1);
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
                        laplacian_relaxation(interface, locked, smoothing_factor, &normals);
                    }
                    else
                    {
                        laplacian_relaxation(interface, locked, smoothing_factor, nullptr);
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

        VertexProperty<math::vec2> texture_coordinates{};
        const AttributeTransferMode texture_transfer_mode = request.attribute_policy.texture_coordinates;
        if (mesh.interface.has_vertex_property("v:texcoord"))
        {
            texture_coordinates = mesh.interface.get_vertex_property<math::vec2>("v:texcoord");
            if (texture_transfer_mode == AttributeTransferMode::kDrop)
            {
                mesh.interface.remove_vertex_property(texture_coordinates);
                texture_coordinates.reset();
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
                                                texture_coordinates ? &texture_coordinates : nullptr,
                                                texture_transfer_mode);
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
                                                 texture_coordinates ? &texture_coordinates : nullptr,
                                                 texture_transfer_mode);
            break;
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
        switch (request.parameterization.mode)
        {
        case ParameterizationMode::kNone:
            output.parameterization = ParameterizationSummary{};
            break;
        case ParameterizationMode::kReuseExisting:
        {
            if (!output.mesh.texture_coordinates.empty())
            {
                ParameterizationSummary summary = compute_parameterization_summary(output.mesh);
                if (request.parameterization.target_texel_density > kEpsilon &&
                    summary.texel_density > kEpsilon)
                {
                    const float scale =
                        request.parameterization.target_texel_density / summary.texel_density;
                    scale_texture_coordinates(output.mesh, scale);
                    summary.average_stretch *= scale;
                    summary.max_stretch *= scale;
                    summary.texel_density *= scale;
                }
                summary.chart_count = output.mesh.texture_coordinates.empty() ? 0U : 1U;
                output.parameterization = summary;
            }
            else
            {
                output.parameterization = ParameterizationSummary{};
            }
            break;
        }
        case ParameterizationMode::kGenerateLscm:
        case ParameterizationMode::kGenerateAbfpp:
            output.parameterization = ParameterizationSummary{};
            break;
        }

        return RemeshResult<RemeshOutput>{output};
    }
} // namespace engine::geometry

