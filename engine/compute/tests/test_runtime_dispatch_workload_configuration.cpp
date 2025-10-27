#include <algorithm>

#include <gtest/gtest.h>

#include "runtime_dispatch_demo/workload_configuration.hpp"

#include "engine/runtime/api.hpp"

namespace samples = engine::compute::samples;

namespace
{
    struct WorkloadExpectation
    {
        samples::WorkloadProfile profile;
        std::size_t mesh_subdivisions;
        std::size_t physics_bodies;
    };

    constexpr std::size_t kGroundBodyCount = 1U;

    [[nodiscard]] std::size_t expected_vertex_count(std::size_t subdivisions)
    {
        const std::size_t dimension = subdivisions + 1U;
        return dimension * dimension;
    }

    [[nodiscard]] std::size_t expected_body_count(std::size_t physics_bodies)
    {
        const std::size_t resolved = std::max<std::size_t>(1U, physics_bodies);
        return resolved + kGroundBodyCount;
    }
}

class RuntimeDispatchWorkloadConfigurationTest
    : public ::testing::TestWithParam<WorkloadExpectation>
{
};

TEST_P(RuntimeDispatchWorkloadConfigurationTest, DefinitionMatchesProfile)
{
    const auto expectation = GetParam();
    const auto definition = samples::workload_definition(expectation.profile);

    EXPECT_EQ(definition.profile, expectation.profile);
    EXPECT_EQ(definition.mesh_subdivisions, expectation.mesh_subdivisions);
    EXPECT_EQ(definition.physics_bodies, expectation.physics_bodies);
}

TEST_P(RuntimeDispatchWorkloadConfigurationTest, HostConfigurationProducesExpectedAssets)
{
    const auto expectation = GetParam();

    engine::runtime::RuntimeHost host{};
    samples::configure_runtime_host(host, expectation.profile);

    struct HostGuard
    {
        engine::runtime::RuntimeHost& host;
        bool initialized{false};

        ~HostGuard()
        {
            if (initialized)
            {
                host.shutdown();
            }
        }
    } guard{host, false};

    ASSERT_NO_THROW(host.initialize());
    guard.initialized = true;

    const auto& mesh = host.current_mesh();
    const auto vertices = expected_vertex_count(expectation.mesh_subdivisions);
    EXPECT_EQ(mesh.positions.size(), vertices);
    EXPECT_EQ(mesh.rest_positions.size(), vertices);
    EXPECT_EQ(mesh.normals.size(), vertices);

    const auto& positions = host.body_positions();
    EXPECT_EQ(positions.size(), expected_body_count(expectation.physics_bodies));
}

INSTANTIATE_TEST_SUITE_P(
    RuntimeDispatchWorkloads,
    RuntimeDispatchWorkloadConfigurationTest,
    ::testing::Values(
        WorkloadExpectation{samples::WorkloadProfile::Light, 16U, 16U},
        WorkloadExpectation{samples::WorkloadProfile::Balanced, 32U, 48U},
        WorkloadExpectation{samples::WorkloadProfile::Heavy, 64U, 128U}));