#include <gtest/gtest.h>

#include <string>

#include "engine/assets/handles.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/backend/vulkan/command_encoder.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/backend/vulkan/resource_provider.hpp"
#include "engine/rendering/render_pass.hpp"

using engine::rendering::backend::vulkan::VulkanCommandEncoderProvider;
using engine::rendering::backend::vulkan::VulkanGpuResourceProvider;
using engine::rendering::backend::vulkan::VulkanGpuScheduler;

TEST(VulkanGpuScheduler, SelectQueueIgnoresCaseHints)
{
    VulkanGpuResourceProvider provider{};
    VulkanGpuScheduler scheduler(provider);

    engine::rendering::CallbackRenderPass compute_pass(
        "compute_pass", [](auto&) {}, [](auto&) {});
    EXPECT_EQ(scheduler.select_queue(compute_pass, engine::rendering::QueueType::Graphics),
              engine::rendering::QueueType::Compute);

    engine::rendering::CallbackRenderPass transfer_pass(
        "async_transfer_copy", [](auto&) {}, [](auto&) {});
    EXPECT_EQ(scheduler.select_queue(transfer_pass, engine::rendering::QueueType::Graphics),
              engine::rendering::QueueType::Transfer);

    engine::rendering::CallbackRenderPass preferred_queue(
        "graphics_main", [](auto&) {}, [](auto&) {}, engine::rendering::QueueType::Compute);
    EXPECT_EQ(scheduler.select_queue(preferred_queue, engine::rendering::QueueType::Compute),
              engine::rendering::QueueType::Compute);
}

TEST(VulkanGpuScheduler, SubmissionsExposeRecordedCommandsAndSync)
{
    VulkanGpuResourceProvider provider{};
    VulkanGpuScheduler scheduler(provider);
    VulkanCommandEncoderProvider encoder_provider(provider);

    const auto command_buffer = scheduler.request_command_buffer(engine::rendering::QueueType::Graphics, "sync-pass");

    engine::rendering::CommandEncoderDescriptor descriptor{
        "sync-pass", engine::rendering::QueueType::Graphics, command_buffer};
    auto encoder = encoder_provider.begin_encoder(descriptor);

    engine::rendering::GeometryDrawCommand draw{};
    draw.geometry = engine::assets::MeshHandle{std::string{"mesh"}};
    draw.material = engine::assets::MaterialHandle{std::string{"material"}};
    draw.transform = engine::math::Transform<float>::Identity();
    encoder->draw_geometry(draw);

    engine::rendering::ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 2U;
    dispatch.group_count_y = 1U;
    dispatch.group_count_z = 3U;
    encoder->dispatch_compute(dispatch);

    encoder_provider.end_encoder(descriptor, std::move(encoder));

    engine::rendering::resources::Fence fence{"FrameFence"};
    engine::rendering::resources::TimelineSemaphore wait_sem{"WaitTimeline"};
    engine::rendering::resources::TimelineSemaphore signal_sem{"SignalTimeline"};

    engine::rendering::FrameGraphResourceHandle resource{};
    resource.index = 7U;

    engine::rendering::resources::Barrier barrier{};
    barrier.resource = resource;
    barrier.source_stage = engine::rendering::resources::PipelineStage::Transfer;
    barrier.destination_stage = engine::rendering::resources::PipelineStage::Graphics;
    barrier.source_access = engine::rendering::resources::Access::Read;
    barrier.destination_access = engine::rendering::resources::Access::Write;

    engine::rendering::GpuSubmitInfo submit{};
    submit.pass_name = "sync-pass";
    submit.queue = engine::rendering::QueueType::Graphics;
    submit.command_buffer = command_buffer;
    submit.begin_barriers.push_back(barrier);
    submit.end_barriers.push_back(barrier);
    submit.waits.push_back(engine::rendering::resources::SemaphoreWait{&wait_sem, 5U});
    submit.signals.push_back(engine::rendering::resources::SemaphoreSignal{&signal_sem, 7U});
    submit.fence = &fence;
    submit.fence_value = 9U;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U);
    const auto& submission = scheduler.submissions().front();

    EXPECT_EQ(submission.pass_name, "sync-pass");
    ASSERT_EQ(submission.commands.size(), 2U);
    EXPECT_TRUE(submission.commands.front().is_draw());
    EXPECT_TRUE(submission.commands.back().is_dispatch());

    ASSERT_EQ(submission.begin_barriers.size(), 1U);
    EXPECT_EQ(submission.begin_barriers.front().resource.index, resource.index);
    ASSERT_EQ(submission.end_barriers.size(), 1U);
    EXPECT_EQ(submission.end_barriers.front().resource.index, resource.index);

    ASSERT_EQ(submission.waits.size(), 1U);
    EXPECT_EQ(submission.waits.front().value, 5U);
    ASSERT_EQ(submission.signals.size(), 1U);
    EXPECT_EQ(submission.signals.front().value, 7U);

    EXPECT_EQ(submission.fence_value, 9U);
    EXPECT_EQ(submission.command_buffer.command_buffer.index, command_buffer.index);
    EXPECT_EQ(submission.command_buffer.queue.api, engine::rendering::resources::GraphicsApi::Vulkan);

    EXPECT_EQ(fence.value(), 9U);
    EXPECT_EQ(wait_sem.last_wait_value(), 5U);
    EXPECT_EQ(signal_sem.value(), 7U);
}
