#include <gtest/gtest.h>

#include <string>

#include "engine/assets/handles.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/backend/vulkan/command_encoder.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/backend/vulkan/resource_provider.hpp"

using engine::rendering::backend::vulkan::VulkanCommandEncoderProvider;
using engine::rendering::backend::vulkan::VulkanGpuResourceProvider;
using engine::rendering::backend::vulkan::VulkanGpuScheduler;

TEST(VulkanCommandEncoder, RecordsDrawCommands)
{
    VulkanGpuResourceProvider provider;
    VulkanGpuScheduler scheduler(provider);
    VulkanCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(engine::rendering::QueueType::Graphics, "EncodePass");

    engine::rendering::CommandEncoderDescriptor descriptor{
        "EncodePass", engine::rendering::QueueType::Graphics, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    engine::rendering::GeometryDrawCommand draw{};
    const auto mesh = engine::assets::MeshHandle{std::string{"mesh.handle"}};
    draw.geometry = mesh;
    draw.material = engine::assets::MaterialHandle{std::string{"material.handle"}};
    draw.transform = engine::math::Transform<float>::Identity();
    encoder->draw_geometry(draw);

    encoders.end_encoder(descriptor, std::move(encoder));

    engine::rendering::GpuSubmitInfo submit{};
    submit.pass_name = "EncodePass";
    submit.queue = engine::rendering::QueueType::Graphics;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    const auto& submission = scheduler.submissions().front();
    ASSERT_EQ(submission.commands.size(), 1U); // NOLINT
    const auto& recorded = submission.commands.front();
    ASSERT_EQ(recorded.type(), engine::rendering::EncodedCommand::Type::DrawGeometry);
    EXPECT_EQ(recorded.geometry_draw().material.id(), std::string{"material.handle"});
    EXPECT_EQ(recorded.geometry_draw().geometry.index(), draw.geometry.index());
}

TEST(VulkanCommandEncoder, RecordsComputeDispatches)
{
    VulkanGpuResourceProvider provider;
    VulkanGpuScheduler scheduler(provider);
    VulkanCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(engine::rendering::QueueType::Compute, "DispatchPass");

    engine::rendering::CommandEncoderDescriptor descriptor{
        "DispatchPass", engine::rendering::QueueType::Compute, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    engine::rendering::ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 2U;
    dispatch.group_count_y = 3U;
    dispatch.group_count_z = 4U;
    encoder->dispatch_compute(dispatch);

    encoders.end_encoder(descriptor, std::move(encoder));

    engine::rendering::GpuSubmitInfo submit{};
    submit.pass_name = "DispatchPass";
    submit.queue = engine::rendering::QueueType::Compute;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    const auto& submission = scheduler.submissions().front();
    ASSERT_EQ(submission.commands.size(), 1U); // NOLINT
    const auto& recorded = submission.commands.front();
    ASSERT_EQ(recorded.type(), engine::rendering::EncodedCommand::Type::DispatchCompute);
    EXPECT_EQ(recorded.compute_dispatch().group_count_x, 2U);
    EXPECT_EQ(recorded.compute_dispatch().group_count_y, 3U);
    EXPECT_EQ(recorded.compute_dispatch().group_count_z, 4U);
}

