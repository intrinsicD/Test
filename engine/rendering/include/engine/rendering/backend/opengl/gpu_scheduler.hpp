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

    inline constexpr std::uint32_t command_barrier_bit = 0x00000040U;
    inline constexpr std::uint32_t uniform_barrier_bit = 0x00000004U;
    inline constexpr std::uint32_t texture_fetch_barrier_bit = 0x00000008U;
    inline constexpr std::uint32_t shader_image_access_barrier_bit = 0x00000020U;
    inline constexpr std::uint32_t shader_storage_barrier_bit = 0x00002000U;
    inline constexpr std::uint32_t atomic_counter_barrier_bit = 0x00001000U;
    inline constexpr std::uint32_t pixel_buffer_barrier_bit = 0x00000080U;
    inline constexpr std::uint32_t texture_update_barrier_bit = 0x00000100U;
    inline constexpr std::uint32_t buffer_update_barrier_bit = 0x00000200U;
    inline constexpr std::uint32_t framebuffer_barrier_bit = 0x00000400U;

    namespace detail
    {
        [[nodiscard]] constexpr std::uint32_t stage_access_mask(resources::PipelineStage stage,
                                                                resources::Access access) noexcept
        {
            if (access == resources::Access::None)
            {
                return 0U;
            }

            switch (stage)
            {
            case resources::PipelineStage::Graphics:
                if (access == resources::Access::Read)
                {
                    return command_barrier_bit | uniform_barrier_bit | texture_fetch_barrier_bit;
                }
                return framebuffer_barrier_bit | texture_update_barrier_bit | buffer_update_barrier_bit;
            case resources::PipelineStage::Compute:
                if (access == resources::Access::Read)
                {
                    return shader_image_access_barrier_bit | uniform_barrier_bit | texture_fetch_barrier_bit;
                }
                return shader_image_access_barrier_bit | shader_storage_barrier_bit | atomic_counter_barrier_bit;
            case resources::PipelineStage::Transfer:
                if (access == resources::Access::Read)
                {
                    return pixel_buffer_barrier_bit | texture_fetch_barrier_bit;
                }
                return pixel_buffer_barrier_bit | buffer_update_barrier_bit | texture_update_barrier_bit;
            }
            return 0U;
        }

        [[nodiscard]] constexpr std::uint32_t barrier_mask(const resources::Barrier& barrier) noexcept
        {
            return stage_access_mask(barrier.source_stage, barrier.source_access)
                   | stage_access_mask(barrier.destination_stage, barrier.destination_access);
        }
    }  // namespace detail

    struct OpenGLBarrier
    {
        resources::Barrier barrier{};
        std::uint32_t memory_barrier_mask{0};
    };

    struct OpenGLSubmission
    {
        std::string pass_name;
        OpenGLCommandEncoderSubmit command_buffer;
        std::vector<OpenGLBarrier> begin_barriers;
        std::vector<OpenGLBarrier> end_barriers;
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

        QueueType select_queue(const RenderPass& pass, QueueType preferred) override
        {
            static_cast<void>(pass);
            if (preferred == QueueType::Graphics)
            {
                return QueueType::Graphics;
            }
            return QueueType::Graphics;
        }

        [[nodiscard]] OpenGLSubmission build_submission(const GpuSubmitInfo& info,
                                                         const typename Base::EncoderRecord& encoder)
        {
            OpenGLSubmission submission{};
            submission.pass_name = std::string{info.pass_name};
            submission.command_buffer.queue = provider_.queue_handle(info.queue);
            submission.command_buffer.command_buffer = encoder.native;
            submission.fence_value = info.fence_value;

            submission.begin_barriers.reserve(info.begin_barriers.size());
            for (const auto& barrier : info.begin_barriers)
            {
                OpenGLBarrier translated{};
                translated.barrier = barrier;
                translated.memory_barrier_mask = detail::barrier_mask(barrier);
                submission.begin_barriers.push_back(translated);
            }

            submission.end_barriers.reserve(info.end_barriers.size());
            for (const auto& barrier : info.end_barriers)
            {
                OpenGLBarrier translated{};
                translated.barrier = barrier;
                translated.memory_barrier_mask = detail::barrier_mask(barrier);
                submission.end_barriers.push_back(translated);
            }

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
