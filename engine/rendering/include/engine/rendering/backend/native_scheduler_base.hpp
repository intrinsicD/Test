#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering::backend
{
    /**
     * \brief Scheduler base that manages command buffer allocation via a resource provider.
     */
    template <typename Derived, typename Submission>
    class NativeSchedulerBase : public IGpuScheduler
    {
    public:
        explicit NativeSchedulerBase(resources::IGpuResourceProvider& provider)
            : provider_(provider)
        {
        }

        CommandBufferHandle request_command_buffer(QueueType queue, std::string_view pass_name) override
        {
            CommandBufferHandle handle{++next_command_buffer_};
            auto native = provider_.allocate_command_buffer(queue, pass_name, handle);
            encoders_.push_back(EncoderRecord{handle, queue, std::string{pass_name}, native});
            return handle;
        }

        void submit(const GpuSubmitInfo& info) override
        {
            const auto* encoder = encoder_for(info.command_buffer);
            if (encoder == nullptr)
            {
                throw std::runtime_error{"NativeSchedulerBase received unknown command buffer"};
            }

            auto submission = static_cast<Derived*>(this)->build_submission(info, *encoder);
            submissions_.push_back(std::move(submission));

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
            provider_.recycle_command_buffer(handle);
        }

        [[nodiscard]] const std::vector<Submission>& submissions() const noexcept
        {
            return submissions_;
        }

    protected:
        struct EncoderRecord
        {
            CommandBufferHandle handle{};
            QueueType queue{QueueType::Graphics};
            std::string label;
            resources::CommandBufferNativeHandle native;
        };

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

        resources::IGpuResourceProvider& provider_;
        std::vector<EncoderRecord> encoders_{};
        std::vector<Submission> submissions_{};
        std::size_t next_command_buffer_{0};
    };
}
