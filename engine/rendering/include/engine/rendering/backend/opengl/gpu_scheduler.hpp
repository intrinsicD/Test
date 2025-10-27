#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/rendering/backend/native_scheduler_base.hpp"
#include "engine/rendering/backend/opengl/command_encoder.hpp"
#include "engine/rendering/backend/opengl/resource_provider.hpp"

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

    class CommandStream;

    CommandStream& default_command_stream() noexcept;

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
        std::vector<GeometryDrawCommand> draw_commands;

        [[nodiscard]] std::uint32_t begin_memory_barrier_mask() const noexcept
        {
            std::uint32_t mask = 0U;
            for (const auto& barrier : begin_barriers)
            {
                mask |= barrier.memory_barrier_mask;
            }
            return mask;
        }

        [[nodiscard]] std::uint32_t end_memory_barrier_mask() const noexcept
        {
            std::uint32_t mask = 0U;
            for (const auto& barrier : end_barriers)
            {
                mask |= barrier.memory_barrier_mask;
            }
            return mask;
        }
    };

    /// Interface that consumes translated OpenGL submissions and drives the command stream.
    class CommandStream
    {
    public:
        virtual ~CommandStream() = default;

        virtual void begin_submission(const OpenGLSubmission& submission) = 0;
        virtual void wait_timeline(const OpenGLTimelineSubmit& submit) = 0;
        virtual void issue_memory_barrier(std::uint32_t mask) = 0;
        virtual void execute_command_buffer(const OpenGLCommandEncoderSubmit& submit) = 0;
        virtual void signal_timeline(const OpenGLTimelineSubmit& submit) = 0;
        virtual void signal_fence(resources::FenceNativeHandle fence, std::uint64_t value) = 0;
        virtual void end_submission(const OpenGLSubmission& submission) = 0;
    };

    /// Scheduler that maps frame-graph work onto an OpenGL command stream.
    class OpenGLGpuScheduler final : public backend::NativeSchedulerBase<OpenGLGpuScheduler, OpenGLSubmission>
    {
    public:
        using Base = backend::NativeSchedulerBase<OpenGLGpuScheduler, OpenGLSubmission>;

        explicit OpenGLGpuScheduler(resources::IGpuResourceProvider& provider, CommandStream* stream = nullptr)
            : Base(provider)
            , command_stream_{stream != nullptr ? stream : &default_command_stream()}
        {
            if (provider.api() != resources::GraphicsApi::OpenGL)
            {
                throw std::invalid_argument{"OpenGLGpuScheduler requires an OpenGL resource provider"};
            }
        }

        void submit(const GpuSubmitInfo& info) override
        {
            const auto* encoder = this->encoder_for(info.command_buffer);
            if (encoder == nullptr)
            {
                throw std::runtime_error{"NativeSchedulerBase received unknown command buffer"};
            }

            auto submission = build_submission(info, *encoder);

            command_stream_->begin_submission(submission);
            for (const auto& wait : submission.waits)
            {
                command_stream_->wait_timeline(wait);
            }

            const auto begin_mask = submission.begin_memory_barrier_mask();
            if (begin_mask != 0U)
            {
                command_stream_->issue_memory_barrier(begin_mask);
            }

            command_stream_->execute_command_buffer(submission.command_buffer);

            const auto end_mask = submission.end_memory_barrier_mask();
            if (end_mask != 0U)
            {
                command_stream_->issue_memory_barrier(end_mask);
            }

            for (const auto& signal : submission.signals)
            {
                command_stream_->signal_timeline(signal);
            }

            if (submission.fence.api != resources::GraphicsApi::Unknown || submission.fence.value != 0U)
            {
                command_stream_->signal_fence(submission.fence, submission.fence_value);
            }

            command_stream_->end_submission(submission);

            submissions_.push_back(submission);

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
                submission.begin_barriers.push_back(translate_barrier(barrier));
            }

            submission.end_barriers.reserve(info.end_barriers.size());
            for (const auto& barrier : info.end_barriers)
            {
                submission.end_barriers.push_back(translate_barrier(barrier));
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

            if (auto* opengl_provider = dynamic_cast<OpenGLGpuResourceProvider*>(&provider_); opengl_provider != nullptr)
            {
                if (const auto* recorded_buffer = opengl_provider->command_buffer(encoder.handle); recorded_buffer != nullptr)
                {
                    submission.draw_commands = recorded_buffer->draws;
                }
            }

            return submission;
        }

    private:
        CommandStream* command_stream_;

        [[nodiscard]] static constexpr OpenGLBarrier translate_barrier(const resources::Barrier& barrier) noexcept
        {
            OpenGLBarrier translated{};
            translated.barrier = barrier;
            translated.memory_barrier_mask = detail::barrier_mask(barrier);
            return translated;
        }
    };
}
