#include "engine/rendering/backend/vulkan/resource_translation.hpp"

#include <stdexcept>

namespace engine::rendering::backend::vulkan
{
    namespace
    {
        [[nodiscard]] VkImageType translate_image_type(ResourceDimension dimension)
        {
            switch (dimension)
            {
            case ResourceDimension::Texture1D:
                return VK_IMAGE_TYPE_1D;
            case ResourceDimension::Texture2D:
            case ResourceDimension::CubeMap:
                return VK_IMAGE_TYPE_2D;
            case ResourceDimension::Texture3D:
                return VK_IMAGE_TYPE_3D;
            case ResourceDimension::Buffer:
            case ResourceDimension::Unknown:
                break;
            }
            throw std::invalid_argument{"translate_image_type received non-texture resource"};
        }

        [[nodiscard]] VkImageViewType translate_view_type(ResourceDimension dimension)
        {
            switch (dimension)
            {
            case ResourceDimension::Texture1D:
                return VK_IMAGE_VIEW_TYPE_1D;
            case ResourceDimension::Texture2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case ResourceDimension::Texture3D:
                return VK_IMAGE_VIEW_TYPE_3D;
            case ResourceDimension::CubeMap:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            case ResourceDimension::Buffer:
            case ResourceDimension::Unknown:
                break;
            }
            throw std::invalid_argument{"translate_view_type received non-texture resource"};
        }

        [[nodiscard]] bool is_depth_format(ResourceFormat format)
        {
            switch (format)
            {
            case ResourceFormat::Depth24Stencil8:
            case ResourceFormat::Depth32f:
                return true;
            case ResourceFormat::Unknown:
            case ResourceFormat::Rgba8Unorm:
            case ResourceFormat::Rgba16f:
            case ResourceFormat::Rgba32f:
                return false;
            }
            return false;
        }

        [[nodiscard]] bool has_stencil(ResourceFormat format)
        {
            return format == ResourceFormat::Depth24Stencil8;
        }
    }

    VulkanResourceDescription translate_resource(const FrameGraphResourceInfo& info)
    {
        if (info.dimension == ResourceDimension::Buffer)
        {
            if (info.size_bytes == 0)
            {
                throw std::invalid_argument{"translate_resource requires buffer size metadata"};
            }

            VulkanBufferResourceDescription description{};
            description.buffer.size = info.size_bytes;
            description.buffer.usage = translate_buffer_usage(info.usage);
            description.buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            description.view.buffer = 0;
            description.view.range = info.size_bytes;
            description.view.format = VK_FORMAT_UNDEFINED;
            return description;
        }

        if (info.dimension == ResourceDimension::Unknown)
        {
            throw std::invalid_argument{"translate_resource requires explicit resource dimension"};
        }

        VulkanImageResourceDescription description{};
        description.image.imageType = translate_image_type(info.dimension);
        description.image.format = translate_format(info.format);
        description.image.extent = VkExtent3D{info.width, info.height, info.depth};
        description.image.mipLevels = info.mip_levels;
        description.image.arrayLayers = info.array_layers;
        description.image.samples = translate_sample_count(info.sample_count);
        description.image.usage = translate_image_usage(info.usage, info.dimension);
        description.image.initialLayout = translate_layout(info.initial_state);
        description.view.viewType = translate_view_type(info.dimension);
        description.view.format = description.image.format;
        description.subresource_range.aspectMask = translate_aspect_mask(info);
        description.subresource_range.levelCount = info.mip_levels;
        description.subresource_range.layerCount = info.array_layers;
        description.view.subresourceRange = description.subresource_range;
        description.initial_layout = translate_layout(info.initial_state);
        description.final_layout = translate_layout(info.final_state);
        return description;
    }

    VkPipelineStageFlags translate_pipeline_stage(resources::PipelineStage stage)
    {
        switch (stage)
        {
        case resources::PipelineStage::Graphics:
            return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        case resources::PipelineStage::Compute:
            return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case resources::PipelineStage::Transfer:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        throw std::invalid_argument{"translate_pipeline_stage received unknown stage"};
    }

    VkAccessFlags translate_access_mask(resources::Access access)
    {
        switch (access)
        {
        case resources::Access::None:
            return 0;
        case resources::Access::Read:
            return VK_ACCESS_MEMORY_READ_BIT;
        case resources::Access::Write:
            return VK_ACCESS_MEMORY_WRITE_BIT;
        }
        throw std::invalid_argument{"translate_access_mask received unknown access"};
    }

    VkImageLayout translate_layout(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case ResourceState::CommonRead:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::CommonWrite:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::ShaderRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ResourceState::ShaderWrite:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::ColorAttachment:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ResourceState::DepthStencilAttachment:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ResourceState::CopySource:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ResourceState::CopyDestination:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ResourceState::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        throw std::invalid_argument{"translate_layout received unknown state"};
    }

    VkSampleCountFlagBits translate_sample_count(ResourceSampleCount samples)
    {
        switch (samples)
        {
        case ResourceSampleCount::Count1:
            return VK_SAMPLE_COUNT_1_BIT;
        case ResourceSampleCount::Count2:
            return VK_SAMPLE_COUNT_2_BIT;
        case ResourceSampleCount::Count4:
            return VK_SAMPLE_COUNT_4_BIT;
        case ResourceSampleCount::Count8:
            return VK_SAMPLE_COUNT_8_BIT;
        case ResourceSampleCount::Count16:
            return VK_SAMPLE_COUNT_16_BIT;
        }
        throw std::invalid_argument{"translate_sample_count received unsupported sample count"};
    }

    VkFormat translate_format(ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::Unknown:
            return VK_FORMAT_UNDEFINED;
        case ResourceFormat::Rgba8Unorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ResourceFormat::Rgba16f:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case ResourceFormat::Rgba32f:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ResourceFormat::Depth24Stencil8:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case ResourceFormat::Depth32f:
            return VK_FORMAT_D32_SFLOAT;
        }
        throw std::invalid_argument{"translate_format received unknown format"};
    }

    VkImageUsageFlags translate_image_usage(ResourceUsage usage, ResourceDimension dimension)
    {
        VkImageUsageFlags flags = 0;
        if (has_flag(usage, ResourceUsage::TransferSource))
        {
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (has_flag(usage, ResourceUsage::TransferDestination))
        {
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (has_flag(usage, ResourceUsage::ShaderRead))
        {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (has_flag(usage, ResourceUsage::ShaderWrite))
        {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (has_flag(usage, ResourceUsage::ColorAttachment))
        {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (has_flag(usage, ResourceUsage::DepthStencilAttachment))
        {
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (has_flag(usage, ResourceUsage::Present) && dimension != ResourceDimension::Buffer)
        {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (flags == 0)
        {
            throw std::invalid_argument{"translate_image_usage requires at least one usage flag"};
        }
        return flags;
    }

    VkBufferUsageFlags translate_buffer_usage(ResourceUsage usage)
    {
        VkBufferUsageFlags flags = 0;
        if (has_flag(usage, ResourceUsage::TransferSource))
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (has_flag(usage, ResourceUsage::TransferDestination))
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (has_flag(usage, ResourceUsage::ShaderWrite))
        {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (has_flag(usage, ResourceUsage::ShaderRead))
        {
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (flags == 0)
        {
            throw std::invalid_argument{"translate_buffer_usage requires at least one usage flag"};
        }
        return flags;
    }

    VkImageAspectFlags translate_aspect_mask(const FrameGraphResourceInfo& info)
    {
        if (info.dimension == ResourceDimension::Buffer)
        {
            return 0;
        }

        VkImageAspectFlags aspect = 0;
        if (is_depth_format(info.format))
        {
            aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if (has_stencil(info.format))
            {
                aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            aspect |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        return aspect;
    }

    VulkanBarrier translate_barrier(const resources::Barrier& barrier)
    {
        VulkanBarrier translated{};
        translated.source_stage = translate_pipeline_stage(barrier.source_stage);
        translated.destination_stage = translate_pipeline_stage(barrier.destination_stage);
        translated.source_access = translate_access_mask(barrier.source_access);
        translated.destination_access = translate_access_mask(barrier.destination_access);
        return translated;
    }
}
