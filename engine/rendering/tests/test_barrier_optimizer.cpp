#include <gtest/gtest.h>

#include "engine/rendering/barrier_optimizer.hpp"
#include "engine/rendering/frame_graph_types.hpp"

namespace
{
    using engine::rendering::BarrierOptimizer;
    using engine::rendering::FrameGraphResourceHandle;
    using engine::rendering::resources::Access;
    using engine::rendering::resources::Barrier;
    using engine::rendering::resources::PipelineStage;
}

TEST(BarrierOptimizer, RemovesNoOpBarriers)
{
    BarrierOptimizer optimizer;
    Barrier barrier{};
    barrier.resource = FrameGraphResourceHandle{0};
    barrier.source_stage = PipelineStage::Graphics;
    barrier.destination_stage = PipelineStage::Graphics;
    barrier.source_access = Access::Read;
    barrier.destination_access = Access::Read;

    auto result = optimizer.optimize({barrier});

    EXPECT_TRUE(result.barriers.empty());
    EXPECT_EQ(result.eliminated_count, 1U);
}

TEST(BarrierOptimizer, MergesSequentialTransitions)
{
    BarrierOptimizer optimizer;
    Barrier first{};
    first.resource = FrameGraphResourceHandle{1};
    first.source_stage = PipelineStage::Graphics;
    first.destination_stage = PipelineStage::Graphics;
    first.source_access = Access::Write;
    first.destination_access = Access::Read;

    Barrier second{};
    second.resource = first.resource;
    second.source_stage = PipelineStage::Graphics;
    second.destination_stage = PipelineStage::Compute;
    second.source_access = Access::Read;
    second.destination_access = Access::Write;

    auto result = optimizer.optimize({first, second});

    ASSERT_EQ(result.barriers.size(), 1U);
    EXPECT_EQ(result.eliminated_count, 1U);
    EXPECT_EQ(result.barriers.front().destination_stage, PipelineStage::Compute);
    EXPECT_EQ(result.barriers.front().destination_access, Access::Write);
}

TEST(BarrierOptimizer, DropsDuplicateTransitionsForSameResource)
{
    BarrierOptimizer optimizer;
    Barrier barrier{};
    barrier.resource = FrameGraphResourceHandle{2};
    barrier.source_stage = PipelineStage::Graphics;
    barrier.destination_stage = PipelineStage::Graphics;
    barrier.source_access = Access::Write;
    barrier.destination_access = Access::Read;

    std::vector<Barrier> barriers{barrier};
    Barrier duplicate = barrier;
    duplicate.source_access = Access::Read;
    barriers.push_back(duplicate);

    auto result = optimizer.optimize(std::move(barriers));

    ASSERT_EQ(result.barriers.size(), 1U);
    EXPECT_EQ(result.eliminated_count, 1U);
    EXPECT_EQ(result.barriers.front().destination_access, Access::Read);
}
