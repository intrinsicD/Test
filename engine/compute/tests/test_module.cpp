#include <gtest/gtest.h>

#include <array>
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

TEST(ComputeModule, DispatcherRespectsDependencies)
{
    engine::compute::KernelDispatcher dispatcher;

    std::array<int, 3> values{0, 0, 0};
    const auto first = dispatcher.add_kernel("first", [&]() { values[0] = 1; });
    const auto second = dispatcher.add_kernel(
        "second",
        [&]()
        {
            values[1] = values[0] + 1;
        },
        {first});
    MAYBE_UNUSED_CONST_AUTO third = dispatcher.add_kernel(
        "third",
        [&]()
        {
            values[2] = values[1] + 1;
        },
        {second});

    const auto report = dispatcher.dispatch();
    EXPECT_EQ(report.execution_order.size(), 3U);
    EXPECT_EQ(report.execution_order.front(), "first");
    EXPECT_EQ(report.execution_order.back(), "third");
    EXPECT_EQ(values[2], 3);
}

TEST(ComputeModule, DispatcherDetectsCycles)
{
    engine::compute::KernelDispatcher dispatcher;
    const auto a = dispatcher.add_kernel("a", []
    {
    }, {1U});
    MAYBE_UNUSED_CONST_AUTO b = dispatcher.add_kernel("b", []
    {
    }, {a});

    try
    {
        (void)dispatcher.dispatch();
        FAIL() << "Expected KernelDispatcher to detect a cycle";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_STREQ(error.what(), "KernelDispatcher detected a cycle");
    }
}

TEST(ComputeModule, DispatcherThrowsOnInvalidDependencyIndex)
{
    engine::compute::KernelDispatcher dispatcher;
    MAYBE_UNUSED_CONST_AUTO a = dispatcher.add_kernel("a", []
    {
    });
    MAYBE_UNUSED_CONST_AUTO b = dispatcher.add_kernel("b", []
    {
    }, {0U, 2U});

    try
    {
        (void)dispatcher.dispatch();
        FAIL() << "Expected KernelDispatcher to detect invalid dependency index";
    }
    catch (const std::out_of_range& error)
    {
        EXPECT_STREQ(error.what(), "KernelDispatcher dependency index out of range");
    }
}
