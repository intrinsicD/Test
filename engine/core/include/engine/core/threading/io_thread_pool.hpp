#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace engine::core::threading {

    enum class IoTaskPriority : std::uint8_t
    {
        High = 0,
        Normal,
        Low
    };

    struct IoThreadPoolConfig
    {
        std::size_t worker_count{0};
        std::size_t queue_capacity{64};
        bool enable{true};

        [[nodiscard]] bool operator==(const IoThreadPoolConfig& other) const noexcept
        {
            return worker_count == other.worker_count && queue_capacity == other.queue_capacity &&
                   enable == other.enable;
        }

        [[nodiscard]] bool operator!=(const IoThreadPoolConfig& other) const noexcept
        {
            return !(*this == other);
        }
    };

    struct IoThreadPoolStatistics
    {
        std::size_t configured_workers{0};
        std::size_t queue_capacity{0};
        std::size_t pending_tasks{0};
        std::size_t active_workers{0};
        std::uint64_t total_enqueued{0};
        std::uint64_t total_executed{0};
    };

    class IoThreadPool
    {
    public:
        IoThreadPool();
        ~IoThreadPool();

        IoThreadPool(const IoThreadPool&) = delete;
        IoThreadPool& operator=(const IoThreadPool&) = delete;

        static IoThreadPool& instance();

        void configure(const IoThreadPoolConfig& config);
        void shutdown();

        [[nodiscard]] bool enqueue(IoTaskPriority priority, std::function<void()> task);
        [[nodiscard]] IoThreadPoolStatistics statistics() const;

    private:
        struct TaskEntry
        {
            IoTaskPriority priority;
            std::function<void()> task;
        };

        void start_workers_locked();
        void shutdown_locked(std::unique_lock<std::mutex>& lock);
        std::optional<TaskEntry> pop_locked();

        static constexpr std::size_t priority_count = 3;

        IoThreadPoolConfig config_{};
        std::vector<std::thread> workers_{};
        mutable std::mutex mutex_{};
        std::condition_variable condition_{};
        std::array<std::queue<std::function<void()>>, priority_count> queues_{};
        bool stopping_{false};

        std::atomic<std::size_t> active_workers_{0};
        std::atomic<std::uint64_t> total_enqueued_{0};
        std::atomic<std::uint64_t> total_executed_{0};
    };

}  // namespace engine::core::threading

