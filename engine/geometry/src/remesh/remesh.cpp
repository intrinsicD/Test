#include "engine/geometry/remesh/remesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
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
                                             bool tangential_smoothing)
        {
            std::uint32_t performed_iterations = 0U;
            const float smoothing_factor = request.relaxation_factor * request.tangential_smoothing_weight;

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
    } // namespace

    RemeshResult<RemeshOutput> Remesh(const RemeshRequest& request) noexcept
    {
        if (const RemeshValidationResult validation = ValidateRemeshRequest(request); !validation.has_value())
        {
            return RemeshResult<RemeshOutput>{validation.error()};
        }

        switch (request.mode)
        {
        case RemeshingMode::kUniform:
        case RemeshingMode::kFeaturePreserving:
            break;
        case RemeshingMode::kAdaptive:
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::unsupported_mode,
                                                                "remeshing mode not yet implemented")};
        }

        auto resolved_targets_result = ResolveRemeshingTargets(request);
        if (!resolved_targets_result.has_value())
        {
            return RemeshResult<RemeshOutput>{resolved_targets_result.error()};
        }

        const ResolvedRemeshingTargets& resolved_targets = resolved_targets_result.value();
        if (!resolved_targets.target_edge_length.has_value())
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

        if (!mesh.interface.is_triangle_mesh())
        {
            return RemeshResult<RemeshOutput>{make_remesh_error(RemeshError::invalid_input_mesh,
                                                                "uniform remeshing currently supports triangle meshes only")};
        }

        std::vector<bool> locked = initialise_locked_vertices(topology_summary, mesh.interface, request);

        const bool use_tangential_smoothing = request.mode == RemeshingMode::kFeaturePreserving;
        const std::unordered_set<std::uint64_t>* protected_edge_ptr =
            (!protected_edges.empty() && use_tangential_smoothing) ? &protected_edges : nullptr;

        const float target_length = resolved_targets.target_edge_length.value();
        const std::uint32_t iterations = execute_uniform_remesh(request,
                                                                mesh.interface,
                                                                locked,
                                                                target_length,
                                                                kDefaultSplitThreshold,
                                                                kDefaultCollapseThreshold,
                                                                protected_edge_ptr,
                                                                use_tangential_smoothing);

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
            const float target = resolved_targets.target_edge_length.value();
            const float max_over_target = math::utils::abs(stats.max_edge_length - target);
            const float max_under_target = math::utils::abs(stats.min_edge_length - target);
            output.statistics.max_error = std::max(max_over_target, max_under_target);
        }
        else
        {
            output.statistics.max_error = 0.0F;
        }
        output.parameterization = ParameterizationSummary{};

        return RemeshResult<RemeshOutput>{output};
    }
} // namespace engine::geometry

