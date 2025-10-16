#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/rendering/gpu_scheduler.hpp"

namespace engine::rendering::tests
{
    /// Test scheduler that records submissions for validation.
    class RecordingScheduler final : public IGpuScheduler
    {
    public:
        struct Submission
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

        QueueType select_queue(const RenderPass&, QueueType preferred) override
        {
            return preferred;
        }

        CommandBufferHandle request_command_buffer(QueueType, std::string_view) override
        {
            return CommandBufferHandle{++next_command_buffer_};
        }

        void submit(const GpuSubmitInfo& info) override
        {
            Submission record{};
            record.pass_name = std::string{info.pass_name};
            record.queue = info.queue;
            record.command_buffer = info.command_buffer;
            record.begin_barriers = info.begin_barriers;
            record.end_barriers = info.end_barriers;
            record.waits = info.waits;
            record.signals = info.signals;
            record.fence_value = info.fence_value;
            submissions.push_back(std::move(record));

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

        void recycle(CommandBufferHandle) override {}

        std::vector<Submission> submissions;

    private:
        std::size_t next_command_buffer_{0};
    };
}

