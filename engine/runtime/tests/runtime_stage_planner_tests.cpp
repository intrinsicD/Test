#include <gtest/gtest.h>

#include <utility>

#include "engine/runtime/errors.hpp"
#include "engine/runtime/loop.hpp"
#include "engine/runtime/runtime_loop_plan.hpp"

namespace engine::runtime
{
    TEST(RuntimeStagePlanner, ReportsErrorWhenNotConfigured)
    {
        RuntimeStagePlanner planner{};
        const auto next = planner.next_stage();
        ASSERT_FALSE(next.has_value());
        EXPECT_EQ(next.error().code(), RuntimeError::loop_stage_planner_unconfigured);
    }

    TEST(RuntimeStagePlanner, IteratesStagesInOrder)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("simulation.start", RuntimeLoopPhase::Simulation, [](double) {}).has_value());
        ASSERT_TRUE(builder.add_stage("presentation.dispatch", RuntimeLoopPhase::Presentation, [](double) {},
                                      {"simulation.start"}, true,
                                      RuntimeLoopThreadAffinity::WorkerThread)
                        .has_value());

        auto plan_result = builder.build();
        ASSERT_TRUE(plan_result.has_value());
        auto plan = std::move(plan_result).value();

        RuntimeStagePlanner planner{};
        ASSERT_TRUE(planner.configure_plan(plan).has_value());
        EXPECT_TRUE(planner.has_plan());

        auto first = planner.next_stage();
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(first.value().has_value());
        const auto first_execution = first.value().value();
        EXPECT_EQ(first_execution.handle.name, "simulation.start");
        EXPECT_EQ(first_execution.handle.phase, RuntimeLoopPhase::Simulation);
        EXPECT_EQ(first_execution.handle.thread_affinity, RuntimeLoopThreadAffinity::MainThread);
        EXPECT_TRUE(first_execution.handle.record_in_execution_report);
        EXPECT_EQ(first_execution.handle.index, 0U);

        auto second = planner.next_stage();
        ASSERT_TRUE(second.has_value());
        ASSERT_TRUE(second.value().has_value());
        const auto second_execution = second.value().value();
        EXPECT_EQ(second_execution.handle.name, "presentation.dispatch");
        EXPECT_EQ(second_execution.handle.phase, RuntimeLoopPhase::Presentation);
        EXPECT_EQ(second_execution.handle.thread_affinity, RuntimeLoopThreadAffinity::WorkerThread);
        EXPECT_TRUE(second_execution.handle.record_in_execution_report);
        EXPECT_EQ(second_execution.handle.index, 1U);

        auto done = planner.next_stage();
        ASSERT_TRUE(done.has_value());
        EXPECT_FALSE(done.value().has_value());
    }

    TEST(RuntimeStagePlanner, ResetIterationRestartsSequence)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("diagnostics.refresh", RuntimeLoopPhase::Diagnostics, [](double) {}).has_value());

        auto plan_result = builder.build();
        ASSERT_TRUE(plan_result.has_value());
        auto plan = std::move(plan_result).value();

        RuntimeStagePlanner planner{};
        ASSERT_TRUE(planner.configure_plan(plan).has_value());

        auto first = planner.next_stage();
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(first.value().has_value());
        EXPECT_EQ(first.value()->handle.name, "diagnostics.refresh");

        auto done = planner.next_stage();
        ASSERT_TRUE(done.has_value());
        EXPECT_FALSE(done.value().has_value());

        planner.reset_iteration();
        auto repeat = planner.next_stage();
        ASSERT_TRUE(repeat.has_value());
        ASSERT_TRUE(repeat.value().has_value());
        EXPECT_EQ(repeat.value()->handle.name, "diagnostics.refresh");

        planner.clear_plan();
        EXPECT_FALSE(planner.has_plan());
        auto after_clear = planner.next_stage();
        ASSERT_FALSE(after_clear.has_value());
        EXPECT_EQ(after_clear.error().code(), RuntimeError::loop_stage_planner_unconfigured);
    }
} // namespace engine::runtime
