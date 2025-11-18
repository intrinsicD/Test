#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "engine/geometry/shapes/frustum.hpp"
#include "engine/scene/selection/primitive_selection.hpp"

namespace
{
    using namespace engine::scene::selection;

    class CountingAdapter final : public PrimitiveSelectionAdapter
    {
    public:
        explicit CountingAdapter(std::size_t hits_per_call) : hits_per_call_(hits_per_call) {}

        [[nodiscard]] bool supports(const PrimitivePickRequest&) const noexcept override
        {
            return true;
        }

        void gather_primitives(const PrimitivePickRequest& request, PrimitiveHitBuffer& out) const override
        {
            for (std::size_t i = 0; i < hits_per_call_ && !out.full(); ++i)
            {
                PrimitiveHit hit{};
                hit.entity = request.hit.entity;
                hit.primitive = SelectionPrimitive::Vertex;
                hit.index0 = static_cast<std::uint32_t>(i);
                out.append(hit);
            }
        }

        void marquee_select(const MarqueeRequest& request, PrimitiveHitBuffer& out) const override
        {
            for (std::size_t i = 0; i < hits_per_call_ && !out.full(); ++i)
            {
                PrimitiveHit hit{};
                hit.entity = request.context.scene ? entt::entity{1} : entt::null;
                hit.primitive = SelectionPrimitive::Point;
                hit.index0 = static_cast<std::uint32_t>(i + 10);
                out.append(hit);
            }
        }

    private:
        std::size_t hits_per_call_{0};
    };
}

TEST(PrimitiveHitBuffer, EnforcesMaximumCapacity)
{
    PrimitiveHitBuffer buffer;
    buffer.set_max_primitives(2);

    PrimitiveHit hit{};
    hit.entity = static_cast<entt::entity>(1);

    EXPECT_TRUE(buffer.append(hit));
    EXPECT_TRUE(buffer.append(hit));
    EXPECT_FALSE(buffer.append(hit));
    EXPECT_EQ(buffer.size(), 2U);
}

TEST(PrimitiveHitBuffer, ProvidesChunkedIteration)
{
    PrimitiveHitBuffer buffer;
    buffer.set_chunk_size(2);

    PrimitiveHit hit{};
    hit.entity = static_cast<entt::entity>(1);

    for (int i = 0; i < 5; ++i)
    {
        hit.index0 = static_cast<std::uint32_t>(i);
        buffer.append(hit);
    }

    std::vector<std::size_t> chunk_sizes;
    for (auto chunk : buffer.chunks())
    {
        chunk_sizes.push_back(chunk.size());
    }

    ASSERT_EQ(chunk_sizes.size(), 3U);
    EXPECT_EQ(chunk_sizes[0], 2U);
    EXPECT_EQ(chunk_sizes[1], 2U);
    EXPECT_EQ(chunk_sizes[2], 1U);
}

TEST(PrimitiveSelectionRegistry, StopsAtCapacityWhenGathering)
{
    PrimitiveSelectionRegistry registry;
    registry.register_adapter(std::make_unique<CountingAdapter>(3));
    registry.register_adapter(std::make_unique<CountingAdapter>(5));

    PrimitivePickRequest request{};
    request.hit.entity = static_cast<entt::entity>(42);
    request.max_results = 4;

    const auto buffer = registry.gather_primitives(request);
    ASSERT_EQ(buffer.size(), 4U);

    std::vector<std::uint32_t> indices;
    for (const auto& hit : buffer.hits())
    {
        indices.push_back(hit.index0);
    }

    EXPECT_EQ(indices, (std::vector<std::uint32_t>{0, 1, 2, 0}));
}

TEST(PrimitiveSelectionRegistry, AggregatesMarqueeSelections)
{
    PrimitiveSelectionRegistry registry;
    registry.register_adapter(std::make_unique<CountingAdapter>(2));

    MarqueeRequest request{};
    request.max_results = 3;

    const auto buffer = registry.marquee_select(request);
    ASSERT_EQ(buffer.size(), 2U);

    for (const auto& hit : buffer.hits())
    {
        EXPECT_EQ(hit.primitive, SelectionPrimitive::Point);
    }
}
