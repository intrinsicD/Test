#include "engine/tools/profiling/profiler.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace engine::tools::profiling;

TEST(Profiler, BasicProfiling)
{
    Profiler profiler;

    profiler.begin("test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    profiler.end("test_operation");

    auto report = profiler.generate_report();

    ASSERT_EQ(report.entries.size(), 1);
    EXPECT_EQ(report.entries[0].name, "test_operation");
    EXPECT_EQ(report.entries[0].call_count, 1);
    EXPECT_GT(report.entries[0].duration_ms, 9.0); // At least 10ms
}


TEST(Profiler, MultipleCalls)
{
    Profiler profiler;

    for (int i = 0; i < 5; ++i)
    {
        profiler.begin("repeated_op");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        profiler.end("repeated_op");
    }

    auto report = profiler.generate_report();

    ASSERT_EQ(report.entries.size(), 1);
    EXPECT_EQ(report.entries[0].call_count, 5);
    EXPECT_GT(report.entries[0].average_ms, 0.0);
}

TEST(Profiler, ScopedProfiling)
{
    Profiler profiler;

    {
        ScopedProfile scope(profiler, "scoped_test");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    auto report = profiler.generate_report();

    ASSERT_EQ(report.entries.size(), 1);
    EXPECT_EQ(report.entries[0].name, "scoped_test");
    EXPECT_EQ(report.entries[0].call_count, 1);
}

TEST(Profiler, Reset)
{
    Profiler profiler;

    profiler.begin("test");
    profiler.end("test");

    auto report1 = profiler.generate_report();
    EXPECT_EQ(report1.entries.size(), 1);

    profiler.reset();

    auto report2 = profiler.generate_report();
    EXPECT_EQ(report2.entries.size(), 0);
}

TEST(Profiler, GlobalProfilerAccess)
{
    auto& profiler = global_profiler();

    // Should return the same instance
    auto& profiler2 = global_profiler();

    EXPECT_EQ(&profiler, &profiler2);
}
