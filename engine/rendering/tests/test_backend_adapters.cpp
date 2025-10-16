#include <gtest/gtest.h>

#include "engine/rendering/backend/directx12/gpu_scheduler.hpp"
#include "engine/rendering/backend/metal/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"

namespace
{
    template <typename Scheduler>
    void verify_submission_translation(Scheduler& scheduler, engine::rendering::resources::RecordingGpuResourceProvider& provider,
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
        info.begin_barriers.push_back(Barrier{});
        info.end_barriers.push_back(Barrier{});
        info.waits.push_back(resources::SemaphoreWait{&wait_semaphore, 1});
        info.signals.push_back(resources::SemaphoreSignal{&signal_semaphore, 2});
        info.fence = &fence;
        info.fence_value = 3;

        scheduler.submit(info);
        scheduler.recycle(command_buffer);

        ASSERT_EQ(provider.command_buffers().count(command_buffer.index), 0U);  // NOLINT
    }
}

TEST(BackendAdapters, VulkanSchedulerTranslatesToNativeHandles)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::Vulkan);
    backend::vulkan::VulkanGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass transfer_pass{"TransferCopy",
                                                        [](engine::rendering::FrameGraphPassBuilder&) {},
                                                        [](engine::rendering::FrameGraphPassExecutionContext&) {}};
    const auto queue = scheduler.select_queue(transfer_pass, transfer_pass.queue());
    EXPECT_EQ(queue, QueueType::Transfer);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);

    ASSERT_EQ(scheduler.submissions().size(), 1);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.pass_name, "AdapterPass");
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::Vulkan);
    EXPECT_EQ(submission.command_buffer.command_buffer.api, resources::GraphicsApi::Vulkan);
    ASSERT_EQ(submission.waits.size(), 1);  // NOLINT
    EXPECT_EQ(submission.waits.front().value, 1U);
    ASSERT_EQ(submission.signals.size(), 1);  // NOLINT
    EXPECT_EQ(submission.signals.front().value, 2U);
    EXPECT_EQ(submission.fence_value, 3U);
}

TEST(BackendAdapters, DirectX12SchedulerBuildsCommandLists)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::DirectX12);
    backend::directx12::DirectX12GpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass compute_pass{"Compute",
                                                       [](engine::rendering::FrameGraphPassBuilder&) {},
                                                       [](engine::rendering::FrameGraphPassExecutionContext&) {}};
    const auto queue = scheduler.select_queue(compute_pass, compute_pass.queue());
    EXPECT_EQ(queue, QueueType::Compute);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_list.queue.api, resources::GraphicsApi::DirectX12);
}

TEST(BackendAdapters, MetalSchedulerBuildsCommandBuffers)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::Metal);
    backend::metal::MetalGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass blit_pass{"BlitResolve",
                                                    [](engine::rendering::FrameGraphPassBuilder&) {},
                                                    [](engine::rendering::FrameGraphPassExecutionContext&) {}};
    const auto queue = scheduler.select_queue(blit_pass, blit_pass.queue());
    EXPECT_EQ(queue, QueueType::Transfer);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::Metal);
}

TEST(BackendAdapters, OpenGLSchedulerRecordsGraphicsQueue)
{
    using namespace engine::rendering;

    resources::RecordingGpuResourceProvider provider(resources::GraphicsApi::OpenGL);
    backend::opengl::OpenGLGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass graphics_pass{"Any",
                                                        [](engine::rendering::FrameGraphPassBuilder&) {},
                                                        [](engine::rendering::FrameGraphPassExecutionContext&) {}};
    EXPECT_EQ(scheduler.select_queue(graphics_pass, graphics_pass.queue()), QueueType::Graphics);

    verify_submission_translation(scheduler, provider, QueueType::Graphics);
    ASSERT_EQ(scheduler.submissions().size(), 1);  // NOLINT
    const auto& submission = scheduler.submissions().front();
    EXPECT_EQ(submission.command_buffer.queue.api, resources::GraphicsApi::OpenGL);
}
