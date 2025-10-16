#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/backend/native_scheduler_base.hpp"
#include "engine/rendering/frame_graph.hpp"

namespace engine::rendering::backend::directx12
{
    struct DirectX12TimelineSubmit
    {
        resources::TimelineSemaphoreNativeHandle semaphore{};
        std::uint64_t value{0};
    };

    struct DirectX12CommandListSubmit
    {
        resources::QueueNativeHandle queue{};
        resources::CommandBufferNativeHandle command_list{};
    };

    struct DirectX12Submission
    {
        std::string pass_name;
        DirectX12CommandListSubmit command_list;
        std::vector<resources::Barrier> begin_barriers;
        std::vector<resources::Barrier> end_barriers;
        std::vector<DirectX12TimelineSubmit> waits;
        std::vector<DirectX12TimelineSubmit> signals;
        resources::FenceNativeHandle fence{};
        std::uint64_t fence_value{0};
    };

    /// GPU scheduler that converts frame-graph submissions into DirectX 12 command queue work.
    class DirectX12GpuScheduler final
        : public backend::NativeSchedulerBase<DirectX12GpuScheduler, DirectX12Submission>
    {
    public:
        using Base = backend::NativeSchedulerBase<DirectX12GpuScheduler, DirectX12Submission>;

        explicit DirectX12GpuScheduler(resources::IGpuResourceProvider& provider)
            : Base(provider)
        {
        }

        QueueType select_queue(const RenderPass& pass, QueueType preferred) override
        {
            if (preferred != QueueType::Graphics)
            {
                return preferred;
            }
            const auto name = pass.name();
            if (name.find("Copy") != std::string_view::npos)
            {
                return QueueType::Transfer;
            }
            if (name.find("Compute") != std::string_view::npos)
            {
                return QueueType::Compute;
            }
            return QueueType::Graphics;
        }

        [[nodiscard]] DirectX12Submission build_submission(const GpuSubmitInfo& info,
                                                            const typename Base::EncoderRecord& encoder)
        {
            DirectX12Submission submission{};
            submission.pass_name = std::string{info.pass_name};
            submission.command_list.queue = provider_.queue_handle(info.queue);
            submission.command_list.command_list = encoder.native;
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
                DirectX12TimelineSubmit submit{};
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
                DirectX12TimelineSubmit submit{};
                submit.semaphore = provider_.resolve_semaphore(*signal.semaphore);
                submit.value = signal.value;
                submission.signals.push_back(submit);
            }

            return submission;
        }
    };
}
