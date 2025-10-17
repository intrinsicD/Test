#include <gtest/gtest.h>

#include <variant>

#include "engine/rendering/backend/vulkan/resource_translation.hpp"

namespace
{
    engine::rendering::FrameGraphResourceInfo make_color_texture_info()
    {
        engine::rendering::FrameGraphResourceInfo info{};
        info.name = "Color";
        info.lifetime = engine::rendering::ResourceLifetime::Transient;
        info.format = engine::rendering::ResourceFormat::Rgba16f;
        info.dimension = engine::rendering::ResourceDimension::Texture2D;
        info.usage = engine::rendering::ResourceUsage::ColorAttachment |
                     engine::rendering::ResourceUsage::ShaderRead |
                     engine::rendering::ResourceUsage::TransferSource;
        info.initial_state = engine::rendering::ResourceState::ColorAttachment;
        info.final_state = engine::rendering::ResourceState::ShaderRead;
        info.width = 1024;
        info.height = 768;
        info.depth = 1;
        info.array_layers = 1;
        info.mip_levels = 1;
        info.sample_count = engine::rendering::ResourceSampleCount::Count1;
        return info;
    }
}

TEST(VulkanTranslation, TranslatesColorTextureDescriptor)
{
    const auto info = make_color_texture_info();
    const auto description = engine::rendering::backend::vulkan::translate_resource(info);
    ASSERT_TRUE(std::holds_alternative<engine::rendering::backend::vulkan::VulkanImageResourceDescription>(
        description));  // NOLINT
    const auto& image = std::get<engine::rendering::backend::vulkan::VulkanImageResourceDescription>(description);
    EXPECT_EQ(image.image.imageType, VK_IMAGE_TYPE_2D);
    EXPECT_EQ(image.image.format, VK_FORMAT_R16G16B16A16_SFLOAT);
    EXPECT_EQ(image.image.extent.width, 1024U);
    EXPECT_EQ(image.image.extent.height, 768U);
    EXPECT_EQ(image.image.mipLevels, 1U);
    EXPECT_EQ(image.image.samples, VK_SAMPLE_COUNT_1_BIT);
    EXPECT_EQ(image.image.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    EXPECT_EQ(image.image.initialLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(image.initial_layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(image.final_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(image.subresource_range.aspectMask, VK_IMAGE_ASPECT_COLOR_BIT);
}

TEST(VulkanTranslation, TranslatesDepthTextureDescriptor)
{
    engine::rendering::FrameGraphResourceInfo info = make_color_texture_info();
    info.name = "Depth";
    info.format = engine::rendering::ResourceFormat::Depth24Stencil8;
    info.usage = engine::rendering::ResourceUsage::DepthStencilAttachment;
    info.initial_state = engine::rendering::ResourceState::DepthStencilAttachment;
    info.final_state = engine::rendering::ResourceState::DepthStencilAttachment;

    const auto description = engine::rendering::backend::vulkan::translate_resource(info);
    const auto& image = std::get<engine::rendering::backend::vulkan::VulkanImageResourceDescription>(description);
    EXPECT_EQ(image.image.format, VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_EQ(image.subresource_range.aspectMask,
              VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    EXPECT_EQ(image.initial_layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(image.final_layout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

TEST(VulkanTranslation, TranslatesBufferDescriptor)
{
    engine::rendering::FrameGraphResourceInfo info{};
    info.name = "Buffer";
    info.dimension = engine::rendering::ResourceDimension::Buffer;
    info.size_bytes = 4096;
    info.usage = engine::rendering::ResourceUsage::TransferDestination |
                 engine::rendering::ResourceUsage::ShaderRead |
                 engine::rendering::ResourceUsage::ShaderWrite;

    const auto description = engine::rendering::backend::vulkan::translate_resource(info);
    ASSERT_TRUE(std::holds_alternative<engine::rendering::backend::vulkan::VulkanBufferResourceDescription>(
        description));  // NOLINT
    const auto& buffer = std::get<engine::rendering::backend::vulkan::VulkanBufferResourceDescription>(description);
    EXPECT_EQ(buffer.buffer.size, 4096U);
    EXPECT_TRUE((buffer.buffer.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0U);
    EXPECT_TRUE((buffer.buffer.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0U);
}

TEST(VulkanTranslation, TranslatesBarriers)
{
    engine::rendering::resources::Barrier barrier{};
    barrier.source_stage = engine::rendering::resources::PipelineStage::Graphics;
    barrier.destination_stage = engine::rendering::resources::PipelineStage::Compute;
    barrier.source_access = engine::rendering::resources::Access::Read;
    barrier.destination_access = engine::rendering::resources::Access::Write;

    const auto translated = engine::rendering::backend::vulkan::translate_barrier(barrier);
    EXPECT_EQ(translated.source_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    EXPECT_EQ(translated.destination_stage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    EXPECT_EQ(translated.source_access, VK_ACCESS_MEMORY_READ_BIT);
    EXPECT_EQ(translated.destination_access, VK_ACCESS_MEMORY_WRITE_BIT);
}
