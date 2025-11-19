#include <gtest/gtest.h>

#include "engine/rendering/gpu_profiler.hpp"

namespace
{
    using engine::rendering::CommandBufferHandle;
    using engine::rendering::GpuProfiler;
    using engine::rendering::QueueType;
}

TEST(GpuProfiler, RecordsPassDurations)
{
    GpuProfiler profiler{};
    profiler.begin_frame(engine::rendering::resources::GraphicsApi::OpenGL);

    CommandBufferHandle handle{.index = 1};
    profiler.begin_pass("ForwardGeometry", QueueType::Graphics, handle);
    profiler.end_pass(handle);

    const auto timings = profiler.consume_pass_timings();
    ASSERT_EQ(timings.size(), 1U);
    EXPECT_EQ(timings[0].pass_name, "ForwardGeometry");
    EXPECT_EQ(timings[0].queue, QueueType::Graphics);
    EXPECT_EQ(timings[0].command_buffer.index, 1U);
    EXPECT_GE(timings[0].timestamp_end_ns, timings[0].timestamp_begin_ns);
}

TEST(GpuProfiler, IgnoresIncompletePasses)
{
    GpuProfiler profiler{};
    profiler.begin_frame(engine::rendering::resources::GraphicsApi::OpenGL);

    CommandBufferHandle handle{.index = 2};
    profiler.begin_pass("Unfinished", QueueType::Graphics, handle);
    profiler.end_frame();

    const auto timings = profiler.consume_pass_timings();
    EXPECT_TRUE(timings.empty());
}
