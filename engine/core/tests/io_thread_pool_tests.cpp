#include <gtest/gtest.h>

#include "engine/core/threading/io_thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    using namespace std::chrono_literals;
}

class IoThreadPoolTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine::core::threading::IoThreadPool::instance().configure({.worker_count = 2, .queue_capacity = 8, .enable = true});
    }

    void TearDown() override
    {
        engine::core::threading::IoThreadPool::instance().shutdown();
    }
};

TEST_F(IoThreadPoolTest, ExecutesHigherPriorityFirst)
{
    auto& pool = engine::core::threading::IoThreadPool::instance();
    std::vector<int> order;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> remaining{3};

    auto record = [&](int value) {
        std::unique_lock lock{mutex};
        order.push_back(value);
        if (--remaining == 0)
        {
            cv.notify_all();
        }
    };

    ASSERT_TRUE(pool.enqueue(engine::core::threading::IoTaskPriority::Low, [&, record]() mutable {
        std::this_thread::sleep_for(10ms);
        record(2);
    }));
    ASSERT_TRUE(pool.enqueue(engine::core::threading::IoTaskPriority::High, [&, record]() mutable {
        record(0);
    }));
    ASSERT_TRUE(pool.enqueue(engine::core::threading::IoTaskPriority::Normal, [&, record]() mutable {
        record(1);
    }));

    std::unique_lock lock{mutex};
    cv.wait(lock, [&]() { return remaining.load() == 0; });

    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], 0);
}

TEST_F(IoThreadPoolTest, RejectsWhenQueueIsFull)
{
    auto& pool = engine::core::threading::IoThreadPool::instance();
    pool.shutdown();
    pool.configure({.worker_count = 1, .queue_capacity = 1, .enable = true});

    std::promise<void> gate;
    std::shared_future<void> ready = gate.get_future().share();

    ASSERT_TRUE(pool.enqueue(engine::core::threading::IoTaskPriority::Normal, [ready]() mutable {
        ready.wait();
    }));

    const bool accepted = pool.enqueue(engine::core::threading::IoTaskPriority::Normal, []() {});
    EXPECT_FALSE(accepted);

    gate.set_value();
}

TEST(IoThreadPoolStandalone, ShutdownWithoutConfigureIsSafe)
{
    auto& pool = engine::core::threading::IoThreadPool::instance();
    pool.shutdown();
    // No assertion needed; the test ensures there is no crash.
}

