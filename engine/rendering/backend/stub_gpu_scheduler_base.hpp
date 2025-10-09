#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/rendering/gpu_scheduler.hpp"

namespace engine::rendering::backend
{
    /// Shared implementation for backend stub schedulers used in tests and software paths.
    class StubGpuSchedulerBase : public IGpuScheduler
    {
    public:
        struct EncoderRecord
        {
            CommandBufferHandle handle{};
            QueueType queue{QueueType::Graphics};
            std::string label;
        };

        struct SubmissionRecord
        {
            std::string pass_name;
            QueueType queue{QueueType::Graphics};
            CommandBufferHandle command_buffer{};
            std::vector<resources::Barrier> begin_barriers;
            std::vector<resources::Barrier> end_barriers;
            std::vector<resources::SemaphoreWait> waits;
            std::vector<resources::SemaphoreSignal> signals;
            std::uint64_t fence_value{0};
        };

        QueueType select_queue(const RenderPass&) override
        {
            return QueueType::Graphics;
        }

        CommandBufferHandle request_command_buffer(QueueType queue, std::string_view pass_name) override
        {
            CommandBufferHandle handle{++next_command_buffer_};
            encoders_.push_back(EncoderRecord{handle, queue, std::string{pass_name}});
            return handle;
        }

        void submit(const GpuSubmitInfo& info) override
        {
            SubmissionRecord record{};
            record.pass_name = std::string{info.pass_name};
            record.queue = info.queue;
            record.command_buffer = info.command_buffer;
            record.begin_barriers = info.begin_barriers;
            record.end_barriers = info.end_barriers;
            record.waits = info.waits;
            record.signals = info.signals;
            record.fence_value = info.fence_value;
            submissions_.push_back(std::move(record));

            if (info.fence != nullptr)
            {
                info.fence->signal(info.fence_value);
            }
            for (const auto& wait : info.waits)
            {
                if (wait.semaphore != nullptr)
                {
                    wait.semaphore->wait(wait.value);
                }
            }
            for (const auto& signal : info.signals)
            {
                if (signal.semaphore != nullptr)
                {
                    signal.semaphore->signal(signal.value);
                }
            }
        }

        void recycle(CommandBufferHandle handle) override
        {
            recycled_.push_back(handle);
        }

        [[nodiscard]] const std::vector<SubmissionRecord>& submissions() const noexcept
        {
            return submissions_;
        }

        [[nodiscard]] const std::vector<EncoderRecord>& encoders() const noexcept
        {
            return encoders_;
        }

        [[nodiscard]] const EncoderRecord* encoder_for(CommandBufferHandle handle) const noexcept
        {
            const auto it = std::find_if(encoders_.begin(), encoders_.end(),
                                         [handle](const EncoderRecord& record) {
                                             return record.handle == handle;
                                         });
            if (it != encoders_.end())
            {
                return &*it;
            }
            return nullptr;
        }

        [[nodiscard]] const std::vector<CommandBufferHandle>& recycled_buffers() const noexcept
        {
            return recycled_;
        }

    protected:
        std::size_t next_command_buffer_{0};
        std::vector<EncoderRecord> encoders_{};
        std::vector<SubmissionRecord> submissions_{};
        std::vector<CommandBufferHandle> recycled_{};
    };
}

