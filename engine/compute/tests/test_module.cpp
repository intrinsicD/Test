#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>

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
}

template <typename ExceptionType>
void ExpectDispatcherThrows(
    DispatcherPtr dispatcher,
    const char* expected_message,
    const std::vector<engine::compute::Dispatcher::kernel_id>& dependencies_for_first,
    const std::vector<engine::compute::Dispatcher::kernel_id>& dependencies_for_second)
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
        EXPECT_STREQ(error.what(), expected_message);
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

TEST(ComputeModule, CpuDispatcherDetectsCycles)
{
    ExpectDispatcherThrows<std::runtime_error>(
        engine::compute::make_cpu_dispatcher(),
        "KernelDispatcher detected a cycle",
        {1U},
        {0U});
}

TEST(ComputeModule, CudaDispatcherDetectsCycles)
{
    ExpectDispatcherThrows<std::runtime_error>(
        engine::compute::make_cuda_dispatcher(),
        "KernelDispatcher detected a cycle",
        {1U},
        {0U});
}

TEST(ComputeModule, CpuDispatcherThrowsOnInvalidDependencyIndex)
{
    ExpectDispatcherThrows<std::out_of_range>(
        engine::compute::make_cpu_dispatcher(),
        "KernelDispatcher dependency index out of range",
        {},
        {0U, 2U});
}

TEST(ComputeModule, CudaDispatcherThrowsOnInvalidDependencyIndex)
{
    ExpectDispatcherThrows<std::out_of_range>(
        engine::compute::make_cuda_dispatcher(),
        "KernelDispatcher dependency index out of range",
        {},
        {0U, 2U});
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
