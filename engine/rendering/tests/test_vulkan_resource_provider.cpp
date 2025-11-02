#include <gtest/gtest.h>

#include "engine/rendering/backend/vulkan/resource_provider.hpp"

namespace
{
    engine::rendering::FrameGraphResourceInfo make_buffer(std::string_view name, std::uint64_t size_bytes)
    {
        engine::rendering::FrameGraphResourceInfo info{};
        info.name = name;
        info.dimension = engine::rendering::ResourceDimension::Buffer;
        info.usage = engine::rendering::ResourceUsage::ShaderRead;
        info.initial_state = engine::rendering::ResourceState::CommonRead;
        info.final_state = engine::rendering::ResourceState::CommonRead;
        info.size_bytes = size_bytes;
        return info;
    }

    engine::rendering::FrameGraphResourceInfo make_color_target(std::string_view name,
                                                                 std::uint32_t width,
                                                                 std::uint32_t height)
    {
        engine::rendering::FrameGraphResourceInfo info{};
        info.name = name;
        info.dimension = engine::rendering::ResourceDimension::Texture2D;
        info.format = engine::rendering::ResourceFormat::Rgba8Unorm;
        info.usage = engine::rendering::ResourceUsage::ColorAttachment;
        info.initial_state = engine::rendering::ResourceState::ColorAttachment;
        info.final_state = engine::rendering::ResourceState::ShaderRead;
        info.width = width;
        info.height = height;
        info.depth = 1;
        info.array_layers = 1;
        info.mip_levels = 1;
        return info;
    }
} // namespace

TEST(VulkanResourceProvider, AllocatesBuffers)
{
    using namespace engine::rendering;
    backend::vulkan::VulkanGpuResourceProvider provider{};

    FrameGraphResourceHandle handle{};
    handle.index = 3;

    provider.begin_frame();
    const auto info = make_buffer("ComputeBuffer", 4096);
    provider.on_transient_acquire(handle, info);

    const auto* record = provider.buffer(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_TRUE(record->in_use);
    EXPECT_EQ(record->size_bytes, 4096U);
    EXPECT_NE(record->handle, 0U);
    EXPECT_EQ(provider.active_buffer_bytes(), 4096U);

    provider.on_transient_release(handle, info);
    record = provider.buffer(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_FALSE(record->in_use);
}

TEST(VulkanResourceProvider, AllocatesImages)
{
    using namespace engine::rendering;
    backend::vulkan::VulkanGpuResourceProvider provider{};

    FrameGraphResourceHandle handle{};
    handle.index = 1;

    provider.begin_frame();
    const auto info = make_color_target("Color", 320, 200);
    provider.on_transient_acquire(handle, info);

    const auto* record = provider.image(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_TRUE(record->in_use);
    EXPECT_EQ(record->width, 320U);
    EXPECT_EQ(record->height, 200U);
    EXPECT_EQ(record->format, ResourceFormat::Rgba8Unorm);
    EXPECT_FALSE(record->depth_attachment);
    EXPECT_FALSE(record->multisampled);
    EXPECT_NE(record->image, 0U);
    EXPECT_NE(record->view, 0U);
    EXPECT_GT(provider.active_image_bytes(), 0U);

    provider.on_transient_release(handle, info);
    record = provider.image(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_FALSE(record->in_use);
}

TEST(VulkanResourceProvider, RecreatesResourcesWhenDescriptorChanges)
{
    using namespace engine::rendering;
    backend::vulkan::VulkanGpuResourceProvider provider{};

    FrameGraphResourceHandle handle{};
    handle.index = 5;

    provider.begin_frame();
    auto info = make_buffer("Dynamic", 1024);
    provider.on_transient_acquire(handle, info);
    const auto initial_handle = provider.buffer(handle)->handle;
    provider.on_transient_release(handle, info);

    info.size_bytes = 2048;
    provider.on_transient_acquire(handle, info);
    const auto* resized = provider.buffer(handle);
    ASSERT_NE(resized, nullptr);
    EXPECT_NE(resized->handle, 0U);
    EXPECT_NE(resized->handle, initial_handle);
    EXPECT_EQ(resized->size_bytes, 2048U);
}

TEST(VulkanResourceProvider, CollectsResourcesAfterRetentionWindow)
{
    using namespace engine::rendering;
    backend::vulkan::VulkanGpuResourceProvider provider{};
    provider.set_retention_frames(0);

    FrameGraphResourceHandle handle{};
    handle.index = 7;

    provider.begin_frame();
    const auto info = make_color_target("Transient", 128, 128);
    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);
    provider.end_frame();

    provider.begin_frame();
    provider.end_frame();

    EXPECT_EQ(provider.image(handle), nullptr);
    EXPECT_EQ(provider.active_image_bytes(), 0U);
}

TEST(VulkanResourceProvider, ProvidesCommandBufferHandles)
{
    using namespace engine::rendering;
    backend::vulkan::VulkanGpuResourceProvider provider{};

    provider.begin_frame();
    const auto queue = provider.queue_handle(QueueType::Graphics);
    EXPECT_EQ(queue.api, resources::GraphicsApi::Vulkan);
    EXPECT_EQ(queue.queue, QueueType::Graphics);

    CommandBufferHandle handle{1};
    const auto native = provider.allocate_command_buffer(QueueType::Graphics, "FrameGraph", handle);
    EXPECT_EQ(native.api, resources::GraphicsApi::Vulkan);
    EXPECT_EQ(native.queue, QueueType::Graphics);
    EXPECT_EQ(native.index, handle.index);
    EXPECT_FALSE(native.label.empty());

    const auto* record = provider.command_buffer(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->queue, QueueType::Graphics);
    EXPECT_EQ(record->native.queue, QueueType::Graphics);
}

