#include "engine/core/threading/io_thread_pool.hpp"

namespace engine::core::threading {

    namespace {
        [[nodiscard]] std::size_t priority_index(IoTaskPriority priority) noexcept
        {
            switch (priority)
            {
            case IoTaskPriority::High:
                return 0;
            case IoTaskPriority::Normal:
                return 1;
            case IoTaskPriority::Low:
            default:
                return 2;
            }
        }
    } // namespace

    IoThreadPool::IoThreadPool() = default;

    IoThreadPool::~IoThreadPool()
    {
        shutdown();
    }

    IoThreadPool& IoThreadPool::instance()
    {
        static IoThreadPool pool;
        return pool;
    }

    void IoThreadPool::configure(const IoThreadPoolConfig& config)
    {
        std::unique_lock lock{mutex_};

        if (!config.enable || config.worker_count == 0)
        {
            config_ = config;
            shutdown_locked(lock);
            return;
        }

        if (config == config_ && !workers_.empty())
        {
            return;
        }

        shutdown_locked(lock);
        config_ = config;
        start_workers_locked();
    }

    void IoThreadPool::shutdown()
    {
        std::unique_lock lock{mutex_};
        shutdown_locked(lock);
    }

    bool IoThreadPool::enqueue(IoTaskPriority priority, std::function<void()> task)
    {
        std::unique_lock lock{mutex_};
        if (workers_.empty() || stopping_)
        {
            return false;
        }

        const auto total_pending = queues_[0].size() + queues_[1].size() + queues_[2].size();
        if (total_pending >= config_.queue_capacity)
        {
            return false;
        }

        queues_[priority_index(priority)].push(std::move(task));
        total_enqueued_.fetch_add(1, std::memory_order_relaxed);
        condition_.notify_one();
        return true;
    }

    IoThreadPoolStatistics IoThreadPool::statistics() const
    {
        std::unique_lock lock{mutex_};
        IoThreadPoolStatistics snapshot{};
        snapshot.configured_workers = config_.worker_count;
        snapshot.queue_capacity = config_.queue_capacity;
        snapshot.pending_tasks = queues_[0].size() + queues_[1].size() + queues_[2].size();
        snapshot.active_workers = active_workers_.load(std::memory_order_relaxed);
        snapshot.total_enqueued = total_enqueued_.load(std::memory_order_relaxed);
        snapshot.total_executed = total_executed_.load(std::memory_order_relaxed);
        return snapshot;
    }

    void IoThreadPool::start_workers_locked()
    {
        stopping_ = false;
        workers_.reserve(config_.worker_count);
        for (std::size_t index = 0; index < config_.worker_count; ++index)
        {
            workers_.emplace_back([this]() {
                for (;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock lock{mutex_};
                        condition_.wait(lock, [this]() {
                            return stopping_ || !queues_[0].empty() || !queues_[1].empty() || !queues_[2].empty();
                        });

                        if (stopping_ && queues_[0].empty() && queues_[1].empty() && queues_[2].empty())
                        {
                            return;
                        }

                        if (auto entry = pop_locked())
                        {
                            task = std::move(entry->task);
                        }
                        else
                        {
                            continue;
                        }
                    }

                    active_workers_.fetch_add(1, std::memory_order_relaxed);
                    try
                    {
                        task();
                    }
                    catch (...)
                    {
                        // Intentionally swallow exceptions to keep the worker alive.
                    }
                    active_workers_.fetch_sub(1, std::memory_order_relaxed);
                    total_executed_.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
    }

    void IoThreadPool::shutdown_locked(std::unique_lock<std::mutex>& lock)
    {
        stopping_ = true;
        condition_.notify_all();

        std::vector<std::thread> workers;
        workers.swap(workers_);

        lock.unlock();
        for (auto& worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
        workers.clear();
        lock.lock();

        while (!queues_[0].empty())
        {
            queues_[0].pop();
        }
        while (!queues_[1].empty())
        {
            queues_[1].pop();
        }
        while (!queues_[2].empty())
        {
            queues_[2].pop();
        }

        stopping_ = false;
        active_workers_.store(0, std::memory_order_relaxed);
    }

    std::optional<IoThreadPool::TaskEntry> IoThreadPool::pop_locked()
    {
        for (std::size_t priority = 0; priority < priority_count; ++priority)
        {
            auto& queue = queues_[priority];
            if (!queue.empty())
            {
                auto task = std::move(queue.front());
                queue.pop();
                return TaskEntry{static_cast<IoTaskPriority>(priority), std::move(task)};
            }
        }
        return std::nullopt;
    }

}  // namespace engine::core::threading

