#include "engine/rendering/gpu_profiler.hpp"

#include <algorithm>

namespace engine::rendering
{
    namespace
    {
        [[nodiscard]] std::uint64_t duration_to_ns(std::chrono::steady_clock::duration duration) noexcept
        {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
        }
    } // namespace

    void GpuProfiler::set_enabled(bool enabled) noexcept
    {
        enabled_ = enabled;
    }

    void GpuProfiler::begin_frame(resources::GraphicsApi api) noexcept
    {
        api_ = api;
        ++frame_index_;
        pending_.clear();
        completed_.clear();
    }

    void GpuProfiler::end_frame() noexcept
    {
        pending_.clear();
    }

    void GpuProfiler::begin_pass(std::string_view pass_name, QueueType queue, CommandBufferHandle handle)
    {
        if (!enabled_ || !handle.valid())
        {
            return;
        }

        PendingPass pending{};
        pending.name.assign(pass_name.begin(), pass_name.end());
        pending.queue = queue;
        pending.command_buffer = handle;
        pending.begin_ns = now_ns();

        pending_[handle.index] = std::move(pending);
    }

    void GpuProfiler::end_pass(CommandBufferHandle handle)
    {
        if (!enabled_ || !handle.valid())
        {
            return;
        }

        const auto it = pending_.find(handle.index);
        if (it == pending_.end())
        {
            return;
        }

        const auto end_ns = now_ns();
        PassTiming timing{};
        timing.pass_name = std::move(it->second.name);
        timing.queue = it->second.queue;
        timing.command_buffer = it->second.command_buffer;
        timing.timestamp_begin_ns = it->second.begin_ns;
        timing.timestamp_end_ns = end_ns;
        const auto elapsed_ns = end_ns > it->second.begin_ns ? end_ns - it->second.begin_ns : 0U;
        timing.gpu_time_ms = static_cast<double>(elapsed_ns) / 1'000'000.0;

        completed_.push_back(std::move(timing));
        pending_.erase(it);
    }

    const std::vector<GpuProfiler::PassTiming>& GpuProfiler::pass_timings() const noexcept
    {
        return completed_;
    }

    std::vector<GpuProfiler::PassTiming> GpuProfiler::consume_pass_timings()
    {
        auto timings = std::move(completed_);
        completed_.clear();
        return timings;
    }

    std::uint64_t GpuProfiler::now_ns() noexcept
    {
        return duration_to_ns(Clock::now().time_since_epoch());
    }
} // namespace engine::rendering

