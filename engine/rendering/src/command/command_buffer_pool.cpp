#include "engine/rendering/command/command_buffer_pool.hpp"

#include <algorithm>
#include <utility>

namespace engine::rendering
{
    CommandBufferPool::CommandBufferPool(resources::IGpuResourceProvider& provider,
                                         std::uint64_t max_age_frames) noexcept
        : provider_(provider)
        , max_age_frames_{max_age_frames}
    {
    }

    void CommandBufferPool::begin_frame(std::uint64_t frame_index) noexcept
    {
        current_frame_ = frame_index;
    }

    void CommandBufferPool::end_frame()
    {
        trim_unused();
    }

    CommandBufferPool::AcquireResult CommandBufferPool::acquire(QueueType queue, std::string_view label)
    {
        ++requests_;
        const auto queue_index = queue_to_index(queue);
        AcquireResult result{};
        if (!available_[queue_index].empty())
        {
            result.handle = available_[queue_index].back();
            available_[queue_index].pop_back();
            ++hits_;
            if (auto it = metadata_.find(result.handle.index); it != metadata_.end())
            {
                it->second.in_use = true;
            }
            result.recycled = true;
        }
        else
        {
            result.handle.index = next_handle_index_++;
            metadata_.emplace(result.handle.index, BufferMetadata{queue_index, current_frame_, true});
        }

        result.native = provider_.allocate_command_buffer(queue, label, result.handle);
        if (auto it = metadata_.find(result.handle.index); it != metadata_.end())
        {
            it->second.queue_index = queue_index;
            it->second.in_use = true;
        }
        return result;
    }

    void CommandBufferPool::release(CommandBufferHandle handle)
    {
        provider_.recycle_command_buffer(handle);
        auto it = metadata_.find(handle.index);
        if (it == metadata_.end())
        {
            return;
        }

        auto& metadata = it->second;
        metadata.in_use = false;
        metadata.last_frame_used = current_frame_;
        available_[metadata.queue_index].push_back(handle);
    }

    void CommandBufferPool::trim_unused()
    {
        if (max_age_frames_ == 0)
        {
            return;
        }

        for (std::size_t queue_index = 0; queue_index < available_.size(); ++queue_index)
        {
            auto& pool = available_[queue_index];
            auto it = pool.begin();
            while (it != pool.end())
            {
                auto metadata_it = metadata_.find(it->index);
                if (metadata_it == metadata_.end())
                {
                    it = pool.erase(it);
                    continue;
                }

                const auto& metadata = metadata_it->second;
                if (metadata.in_use || metadata.last_frame_used == 0)
                {
                    ++it;
                    continue;
                }

                if (current_frame_ <= metadata.last_frame_used
                    || (current_frame_ - metadata.last_frame_used) <= max_age_frames_)
                {
                    ++it;
                    continue;
                }

                metadata_.erase(metadata_it);
                it = pool.erase(it);
                ++trims_;
            }
        }
    }

    void CommandBufferPool::set_max_age_frames(std::uint64_t frames) noexcept
    {
        max_age_frames_ = frames;
    }

    std::uint64_t CommandBufferPool::max_age_frames() const noexcept
    {
        return max_age_frames_;
    }

    CommandBufferPool::Metrics CommandBufferPool::metrics() const noexcept
    {
        Metrics snapshot{};
        snapshot.requests = requests_;
        snapshot.hits = hits_;
        snapshot.trims = trims_;
        snapshot.live_handles = metadata_.size();
        std::size_t pooled = 0;
        for (const auto& pool : available_)
        {
            pooled += pool.size();
        }
        snapshot.pooled_handles = pooled;
        return snapshot;
    }
}
