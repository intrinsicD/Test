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