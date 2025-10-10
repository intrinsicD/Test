#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "engine/rendering/backend/native_scheduler_base.hpp"

namespace engine::rendering::backend::opengl
{
    struct OpenGLTimelineSubmit
    {
        resources::TimelineSemaphoreNativeHandle semaphore{};
        std::uint64_t value{0};
    };

    struct OpenGLCommandEncoderSubmit
    {
        resources::QueueNativeHandle queue{};
        resources::CommandBufferNativeHandle command_buffer{};
    };

    struct OpenGLSubmission
    {
        std::string pass_name;
        OpenGLCommandEncoderSubmit command_buffer;
        std::vector<resources::Barrier> begin_barriers;
        std::vector<resources::Barrier> end_barriers;
        std::vector<OpenGLTimelineSubmit> waits;
        std::vector<OpenGLTimelineSubmit> signals;
        resources::FenceNativeHandle fence{};
        std::uint64_t fence_value{0};
    };

    /// Scheduler that maps frame-graph work onto an OpenGL command stream.
    class OpenGLGpuScheduler final : public backend::NativeSchedulerBase<OpenGLGpuScheduler, OpenGLSubmission>
    {
    public:
        using Base = backend::NativeSchedulerBase<OpenGLGpuScheduler, OpenGLSubmission>;

        explicit OpenGLGpuScheduler(resources::IGpuResourceProvider& provider)
            : Base(provider)
        {
        }

        QueueType select_queue(const RenderPass&) override
        {
            return QueueType::Graphics;
        }

        [[nodiscard]] OpenGLSubmission build_submission(const GpuSubmitInfo& info,
                                                         const typename Base::EncoderRecord& encoder)
        {
            OpenGLSubmission submission{};
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
                OpenGLTimelineSubmit submit{};
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
                OpenGLTimelineSubmit submit{};
                submit.semaphore = provider_.resolve_semaphore(*signal.semaphore);
                submit.value = signal.value;
                submission.signals.push_back(submit);
            }

            return submission;
        }
    };
}
