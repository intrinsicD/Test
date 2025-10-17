#pragma once

#ifndef VULKAN_CORE_H_
#define VULKAN_CORE_H_

#include <cstdint>

using VkFlags = std::uint32_t;
using VkBool32 = std::uint32_t;
using VkDeviceSize = std::uint64_t;

enum VkStructureType
{
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15,
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
    VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO = 17,
};

enum VkImageType
{
    VK_IMAGE_TYPE_1D = 0,
    VK_IMAGE_TYPE_2D = 1,
    VK_IMAGE_TYPE_3D = 2,
};

enum VkFormat
{
    VK_FORMAT_UNDEFINED = 0,
    VK_FORMAT_R8G8B8A8_UNORM = 37,
    VK_FORMAT_R16G16B16A16_SFLOAT = 97,
    VK_FORMAT_R32G32B32A32_SFLOAT = 109,
    VK_FORMAT_D24_UNORM_S8_UINT = 129,
    VK_FORMAT_D32_SFLOAT = 126,
};

enum VkSampleCountFlagBits
{
    VK_SAMPLE_COUNT_1_BIT = 0x00000001,
    VK_SAMPLE_COUNT_2_BIT = 0x00000002,
    VK_SAMPLE_COUNT_4_BIT = 0x00000004,
    VK_SAMPLE_COUNT_8_BIT = 0x00000008,
    VK_SAMPLE_COUNT_16_BIT = 0x00000010,
};
using VkSampleCountFlags = VkFlags;

enum VkImageUsageFlagBits
{
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
};
using VkImageUsageFlags = VkFlags;

enum VkBufferUsageFlagBits
{
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
};
using VkBufferUsageFlags = VkFlags;

enum VkImageTiling
{
    VK_IMAGE_TILING_OPTIMAL = 0,
    VK_IMAGE_TILING_LINEAR = 1,
};

enum VkSharingMode
{
    VK_SHARING_MODE_EXCLUSIVE = 0,
    VK_SHARING_MODE_CONCURRENT = 1,
};

enum VkImageLayout
{
    VK_IMAGE_LAYOUT_UNDEFINED = 0,
    VK_IMAGE_LAYOUT_GENERAL = 1,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    VK_IMAGE_LAYOUT_PREINITIALIZED = 8,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
};

enum VkImageAspectFlagBits
{
    VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
    VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
    VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
};
using VkImageAspectFlags = VkFlags;

enum VkImageViewType
{
    VK_IMAGE_VIEW_TYPE_1D = 0,
    VK_IMAGE_VIEW_TYPE_2D = 1,
    VK_IMAGE_VIEW_TYPE_3D = 2,
    VK_IMAGE_VIEW_TYPE_CUBE = 3,
};

enum VkComponentSwizzle
{
    VK_COMPONENT_SWIZZLE_IDENTITY = 0,
};

struct VkComponentMapping
{
    VkComponentSwizzle r{VK_COMPONENT_SWIZZLE_IDENTITY};
    VkComponentSwizzle g{VK_COMPONENT_SWIZZLE_IDENTITY};
    VkComponentSwizzle b{VK_COMPONENT_SWIZZLE_IDENTITY};
    VkComponentSwizzle a{VK_COMPONENT_SWIZZLE_IDENTITY};
};

struct VkExtent3D
{
    std::uint32_t width{1};
    std::uint32_t height{1};
    std::uint32_t depth{1};
};

struct VkImageSubresourceRange
{
    VkImageAspectFlags aspectMask{0};
    std::uint32_t baseMipLevel{0};
    std::uint32_t levelCount{1};
    std::uint32_t baseArrayLayer{0};
    std::uint32_t layerCount{1};
};

struct VkImageCreateInfo
{
    VkStructureType sType{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    const void* pNext{nullptr};
    VkFlags flags{0};
    VkImageType imageType{VK_IMAGE_TYPE_2D};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkExtent3D extent{};
    std::uint32_t mipLevels{1};
    std::uint32_t arrayLayers{1};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
    VkImageTiling tiling{VK_IMAGE_TILING_OPTIMAL};
    VkImageUsageFlags usage{0};
    VkSharingMode sharingMode{VK_SHARING_MODE_EXCLUSIVE};
    std::uint32_t queueFamilyIndexCount{0};
    const std::uint32_t* pQueueFamilyIndices{nullptr};
    VkImageLayout initialLayout{VK_IMAGE_LAYOUT_UNDEFINED};
};

struct VkImageViewCreateInfo
{
    VkStructureType sType{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    const void* pNext{nullptr};
    VkFlags flags{0};
    std::uint64_t image{0};
    VkImageViewType viewType{VK_IMAGE_VIEW_TYPE_2D};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkComponentMapping components{};
    VkImageSubresourceRange subresourceRange{};
};

struct VkBufferCreateInfo
{
    VkStructureType sType{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    const void* pNext{nullptr};
    VkFlags flags{0};
    VkDeviceSize size{0};
    VkBufferUsageFlags usage{0};
    VkSharingMode sharingMode{VK_SHARING_MODE_EXCLUSIVE};
    std::uint32_t queueFamilyIndexCount{0};
    const std::uint32_t* pQueueFamilyIndices{nullptr};
};

struct VkBufferViewCreateInfo
{
    VkStructureType sType{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    const void* pNext{nullptr};
    VkFlags flags{0};
    std::uint64_t buffer{0};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkDeviceSize offset{0};
    VkDeviceSize range{0};
};

enum VkPipelineStageFlagBits
{
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
    VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
};
using VkPipelineStageFlags = VkFlags;

enum VkAccessFlagBits
{
    VK_ACCESS_MEMORY_READ_BIT = 0x00000020,
    VK_ACCESS_MEMORY_WRITE_BIT = 0x00000040,
};
using VkAccessFlags = VkFlags;

#endif // VULKAN_CORE_H_
