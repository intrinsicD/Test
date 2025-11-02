#include <cstdint>
#include <string_view>

#include <gtest/gtest.h>

#include "engine/rendering/backend/opengl/resource_provider.hpp"
#include "engine/rendering/frame_graph.hpp"

namespace
{
    engine::rendering::FrameGraphResourceInfo make_buffer_resource(std::string_view name,
                                                                   std::uint64_t size_bytes,
                                                                   engine::rendering::ResourceUsage usage)
    {
        engine::rendering::FrameGraphResourceInfo info{};
        info.name = name;
        info.dimension = engine::rendering::ResourceDimension::Buffer;
        info.usage = usage;
        info.initial_state = engine::rendering::ResourceState::CommonRead;
        info.final_state = engine::rendering::ResourceState::CommonRead;
        info.size_bytes = size_bytes;
        return info;
    }

    engine::rendering::FrameGraphResourceInfo make_color_resource(std::string_view name,
                                                                  std::uint32_t width,
                                                                  std::uint32_t height)
    {
        engine::rendering::FrameGraphResourceInfo info{};
        info.name = name;
        info.format = engine::rendering::ResourceFormat::Rgba8Unorm;
        info.dimension = engine::rendering::ResourceDimension::Texture2D;
        info.usage = engine::rendering::ResourceUsage::ColorAttachment;
        info.initial_state = engine::rendering::ResourceState::ColorAttachment;
        info.final_state = engine::rendering::ResourceState::ShaderRead;
        info.width = width;
        info.height = height;
        info.depth = 1;
        info.array_layers = 1;
        info.mip_levels = 1;
        info.sample_count = engine::rendering::ResourceSampleCount::Count1;
        return info;
    }
}

TEST(OpenGLResourceProvider, AllocatesTextureOnAcquire)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 0;

    provider.begin_frame();

    const auto info = make_color_resource("Color", 640, 480);

    provider.on_transient_acquire(handle, info);

    const auto* record = provider.texture(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->width, 640U);
    EXPECT_EQ(record->height, 480U);
    EXPECT_TRUE(record->in_use);
    EXPECT_NE(record->handle, 0U);
    EXPECT_FALSE(record->depth_attachment);

    provider.on_transient_release(handle, info);

    record = provider.texture(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_FALSE(record->in_use);
}

TEST(OpenGLResourceProvider, AllocatesBufferOnAcquire)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 10;

    provider.begin_frame();

    const auto info = make_buffer_resource("Compute", 4096,
                                           engine::rendering::ResourceUsage::ShaderRead
                                           | engine::rendering::ResourceUsage::ShaderWrite);

    provider.on_transient_acquire(handle, info);

    const auto* record = provider.buffer(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->size_bytes, 4096U);
    EXPECT_TRUE(record->in_use);
    EXPECT_NE(record->handle, 0U);

    provider.on_transient_release(handle, info);

    record = provider.buffer(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_FALSE(record->in_use);
}

TEST(OpenGLResourceProvider, ReallocatesBufferWhenDescriptorChanges)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 11;

    provider.begin_frame();

    auto info = make_buffer_resource("ComputeResize", 2048,
                                     engine::rendering::ResourceUsage::ShaderRead);
    provider.on_transient_acquire(handle, info);
    const auto* initial = provider.buffer(handle);
    ASSERT_NE(initial, nullptr);
    const auto first_handle = initial->handle;

    provider.on_transient_release(handle, info);

    info.size_bytes = 8192;
    provider.on_transient_acquire(handle, info);

    const auto* resized = provider.buffer(handle);
    ASSERT_NE(resized, nullptr);
    EXPECT_EQ(resized->size_bytes, 8192U);
    EXPECT_NE(resized->handle, 0U);
    EXPECT_NE(resized->handle, first_handle);
}

TEST(OpenGLResourceProvider, ReallocatesTextureWhenDescriptorChanges)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 1;

    provider.begin_frame();

    auto info = make_color_resource("Color.Resize", 320, 200);
    provider.on_transient_acquire(handle, info);
    const auto* initial = provider.texture(handle);
    ASSERT_NE(initial, nullptr);
    const auto first_handle = initial->handle;

    provider.on_transient_release(handle, info);

    info.width = 800;
    info.height = 600;
    provider.on_transient_acquire(handle, info);

    const auto* resized = provider.texture(handle);
    ASSERT_NE(resized, nullptr);
    EXPECT_EQ(resized->width, 800U);
    EXPECT_EQ(resized->height, 600U);
    EXPECT_NE(resized->handle, 0U);
    EXPECT_NE(resized->handle, first_handle);
}

TEST(OpenGLResourceProvider, TracksDepthAttachmentDescriptors)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 2;

    provider.begin_frame();

    engine::rendering::FrameGraphResourceInfo info{};
    info.name = "Depth";
    info.format = engine::rendering::ResourceFormat::Depth24Stencil8;
    info.dimension = engine::rendering::ResourceDimension::Texture2D;
    info.usage = engine::rendering::ResourceUsage::DepthStencilAttachment;
    info.initial_state = engine::rendering::ResourceState::DepthStencilAttachment;
    info.final_state = engine::rendering::ResourceState::DepthStencilAttachment;
    info.width = 256;
    info.height = 256;
    info.depth = 1;
    info.array_layers = 1;
    info.mip_levels = 1;
    info.sample_count = engine::rendering::ResourceSampleCount::Count1;

    provider.on_transient_acquire(handle, info);

    const auto* record = provider.texture(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_TRUE(record->depth_attachment);
    EXPECT_TRUE(record->in_use);

    provider.on_transient_release(handle, info);
    record = provider.texture(handle);
    ASSERT_NE(record, nullptr);
    EXPECT_FALSE(record->in_use);
}

TEST(OpenGLResourceProvider, RecordsResourceEvents)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 42;

    provider.begin_frame();

    const auto info = make_color_resource("Telemetry.Color", 128, 64);

    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);

    const auto& acquired = provider.acquired();
    ASSERT_EQ(acquired.size(), 1U);
    EXPECT_EQ(acquired.front().handle, handle);
    EXPECT_EQ(acquired.front().info.name, info.name);
    EXPECT_EQ(acquired.front().info.width, info.width);
    EXPECT_EQ(acquired.front().info.height, info.height);
    EXPECT_EQ(acquired.front().info.final_state, info.final_state);

    const auto& released = provider.released();
    ASSERT_EQ(released.size(), 1U);
    EXPECT_EQ(released.front().handle, handle);
    EXPECT_EQ(released.front().info.name, info.name);
    EXPECT_EQ(released.front().info.usage, info.usage);
}

TEST(OpenGLResourceProvider, ReusesResourcesAcrossFramesWithinRetention)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 7;

    auto info = make_color_resource("Color.Persistent", 512, 512);

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    const auto* initial = provider.texture(handle);
    ASSERT_NE(initial, nullptr);
    const auto first_handle = initial->handle;
    provider.on_transient_release(handle, info);
    provider.end_frame();

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    const auto* reused = provider.texture(handle);
    ASSERT_NE(reused, nullptr);
    EXPECT_EQ(reused->handle, first_handle);
}

TEST(OpenGLResourceProvider, CollectsTexturesUnusedForMultipleFrames)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 8;

    auto info = make_color_resource("Color.Evict", 128, 128);

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);
    provider.end_frame();

    provider.begin_frame();
    provider.end_frame();

    EXPECT_EQ(provider.texture(handle), nullptr);
}

TEST(OpenGLResourceProvider, CollectsBuffersUnusedForMultipleFrames)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider;
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 9;

    auto info = make_buffer_resource("Buffer.Evict", 1024, engine::rendering::ResourceUsage::ShaderRead);

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);
    provider.end_frame();

    provider.begin_frame();
    provider.end_frame();

    EXPECT_EQ(provider.buffer(handle), nullptr);
}

TEST(OpenGLResourceProvider, HonorsCustomRetentionWindow)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider(2);
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 12;

    const auto info = make_color_resource("Color.Retention", 64, 64);

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);
    provider.end_frame();

    for (int frame = 0; frame < 2; ++frame)
    {
        provider.begin_frame();
        const auto* record = provider.texture(handle);
        ASSERT_NE(record, nullptr);
        EXPECT_FALSE(record->in_use);
        provider.end_frame();
    }

    provider.begin_frame();
    provider.end_frame();

    EXPECT_EQ(provider.texture(handle), nullptr);
}

TEST(OpenGLResourceProvider, UpdatesRetentionWindowAtRuntime)
{
    engine::rendering::backend::opengl::OpenGLGpuResourceProvider provider(0);
    engine::rendering::FrameGraphResourceHandle handle{};
    handle.index = 13;

    const auto info = make_color_resource("Color.DynamicRetention", 32, 32);

    provider.begin_frame();
    provider.on_transient_acquire(handle, info);
    provider.on_transient_release(handle, info);
    provider.end_frame();

    provider.set_retention_frames(1);

    provider.begin_frame();
    const auto* record = provider.texture(handle);
    ASSERT_NE(record, nullptr);
    provider.end_frame();

    EXPECT_NE(provider.texture(handle), nullptr);

    provider.begin_frame();
    provider.end_frame();

    EXPECT_EQ(provider.texture(handle), nullptr);
}
