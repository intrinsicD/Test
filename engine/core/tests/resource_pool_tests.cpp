#include <gtest/gtest.h>

#include "engine/core/memory/resource_pool.hpp"

namespace {

struct IntTag {};

}  // namespace

TEST(ResourcePool, ReusesSlotsWithGeneration)
{
    engine::core::memory::ResourcePool<int, IntTag> pool;

    auto [handle_a, value_a] = pool.acquire(1);
    EXPECT_TRUE(pool.is_valid(handle_a));
    EXPECT_EQ(pool.active_count(), 1U);
    EXPECT_EQ(pool.get(handle_a), 1);

    pool.release(handle_a);
    EXPECT_FALSE(pool.is_valid(handle_a));
    EXPECT_EQ(pool.active_count(), 0U);

    auto [handle_b, value_b] = pool.acquire(2);
    EXPECT_EQ(handle_a.index, handle_b.index);
    EXPECT_NE(handle_a.generation, handle_b.generation);
    EXPECT_EQ(value_b, 2);

    EXPECT_TRUE(pool.is_valid(handle_b));
}

TEST(ResourcePool, ForEachVisitsActiveEntries)
{
    engine::core::memory::ResourcePool<int, IntTag> pool;
    auto [handle_a, value_a] = pool.acquire(3);
    auto [handle_b, value_b] = pool.acquire(4);

    value_a = 3;
    value_b = 4;

    int sum = 0;
    pool.for_each([&](const engine::core::memory::ResourcePool<int, IntTag>::handle_type& handle,
                      int& value) {
        (void)handle;
        sum += value;
    });

    EXPECT_EQ(sum, 7);

    pool.release(handle_a);
    pool.release(handle_b);
}
