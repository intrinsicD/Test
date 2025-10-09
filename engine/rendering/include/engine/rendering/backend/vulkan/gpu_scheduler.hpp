#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../native_scheduler_base.hpp"

namespace engine::rendering::backend::vulkan
{
    struct VulkanSemaphoreSubmit
    {
        resources::TimelineSemaphoreNativeHandle semaphore{};
        std::uint64_t value{0};
    };

    struct VulkanCommandBufferSubmit
    {
        resources::QueueNativeHandle queue{};
        resources::CommandBufferNativeHandle command_buffer{};
    };

    struct VulkanSubmission
    {
        std::string pass_name;
        VulkanCommandBufferSubmit command_buffer;
        std::vector<resources::Barrier> begin_barriers;
        std::vector<resources::Barrier> end_barriers;
        std::vector<VulkanSemaphoreSubmit> waits;
        std::vector<VulkanSemaphoreSubmit> signals;
        resources::FenceNativeHandle fence{};
        std::uint64_t fence_value{0};
    };

    /// GPU scheduler that translates frame-graph submissions into Vulkan primitives.
    class VulkanGpuScheduler final : public backend::NativeSchedulerBase<VulkanGpuScheduler, VulkanSubmission>
    {
    public:
        using Base = backend::NativeSchedulerBase<VulkanGpuScheduler, VulkanSubmission>;

        explicit VulkanGpuScheduler(resources::IGpuResourceProvider& provider)
            : Base(provider)
        {
        }

        QueueType select_queue(const RenderPass& pass) override
        {
            const auto name = pass.name();
            if (name.find("Transfer") != std::string_view::npos || name.find("Copy") != std::string_view::npos)
            {
                return QueueType::Transfer;
            }
            if (name.find("Compute") != std::string_view::npos)
            {
                return QueueType::Compute;
            }
            return QueueType::Graphics;
        }

        [[nodiscard]] VulkanSubmission build_submission(const GpuSubmitInfo& info,
                                                         const typename Base::EncoderRecord& encoder)
        {
            VulkanSubmission submission{};
            submission.pass_name = std::string{info.pass_name};
            submission.command_buffer.queue = provider_.queue_handle(info.queue);
            submission.command_buffer.command_buffer = encoder.native;
            submission.begin_barriers = info.begin_barriers;
            submission.end_barriers = info.end_barriers;
            submission.fence_value = info.fence_value;

            if (info.fence != nullptr)
            {
                submission.fence = provider_.resolve_fence(*info.fence);
            }

            submission.waits.reserve(info.waits.size());
            for (const auto& wait : info.waits)
            {
                if (wait.semaphore == nullptr)
                {
                    continue;
                }
                VulkanSemaphoreSubmit submit{};
                submit.semaphore = provider_.resolve_semaphore(*wait.semaphore);
                submit.value = wait.value;
                submission.waits.push_back(submit);
            }

            submission.signals.reserve(info.signals.size());
            for (const auto& signal : info.signals)
            {
                if (signal.semaphore == nullptr)
                {
                    continue;
                }
                VulkanSemaphoreSubmit submit{};
                submit.semaphore = provider_.resolve_semaphore(*signal.semaphore);
                submit.value = signal.value;
                submission.signals.push_back(submit);
            }

            return submission;
        }
    };
}
