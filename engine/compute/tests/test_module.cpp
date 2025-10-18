#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "engine/compute/api.hpp"

TEST(ComputeModule, ModuleNameMatchesNamespace)
{
    EXPECT_EQ(engine::compute::module_name(), "compute");
    EXPECT_STREQ(engine_compute_module_name(), "compute");
}

TEST(ComputeModule, IdentityTransformIsMatrixIdentity)
{
    const auto transform = engine::compute::identity_transform();

    for (std::size_t row = 0; row < 4; ++row)
    {
        for (std::size_t column = 0; column < 4; ++column)
        {
            const float expected = (row == column) ? 1.0F : 0.0F;
            EXPECT_FLOAT_EQ(transform[row][column], expected);
        }
    }
}

namespace
{

using DispatcherPtr = std::unique_ptr<engine::compute::Dispatcher>;

void ExpectSubstring(std::string_view haystack, std::string_view needle)
{
    if (haystack.find(needle) == std::string_view::npos)
    {
        FAIL() << "Expected substring '" << needle << "' within '" << haystack << "'";
    }
}

void ExpectDispatcherRespectsDependencies(DispatcherPtr dispatcher)
{
    std::array<int, 3> values{0, 0, 0};
    const auto first = dispatcher->add_kernel("first", [&]() { values[0] = 1; });
    const auto second = dispatcher->add_kernel(
        "second",
        [&]()
        {
            values[1] = values[0] + 1;
        },
        {first});
    MAYBE_UNUSED_CONST_AUTO third = dispatcher->add_kernel(
        "third",
        [&]()
        {
            values[2] = values[1] + 1;
        },
        {second});

    const auto report = dispatcher->dispatch();
    ASSERT_EQ(report.execution_order.size(), 3U);
    ASSERT_EQ(report.kernel_durations.size(), report.execution_order.size());
    EXPECT_EQ(report.execution_order.front(), "first");
    EXPECT_EQ(report.execution_order.back(), "third");
    for (const auto duration : report.kernel_durations)
    {
        EXPECT_GE(duration, 0.0);
    }
    EXPECT_EQ(values[2], 3);

    ASSERT_EQ(report.dependency_graph.nodes.size(), 3U);
    EXPECT_TRUE(report.dependency_graph.nodes[0].dependencies.empty());
    ASSERT_EQ(report.dependency_graph.nodes[2].dependencies.size(), 1U);
    EXPECT_EQ(report.dependency_graph.nodes[2].dependencies.front(), second);
}

template <typename ExceptionType>
void ExpectDispatcherThrows(
    DispatcherPtr dispatcher,
    const std::vector<engine::compute::Dispatcher::kernel_id>& dependencies_for_first,
    const std::vector<engine::compute::Dispatcher::kernel_id>& dependencies_for_second,
    std::string_view expected_prefix)
{
    MAYBE_UNUSED_CONST_AUTO a = dispatcher->add_kernel("a", []
    {
    }, dependencies_for_first);
    MAYBE_UNUSED_CONST_AUTO b = dispatcher->add_kernel("b", []
    {
    }, dependencies_for_second);

    try
    {
        (void)dispatcher->dispatch();
        FAIL() << "Dispatcher did not raise the expected exception";
    }
    catch (const ExceptionType& error)
    {
        const std::string_view message{error.what()};
        ExpectSubstring(message, expected_prefix);
        ExpectSubstring(message, "digraph");
    }
}

}  // namespace

TEST(ComputeModule, CpuDispatcherRespectsDependencies)
{
    ExpectDispatcherRespectsDependencies(engine::compute::make_cpu_dispatcher());
}

TEST(ComputeModule, CudaDispatcherRespectsDependencies)
{
    ExpectDispatcherRespectsDependencies(engine::compute::make_cuda_dispatcher());
}

TEST(ComputeModule, CpuDispatcherDetectsCyclesDuringRegistration)
{
    auto dispatcher = engine::compute::make_cpu_dispatcher();
    MAYBE_UNUSED_CONST_AUTO first = dispatcher->add_kernel("a", []
    {
    }, {1U});

    try
    {
        MAYBE_UNUSED_CONST_AUTO second = dispatcher->add_kernel("b", []
        {
        }, {0U});
        FAIL() << "Dispatcher accepted a cyclic registration";
    }
    catch (const std::runtime_error& error)
    {
        const std::string_view message{error.what()};
        ExpectSubstring(message, "KernelDispatcher detected a cycle during registration");
        ExpectSubstring(message, "digraph");
    }

    EXPECT_EQ(dispatcher->size(), 1U);
}

TEST(ComputeModule, CudaDispatcherDetectsCyclesDuringRegistration)
{
    auto dispatcher = engine::compute::make_cuda_dispatcher();
    MAYBE_UNUSED_CONST_AUTO first = dispatcher->add_kernel("a", []
    {
    }, {1U});

    try
    {
        MAYBE_UNUSED_CONST_AUTO second = dispatcher->add_kernel("b", []
        {
        }, {0U});
        FAIL() << "Dispatcher accepted a cyclic registration";
    }
    catch (const std::runtime_error& error)
    {
        const std::string_view message{error.what()};
        ExpectSubstring(message, "KernelDispatcher detected a cycle during registration");
        ExpectSubstring(message, "digraph");
    }

    EXPECT_EQ(dispatcher->size(), 1U);
}

TEST(ComputeModule, CpuDispatcherThrowsOnInvalidDependencyIndex)
{
    ExpectDispatcherThrows<std::out_of_range>(
        engine::compute::make_cpu_dispatcher(),
        {},
        {0U, 2U},
        "KernelDispatcher dependency index out of range");
}

TEST(ComputeModule, CudaDispatcherThrowsOnInvalidDependencyIndex)
{
    ExpectDispatcherThrows<std::out_of_range>(
        engine::compute::make_cuda_dispatcher(),
        {},
        {0U, 2U},
        "KernelDispatcher dependency index out of range");
}

TEST(ComputeModule, DispatcherDependencyGraphExposesUnresolvedEdges)
{
    auto dispatcher = engine::compute::make_cpu_dispatcher();
    MAYBE_UNUSED_CONST_AUTO first = dispatcher->add_kernel("first", []
    {
    }, {3U});

    const auto graph = dispatcher->dependency_graph();
    ASSERT_EQ(graph.nodes.size(), 1U);
    ASSERT_TRUE(graph.nodes.front().dependencies.empty());
    ASSERT_EQ(graph.nodes.front().unresolved_dependencies.size(), 1U);
    EXPECT_EQ(graph.nodes.front().unresolved_dependencies.front(), 3U);

    const auto dot = graph.to_dot();
    ExpectSubstring(dot, "pending:3");
    ExpectSubstring(dot, "node0");
}

TEST(ComputeModule, DispatcherDependencyGraphTracksResolvedEdges)
{
    auto dispatcher = engine::compute::make_cpu_dispatcher();
    const auto first = dispatcher->add_kernel("first", []
    {
    });
    MAYBE_UNUSED_CONST_AUTO second = dispatcher->add_kernel("second", []
    {
    }, {first});

    const auto graph = dispatcher->dependency_graph();
    ASSERT_EQ(graph.nodes.size(), 2U);
    ASSERT_EQ(graph.nodes[1].dependencies.size(), 1U);
    EXPECT_EQ(graph.nodes[1].dependencies.front(), first);
    EXPECT_TRUE(graph.nodes[1].unresolved_dependencies.empty());

    const auto dot = graph.to_dot();
    ExpectSubstring(dot, "node0 -> node1");
}

TEST(ComputeModule, ReportsDispatcherAvailability)
{
    EXPECT_TRUE(engine::compute::is_cpu_dispatcher_available());

#if ENGINE_ENABLE_COMPUTE_CUDA
    EXPECT_TRUE(engine::compute::is_cuda_dispatcher_available());
#else
    EXPECT_FALSE(engine::compute::is_cuda_dispatcher_available());
#endif

    const auto capabilities = engine::compute::dispatcher_capabilities();
    EXPECT_EQ(capabilities.cpu_available, engine::compute::is_cpu_dispatcher_available());
    EXPECT_EQ(capabilities.cuda_available, engine::compute::is_cuda_dispatcher_available());
}
