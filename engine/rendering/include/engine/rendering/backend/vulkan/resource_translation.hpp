#pragma once

#include <variant>

#include "engine/rendering/backend/vulkan/vulkan_stub.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/resources/synchronization.hpp"

namespace engine::rendering::backend::vulkan
{
    struct VulkanImageResourceDescription
    {
        VkImageCreateInfo image{};
        VkImageViewCreateInfo view{};
        VkImageSubresourceRange subresource_range{};
        VkImageLayout initial_layout{VK_IMAGE_LAYOUT_UNDEFINED};
        VkImageLayout final_layout{VK_IMAGE_LAYOUT_UNDEFINED};
    };

    struct VulkanBufferResourceDescription
    {
        VkBufferCreateInfo buffer{};
        VkBufferViewCreateInfo view{};
    };

    using VulkanResourceDescription =
        std::variant<VulkanImageResourceDescription, VulkanBufferResourceDescription>;

    VulkanResourceDescription translate_resource(const FrameGraphResourceInfo& info);

    VkPipelineStageFlags translate_pipeline_stage(resources::PipelineStage stage);
    VkAccessFlags translate_access_mask(resources::Access access);
    VkImageLayout translate_layout(ResourceState state);
    VkSampleCountFlagBits translate_sample_count(ResourceSampleCount samples);
    VkFormat translate_format(ResourceFormat format);
    VkImageUsageFlags translate_image_usage(ResourceUsage usage, ResourceDimension dimension);
    VkBufferUsageFlags translate_buffer_usage(ResourceUsage usage);
    VkImageAspectFlags translate_aspect_mask(const FrameGraphResourceInfo& info);

    struct VulkanBarrier
    {
        VkPipelineStageFlags source_stage{0};
        VkPipelineStageFlags destination_stage{0};
        VkAccessFlags source_access{0};
        VkAccessFlags destination_access{0};
    };

    VulkanBarrier translate_barrier(const resources::Barrier& barrier);
}
