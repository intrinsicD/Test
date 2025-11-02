#include <gtest/gtest.h>

#include "engine/runtime/errors.hpp"
#include "engine/runtime/loop.hpp"

namespace engine::runtime
{

    TEST(RuntimeLoopBuilder, BuildsTopologicallySortedPlan)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("Simulation.Start", RuntimeLoopPhase::Simulation,
                                      [](double) {}).has_value());
        ASSERT_TRUE(builder.add_stage("Presentation.Prepare", RuntimeLoopPhase::Presentation,
                                      [](double) {}, {"Simulation.Start"}).has_value());
        ASSERT_TRUE(builder.add_stage("Diagnostics.Flush", RuntimeLoopPhase::Diagnostics,
                                      [](double) {}, {"Presentation.Prepare"}).has_value());

        const auto result = builder.build();
        ASSERT_TRUE(result.has_value());

        const auto& plan = result.value();
        ASSERT_EQ(plan.stages().size(), 3U);
        EXPECT_EQ(plan.stages()[0].name, "Simulation.Start");
        EXPECT_EQ(plan.stages()[1].name, "Presentation.Prepare");
        EXPECT_EQ(plan.stages()[2].name, "Diagnostics.Flush");
    }

    TEST(RuntimeLoopBuilder, RejectsDuplicateStageNames)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("Update", RuntimeLoopPhase::Simulation,
                                      [](double) {}).has_value());
        const auto duplicate = builder.add_stage("Update", RuntimeLoopPhase::Presentation,
                                                 [](double) {});
        ASSERT_FALSE(duplicate.has_value());
        EXPECT_EQ(duplicate.error().code(), RuntimeError::loop_stage_duplicate_name);
    }

    TEST(RuntimeLoopBuilder, RejectsUnknownDependencies)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("Presentation", RuntimeLoopPhase::Presentation,
                                      [](double) {}, {"Unknown"}).has_value());

        const auto result = builder.build();
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RuntimeError::loop_stage_unknown_dependency);
    }

    TEST(RuntimeLoopBuilder, RejectsDependencyCycles)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("A", RuntimeLoopPhase::Simulation, [](double) {}, {"C"}).has_value());
        ASSERT_TRUE(builder.add_stage("B", RuntimeLoopPhase::Presentation, [](double) {}, {"A"}).has_value());
        ASSERT_TRUE(builder.add_stage("C", RuntimeLoopPhase::Diagnostics, [](double) {}, {"B"}).has_value());

        const auto result = builder.build();
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(), RuntimeError::loop_stage_dependency_cycle);
    }

    TEST(RuntimeLoopPlan, FiltersStagesByPhase)
    {
        RuntimeLoopBuilder builder{};
        ASSERT_TRUE(builder.add_stage("Sim.A", RuntimeLoopPhase::Simulation, [](double) {}).has_value());
        ASSERT_TRUE(builder.add_stage("Sim.B", RuntimeLoopPhase::Simulation, [](double) {}, {"Sim.A"}).has_value());
        ASSERT_TRUE(builder.add_stage("Present.A", RuntimeLoopPhase::Presentation, [](double) {}, {"Sim.B"}).has_value());
        ASSERT_TRUE(builder.add_stage("Diag.A", RuntimeLoopPhase::Diagnostics, [](double) {}, {"Present.A"}).has_value());

        const auto plan_result = builder.build();
        ASSERT_TRUE(plan_result.has_value());
        const auto& plan = plan_result.value();

        const auto sim_stages = plan.stages_for_phase(RuntimeLoopPhase::Simulation);
        ASSERT_EQ(sim_stages.size(), 2U);
        EXPECT_EQ(sim_stages[0]->name, "Sim.A");
        EXPECT_EQ(sim_stages[1]->name, "Sim.B");

        const auto presentation = plan.stages_for_phase(RuntimeLoopPhase::Presentation);
        ASSERT_EQ(presentation.size(), 1U);
        EXPECT_EQ(presentation[0]->name, "Present.A");

        const auto diagnostics = plan.stages_for_phase(RuntimeLoopPhase::Diagnostics);
        ASSERT_EQ(diagnostics.size(), 1U);
        EXPECT_EQ(diagnostics[0]->name, "Diag.A");
    }
}
