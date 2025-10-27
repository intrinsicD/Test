#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/rendering/backend/directx12/gpu_scheduler.hpp"
#include "engine/rendering/backend/metal/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "engine/rendering/resources/synchronization.hpp"

namespace
{
    class RecordingOpenGLStream final : public engine::rendering::backend::opengl::CommandStream
    {
    public:
        enum class EventType
        {
            Begin,
            Wait,
            Barrier,
            Execute,
            Signal,
            Fence,
            End,
        };

        struct Event
        {
            EventType type;
            std::uint32_t mask{0};
            std::uint64_t value{0};
        };

        void begin_submission(const engine::rendering::backend::opengl::OpenGLSubmission& submission) override
        {
            last_submission = submission.pass_name;
            events.push_back(Event{EventType::Begin});
        }

        void wait_timeline(const engine::rendering::backend::opengl::OpenGLTimelineSubmit& submit) override
        {
            events.push_back(Event{EventType::Wait, 0U, submit.value});
        }

        void issue_memory_barrier(std::uint32_t mask) override
        {
            events.push_back(Event{EventType::Barrier, mask});
        }

        void execute_command_buffer(
            const engine::rendering::backend::opengl::OpenGLCommandEncoderSubmit& submit) override
        {
            executed_command = submit;
            events.push_back(Event{EventType::Execute});
        }

        void signal_timeline(const engine::rendering::backend::opengl::OpenGLTimelineSubmit& submit) override
        {
            events.push_back(Event{EventType::Signal, 0U, submit.value});
        }

        void signal_fence(engine::rendering::resources::FenceNativeHandle fence, std::uint64_t value) override
        {
            fence_handle = fence;
            events.push_back(Event{EventType::Fence, 0U, value});
        }

        void end_submission(const engine::rendering::backend::opengl::OpenGLSubmission& submission) override
        {
            events.push_back(Event{EventType::End, 0U, submission.fence_value});
        }

        std::string last_submission;
        engine::rendering::backend::opengl::OpenGLCommandEncoderSubmit executed_command{};
        engine::rendering::resources::FenceNativeHandle fence_handle{};
        std::vector<Event> events;
    };

    template <typename Scheduler>
    void verify_submission_translation(Scheduler& scheduler,
                                       engine::rendering::resources::RecordingGpuResourceProvider& provider,
                                       engine::rendering::QueueType queue_type)
    {
        using namespace engine::rendering;
        using engine::rendering::resources::Barrier;

        auto command_buffer = scheduler.request_command_buffer(queue_type, "AdapterPass");
        resources::TimelineSemaphore wait_semaphore{"Wait", 0};
        resources::TimelineSemaphore signal_semaphore{"Signal", 0};
        resources::Fence fence{"Fence"};

        GpuSubmitInfo info{};
        info.pass_name = "AdapterPass";
        info.queue = queue_type;
        info.command_buffer = command_buffer;
        Barrier begin{};
        begin.source_stage = resources::PipelineStage::Compute;
        begin.source_access = resources::Access::Write;
        begin.destination_stage = resources::PipelineStage::Graphics;
        begin.destination_access = resources::Access::Read;
        info.begin_barriers.push_back(begin);

        Barrier end{};
        end.source_stage = resources::PipelineStage::Graphics;
        end.source_access = resources::Access::Read;
        end.destination_stage = resources::PipelineStage::Transfer;
        end.destination_access = resources::Access::Write;
        info.end_barriers.push_back(end);
        info.waits.push_back(resources::SemaphoreWait{&wait_semaphore, 1});
        info.signals.push_back(resources::SemaphoreSignal{&signal_semaphore, 2});
        info.fence = &fence;
        info.fence_value = 3;

        scheduler.submit(info);
        scheduler.recycle(command_buffer);

        ASSERT_EQ(provider.command_buffers().count(command_buffer.index), 0U); // NOLINT
    }
}

TEST(BackendAdapters, VulkanSchedulerTranslatesToNativeHandles)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::Vulkan);
    backend::vulkan::VulkanGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass transfer_pass{
        "TransferCopy",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        }
    };
    const auto queue = scheduler.select_queue(transfer_pass, transfer_pass.queue());
    EXPECT_EQ(queue, QueueType::Transfer);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);

    ASSERT_EQ(scheduler.submissions().size(), 1); // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.pass_name, "AdapterPass");
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::Vulkan);
    EXPECT_EQ(submission.command_buffer.command_buffer.api, resources::GraphicsApi::Vulkan);
    ASSERT_EQ(submission.waits.size(), 1); // NOLINT
    EXPECT_EQ(submission.waits.front().value, 1U);
    ASSERT_EQ(submission.signals.size(), 1); // NOLINT
    EXPECT_EQ(submission.signals.front().value, 2U);
    EXPECT_EQ(submission.fence_value, 3U);
}

TEST(BackendAdapters, DirectX12SchedulerBuildsCommandLists)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::DirectX12);
    backend::directx12::DirectX12GpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass compute_pass{
        "Compute",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        }
    };
    const auto queue = scheduler.select_queue(compute_pass, compute_pass.queue());
    EXPECT_EQ(queue, QueueType::Compute);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1); // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_list.queue.api, resources::GraphicsApi::DirectX12);
}

TEST(BackendAdapters, MetalSchedulerBuildsCommandBuffers)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::Metal);
    backend::metal::MetalGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass blit_pass{
        "BlitResolve",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        }
    };
    const auto queue = scheduler.select_queue(blit_pass, blit_pass.queue());
    EXPECT_EQ(queue, QueueType::Transfer);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1); // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::Metal);
}

TEST(BackendAdapters, OpenGLSchedulerRecordsGraphicsQueue)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::OpenGL);
    RecordingOpenGLStream stream;
    backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);

    engine::rendering::CallbackRenderPass graphics_pass{
        "Any",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        }
    };
    EXPECT_EQ(scheduler.select_queue(graphics_pass, graphics_pass.queue()), QueueType::Graphics);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1); // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::OpenGL);

    ASSERT_EQ(submission.begin_barriers.size(), 1); // NOLINT
    EXPECT_EQ(submission.begin_barriers.front().memory_barrier_mask,
              backend::opengl::shader_image_access_barrier_bit | backend::opengl::shader_storage_barrier_bit
              | backend::opengl::atomic_counter_barrier_bit | backend::opengl::uniform_barrier_bit
              | backend::opengl::texture_fetch_barrier_bit | backend::opengl::command_barrier_bit);

    ASSERT_EQ(submission.end_barriers.size(), 1); // NOLINT
    EXPECT_EQ(submission.end_barriers.front().memory_barrier_mask,
              backend::opengl::pixel_buffer_barrier_bit | backend::opengl::buffer_update_barrier_bit
              | backend::opengl::texture_update_barrier_bit | backend::opengl::uniform_barrier_bit
              | backend::opengl::texture_fetch_barrier_bit | backend::opengl::command_barrier_bit);
}

TEST(BackendAdapters, OpenGLSchedulerNormalisesQueueSelections)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::OpenGL);
    RecordingOpenGLStream stream;
    backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);

    engine::rendering::CallbackRenderPass compute_pass{
        "ComputeStage",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        },
        QueueType::Compute
    };
    EXPECT_EQ(scheduler.select_queue(compute_pass, compute_pass.queue()), QueueType::Graphics);

    engine::rendering::CallbackRenderPass transfer_pass{
        "TransferStage",
        [](engine::rendering::FrameGraphPassBuilder&)
        {
        },
        [](engine::rendering::FrameGraphPassExecutionContext&)
        {
        },
        QueueType::Transfer
    };
    EXPECT_EQ(scheduler.select_queue(transfer_pass, transfer_pass.queue()), QueueType::Graphics);
}

TEST(BackendAdapters, OpenGLSchedulerRejectsNonOpenGLProviders)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::Vulkan);
    EXPECT_THROW({backend::opengl::OpenGLGpuScheduler scheduler(provider); }, std::invalid_argument);
}

TEST(BackendAdapters, OpenGLSchedulerPropagatesNoAccessBarriers)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::OpenGL);
    RecordingOpenGLStream stream;
    backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);

    auto command_buffer = scheduler.request_command_buffer(QueueType::Graphics, "NoAccess");

    GpuSubmitInfo info{};
    info.pass_name = "NoAccess";
    info.queue = QueueType::Graphics;
    info.command_buffer = command_buffer;

    resources::Barrier none{};
    none.source_access = resources::Access::None;
    none.destination_access = resources::Access::None;
    info.begin_barriers.push_back(none);

    scheduler.submit(info);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    const auto& submission = scheduler.submissions().front();
    ASSERT_EQ(submission.begin_barriers.size(), 1U); // NOLINT
    EXPECT_EQ(submission.begin_barriers.front().memory_barrier_mask, 0U);
    EXPECT_EQ(submission.begin_memory_barrier_mask(), 0U);
    EXPECT_EQ(submission.end_memory_barrier_mask(), 0U);
}

TEST(BackendAdapters, OpenGLSchedulerDispatchesCommandStream)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::OpenGL);
    RecordingOpenGLStream stream;
    backend::opengl::OpenGLGpuScheduler scheduler(provider, &stream);

    resources::Fence fence{"open.gl.fence"};
    resources::TimelineSemaphore wait_semaphore{"wait.timeline"};
    resources::TimelineSemaphore signal_semaphore{"signal.timeline"};

    auto command_buffer = scheduler.request_command_buffer(QueueType::Graphics, "DispatchPass");

    GpuSubmitInfo info{};
    info.pass_name = "DispatchPass";
    info.queue = QueueType::Graphics;
    info.command_buffer = command_buffer;

    resources::Barrier begin_barrier{};
    begin_barrier.source_stage = resources::PipelineStage::Compute;
    begin_barrier.destination_stage = resources::PipelineStage::Graphics;
    begin_barrier.source_access = resources::Access::Read;
    begin_barrier.destination_access = resources::Access::Write;
    const auto expected_begin_mask = backend::opengl::detail::barrier_mask(begin_barrier);
    info.begin_barriers.push_back(begin_barrier);

    resources::Barrier end_barrier{};
    end_barrier.source_stage = resources::PipelineStage::Graphics;
    end_barrier.destination_stage = resources::PipelineStage::Transfer;
    end_barrier.source_access = resources::Access::Read;
    end_barrier.destination_access = resources::Access::Write;
    const auto expected_end_mask = backend::opengl::detail::barrier_mask(end_barrier);
    info.end_barriers.push_back(end_barrier);

    info.waits.push_back(resources::SemaphoreWait{&wait_semaphore, 5});
    info.signals.push_back(resources::SemaphoreSignal{&signal_semaphore, 7});
    info.fence = &fence;
    info.fence_value = 11;

    scheduler.submit(info);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(stream.events.size(), 8U); // NOLINT
    EXPECT_EQ(stream.events[0].type, RecordingOpenGLStream::EventType::Begin);
    EXPECT_EQ(stream.events[1].type, RecordingOpenGLStream::EventType::Wait);
    EXPECT_EQ(stream.events[1].value, 5U);
    EXPECT_EQ(stream.events[2].type, RecordingOpenGLStream::EventType::Barrier);
    EXPECT_EQ(stream.events[2].mask, expected_begin_mask);
    EXPECT_EQ(stream.events[3].type, RecordingOpenGLStream::EventType::Execute);
    EXPECT_EQ(stream.events[4].type, RecordingOpenGLStream::EventType::Barrier);
    EXPECT_EQ(stream.events[4].mask, expected_end_mask);
    EXPECT_EQ(stream.events[5].type, RecordingOpenGLStream::EventType::Signal);
    EXPECT_EQ(stream.events[5].value, 7U);
    EXPECT_EQ(stream.events[6].type, RecordingOpenGLStream::EventType::Fence);
    EXPECT_EQ(stream.events[6].value, 11U);
    EXPECT_EQ(stream.events[7].type, RecordingOpenGLStream::EventType::End);

    EXPECT_EQ(stream.executed_command.queue.api, resources::GraphicsApi::OpenGL);
    EXPECT_EQ(stream.executed_command.command_buffer.queue, QueueType::Graphics);
    EXPECT_EQ(stream.fence_handle.api, resources::GraphicsApi::OpenGL);

    EXPECT_EQ(fence.value(), 11U);
    EXPECT_EQ(wait_semaphore.last_wait_value(), 5U);
    EXPECT_EQ(signal_semaphore.value(), 7U);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    EXPECT_EQ(scheduler.submissions().front().pass_name, "DispatchPass");
}