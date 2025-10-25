#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>

#include "engine/geometry/remesh/remesh.hpp"
#include "engine/geometry/topology/surface_topology.hpp"
#include "engine/math/math.hpp"

namespace engine::geometry
{
    namespace
    {
        SurfaceMesh make_wedge_mesh()
        {
            SurfaceMesh mesh{};
            mesh.positions = {
                {0.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F},
                {0.0F, 1.0F, 0.0F},
                {0.0F, 0.0F, 1.0F},
            };
            mesh.rest_positions = mesh.positions;
            mesh.indices = {
                0U, 1U, 2U,
                0U, 3U, 1U,
            };
            return mesh;
        }
    } // namespace

    TEST(RemeshFeaturePreserving, PreservesCreaseVertices)
    {
        SurfaceMesh mesh = make_wedge_mesh();

        RemeshRequest request{};
        request.input_mesh = &mesh;
        request.mode = RemeshingMode::kFeaturePreserving;
        request.targets.target_edge_length = 0.25F;
        request.max_iterations = 6U;
        request.relaxation_factor = 0.5F;
        request.tangential_smoothing_weight = 0.5F;
        request.feature_preservation.lock_boundary_edges = true;
        request.feature_preservation.lock_feature_edges = true;
        request.feature_preservation.minimum_feature_angle_degrees = 30.0F;

        const RemeshResult<RemeshOutput> result = Remesh(request);
        ASSERT_TRUE(result.has_value());

        const RemeshOutput& output = result.value();
        EXPECT_GE(output.statistics.iteration_count, 1U);

        const SurfaceTopologySummary summary = AnalyzeSurfaceTopology(
            output.mesh, math::radians(request.feature_preservation.minimum_feature_angle_degrees));

        std::vector<bool> crease_vertices(output.mesh.positions.size(), false);
        for (const SurfaceEdgeTag& edge : summary.edges)
        {
            if (!edge.is_crease)
            {
                continue;
            }

            ASSERT_LT(edge.v0, output.mesh.positions.size());
            ASSERT_LT(edge.v1, output.mesh.positions.size());
            crease_vertices[edge.v0] = true;
            crease_vertices[edge.v1] = true;
        }

        std::size_t crease_vertex_count = 0U;
        float max_crease_distance = 0.0F;
        float min_non_crease_distance = std::numeric_limits<float>::infinity();

        for (std::size_t index = 0; index < output.mesh.positions.size(); ++index)
        {
            const math::vec3& position = output.mesh.positions[index];
            const float distance_to_axis = std::sqrt(position[1] * position[1] + position[2] * position[2]);

            if (crease_vertices[index])
            {
                ++crease_vertex_count;
                max_crease_distance = std::max(max_crease_distance, distance_to_axis);
            }
            else
            {
                min_non_crease_distance = std::min(min_non_crease_distance, distance_to_axis);
            }
        }

        EXPECT_GE(crease_vertex_count, 2U);
        EXPECT_LT(max_crease_distance, 0.1F);
        EXPECT_LT(max_crease_distance, min_non_crease_distance);
    }
} // namespace engine::geometry
