#include "engine/rendering/backend/vulkan/resource_provider.hpp"

#include <algorithm>
#include <utility>

namespace engine::rendering::backend::vulkan
{
    namespace
    {
        [[nodiscard]] resources::QueueNativeHandle make_queue_handle(QueueType queue, std::uint64_t id) noexcept
        {
            resources::QueueNativeHandle native{};
            native.api = resources::GraphicsApi::Vulkan;
            native.queue = queue;
            native.value = id;
            return native;
        }

        [[nodiscard]] resources::CommandBufferNativeHandle make_command_buffer_handle(
            QueueType queue, std::uint64_t id, std::string_view label, CommandBufferHandle handle) noexcept
        {
            resources::CommandBufferNativeHandle native{};
            native.api = resources::GraphicsApi::Vulkan;
            native.queue = queue;
            native.value = id;
            native.index = handle.index;
            native.label = std::string{label};
            if (native.label.empty())
            {
                native.label = "VkCommandBuffer";
            }
            return native;
        }
    } // namespace

    VulkanGpuResourceProvider::VulkanGpuResourceProvider(std::uint64_t retention_frames)
        : retention_frames_{retention_frames}
    {
    }

    resources::GraphicsApi VulkanGpuResourceProvider::api() const noexcept
    {
        return resources::GraphicsApi::Vulkan;
    }

    void VulkanGpuResourceProvider::begin_frame()
    {
        ++current_frame_;
        acquired_.clear();
        released_.clear();
    }

    void VulkanGpuResourceProvider::end_frame()
    {
        const auto should_collect = [this](const auto& record) {
            if (record.in_use)
            {
                return false;
            }
            if (record.last_used_frame == 0)
            {
                return false;
            }
            if (current_frame_ <= record.last_used_frame)
            {
                return false;
            }
            return (current_frame_ - record.last_used_frame) > retention_frames_;
        };

        std::erase_if(buffers_, [&](auto& entry) {
            if (should_collect(entry.second))
            {
                destroy_buffer(entry.second);
                return true;
            }
            return false;
        });

        std::erase_if(images_, [&](auto& entry) {
            if (should_collect(entry.second))
            {
                destroy_image(entry.second);
                return true;
            }
            return false;
        });
    }

    resources::QueueNativeHandle VulkanGpuResourceProvider::queue_handle(QueueType queue) const
    {
        auto it = queues_.find(queue);
        if (it != queues_.end())
        {
            return it->second.native;
        }

        QueueRecord record{};
        const auto id = next_queue_id_.fetch_add(1, std::memory_order_relaxed);
        record.native = make_queue_handle(queue, id);
        auto [inserted, success] = queues_.emplace(queue, record);
        static_cast<void>(success);
        return inserted->second.native;
    }

    resources::CommandBufferNativeHandle VulkanGpuResourceProvider::allocate_command_buffer(
        QueueType queue, std::string_view label, CommandBufferHandle handle)
    {
        auto& record = command_buffers_[handle.index];
        record.queue = queue;
        record.label.assign(label.begin(), label.end());
        if (record.buffer == nullptr)
        {
            record.buffer = std::make_unique<VulkanCommandBuffer>();
        }
        const auto id = next_command_buffer_id_++;
        record.native = make_command_buffer_handle(queue, id, record.label, handle);
        return record.native;
    }

    void VulkanGpuResourceProvider::recycle_command_buffer(CommandBufferHandle handle)
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return;
        }
        it->second.native.value = 0;
        it->second.native.label.clear();
        if (it->second.buffer != nullptr)
        {
            it->second.buffer->clear_commands();
        }
    }

    resources::FenceNativeHandle VulkanGpuResourceProvider::resolve_fence(const resources::Fence& fence)
    {
        const auto name = std::string{fence.name()};
        auto& native = fences_[name];
        if (native.api == resources::GraphicsApi::Unknown)
        {
            native.api = resources::GraphicsApi::Vulkan;
            native.value = next_fence_id_++;
        }
        return native;
    }

    resources::TimelineSemaphoreNativeHandle
    VulkanGpuResourceProvider::resolve_semaphore(const resources::TimelineSemaphore& semaphore)
    {
        const auto name = std::string{semaphore.name()};
        auto& native = timelines_[name];
        if (native.api == resources::GraphicsApi::Unknown)
        {
            native.api = resources::GraphicsApi::Vulkan;
            native.value = next_timeline_id_++;
        }
        return native;
    }

    void VulkanGpuResourceProvider::on_transient_acquire(FrameGraphResourceHandle handle,
                                                          const FrameGraphResourceInfo& info)
    {
        if (!handle.valid())
        {
            return;
        }

        acquired_.push_back(ResourceEventRecord{handle, info});

        if (is_buffer_resource(info))
        {
            auto& record = buffers_[handle.index];
            if (record.handle == 0 || !buffer_descriptor_matches(record, info))
            {
                if (record.handle != 0)
                {
                    destroy_buffer(record);
                }
                allocate_buffer(handle.index, info);
            }
            auto& buffer_record = buffers_[handle.index];
            buffer_record.in_use = true;
            buffer_record.last_used_frame = current_frame_;
            return;
        }

        if (is_image_resource(info))
        {
            auto& record = images_[handle.index];
            if (record.image == 0 || !image_descriptor_matches(record, info))
            {
                if (record.image != 0)
                {
                    destroy_image(record);
                }
                allocate_image(handle.index, info);
            }
            auto& image_record = images_[handle.index];
            image_record.in_use = true;
            image_record.last_used_frame = current_frame_;
        }
    }

    void VulkanGpuResourceProvider::on_transient_release(FrameGraphResourceHandle handle,
                                                         const FrameGraphResourceInfo& info)
    {
        if (!handle.valid())
        {
            return;
        }

        released_.push_back(ResourceEventRecord{handle, info});

        if (is_buffer_resource(info))
        {
            auto it = buffers_.find(handle.index);
            if (it != buffers_.end())
            {
                it->second.in_use = false;
                it->second.last_used_frame = current_frame_;
            }
            return;
        }

        if (is_image_resource(info))
        {
            auto it = images_.find(handle.index);
            if (it != images_.end())
            {
                it->second.in_use = false;
                it->second.last_used_frame = current_frame_;
            }
        }
    }

    const std::vector<VulkanGpuResourceProvider::ResourceEventRecord>& VulkanGpuResourceProvider::acquired() const noexcept
    {
        return acquired_;
    }

    const std::vector<VulkanGpuResourceProvider::ResourceEventRecord>& VulkanGpuResourceProvider::released() const noexcept
    {
        return released_;
    }

    const VulkanGpuResourceProvider::BufferRecord*
    VulkanGpuResourceProvider::buffer(FrameGraphResourceHandle handle) const noexcept
    {
        const auto it = buffers_.find(handle.index);
        if (it != buffers_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::uint64_t VulkanGpuResourceProvider::active_buffer_bytes() const noexcept
    {
        return active_buffer_bytes_;
    }

    const VulkanGpuResourceProvider::ImageRecord*
    VulkanGpuResourceProvider::image(FrameGraphResourceHandle handle) const noexcept
    {
        const auto it = images_.find(handle.index);
        if (it != images_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::uint64_t VulkanGpuResourceProvider::active_image_bytes() const noexcept
    {
        return active_image_bytes_;
    }

    std::uint64_t VulkanGpuResourceProvider::active_memory_bytes() const noexcept
    {
        return active_buffer_bytes_ + active_image_bytes_;
    }

    VulkanCommandBuffer* VulkanGpuResourceProvider::command_buffer(CommandBufferHandle handle) noexcept
    {
        auto it = command_buffers_.find(handle.index);
        if (it != command_buffers_.end())
        {
            return it->second.buffer.get();
        }
        return nullptr;
    }

    const VulkanCommandBuffer*
    VulkanGpuResourceProvider::command_buffer(CommandBufferHandle handle) const noexcept
    {
        const auto it = command_buffers_.find(handle.index);
        if (it != command_buffers_.end())
        {
            return it->second.buffer.get();
        }
        return nullptr;
    }

    void VulkanGpuResourceProvider::set_retention_frames(std::uint64_t frames) noexcept
    {
        retention_frames_ = frames;
    }

    std::uint64_t VulkanGpuResourceProvider::retention_frames() const noexcept
    {
        return retention_frames_;
    }

    bool VulkanGpuResourceProvider::is_buffer_resource(const FrameGraphResourceInfo& info) noexcept
    {
        return info.dimension == ResourceDimension::Buffer;
    }

    bool VulkanGpuResourceProvider::is_image_resource(const FrameGraphResourceInfo& info) noexcept
    {
        return info.dimension == ResourceDimension::Texture1D
            || info.dimension == ResourceDimension::Texture2D
            || info.dimension == ResourceDimension::Texture3D
            || info.dimension == ResourceDimension::CubeMap;
    }

    bool VulkanGpuResourceProvider::is_depth_format(ResourceFormat format) noexcept
    {
        return format == ResourceFormat::Depth24Stencil8 || format == ResourceFormat::Depth32f;
    }

    std::uint32_t VulkanGpuResourceProvider::sample_count_value(ResourceSampleCount count) noexcept
    {
        switch (count)
        {
        case ResourceSampleCount::Count1:
            return 1U;
        case ResourceSampleCount::Count2:
            return 2U;
        case ResourceSampleCount::Count4:
            return 4U;
        case ResourceSampleCount::Count8:
            return 8U;
        case ResourceSampleCount::Count16:
            return 16U;
        }
        return 1U;
    }

    std::uint32_t VulkanGpuResourceProvider::bytes_per_pixel(ResourceFormat format) noexcept
    {
        switch (format)
        {
        case ResourceFormat::Rgba8Unorm:
            return 4U;
        case ResourceFormat::Rgba16f:
            return 8U;
        case ResourceFormat::Rgba32f:
            return 16U;
        case ResourceFormat::Depth24Stencil8:
            return 4U;
        case ResourceFormat::Depth32f:
            return 4U;
        case ResourceFormat::Unknown:
        default:
            return 0U;
        }
    }

    std::uint64_t VulkanGpuResourceProvider::estimate_image_size(const FrameGraphResourceInfo& info) noexcept
    {
        const std::uint64_t bytes_from_descriptor = info.size_bytes;
        const auto samples = static_cast<std::uint64_t>(sample_count_value(info.sample_count));
        const auto pixel_count = static_cast<std::uint64_t>(info.width) * static_cast<std::uint64_t>(info.height)
            * static_cast<std::uint64_t>(info.depth) * static_cast<std::uint64_t>(info.array_layers)
            * static_cast<std::uint64_t>(info.mip_levels == 0 ? 1 : info.mip_levels);
        const auto bytes_per_texel = static_cast<std::uint64_t>(bytes_per_pixel(info.format));
        const std::uint64_t computed = pixel_count * bytes_per_texel * samples;
        if (bytes_from_descriptor != 0)
        {
            return bytes_from_descriptor;
        }
        return computed;
    }

    void VulkanGpuResourceProvider::allocate_buffer(std::size_t index, const FrameGraphResourceInfo& info)
    {
        BufferRecord record{};
        record.name.assign(info.name.begin(), info.name.end());
        record.usage = info.usage;
        record.size_bytes = info.size_bytes;
        record.handle = next_buffer_id_++;
        record.in_use = true;
        record.last_used_frame = current_frame_;

        active_buffer_bytes_ += record.size_bytes;
        buffers_[index] = std::move(record);
    }

    void VulkanGpuResourceProvider::allocate_image(std::size_t index, const FrameGraphResourceInfo& info)
    {
        ImageRecord record{};
        record.name.assign(info.name.begin(), info.name.end());
        record.format = info.format;
        record.usage = info.usage;
        record.dimension = info.dimension;
        record.sample_count = info.sample_count;
        record.width = info.width;
        record.height = info.height;
        record.depth = info.depth;
        record.array_layers = info.array_layers;
        record.mip_levels = info.mip_levels;
        record.image = next_image_id_++;
        record.view = next_view_id_++;
        record.depth_attachment = is_depth_format(info.format)
            || has_flag(info.usage, ResourceUsage::DepthStencilAttachment);
        record.multisampled = info.sample_count != ResourceSampleCount::Count1;
        record.in_use = true;
        record.last_used_frame = current_frame_;
        record.byte_size = estimate_image_size(info);

        active_image_bytes_ += record.byte_size;
        images_[index] = std::move(record);
    }

    void VulkanGpuResourceProvider::destroy_buffer(BufferRecord& record) noexcept
    {
        if (record.size_bytes <= active_buffer_bytes_)
        {
            active_buffer_bytes_ -= record.size_bytes;
        }
        else
        {
            active_buffer_bytes_ = 0;
        }
        record = BufferRecord{};
    }

    void VulkanGpuResourceProvider::destroy_image(ImageRecord& record) noexcept
    {
        if (record.byte_size <= active_image_bytes_)
        {
            active_image_bytes_ -= record.byte_size;
        }
        else
        {
            active_image_bytes_ = 0;
        }
        record = ImageRecord{};
    }

    bool VulkanGpuResourceProvider::buffer_descriptor_matches(const BufferRecord& record,
                                                               const FrameGraphResourceInfo& info) const noexcept
    {
        return record.size_bytes == info.size_bytes && record.usage == info.usage
            && record.name == std::string(info.name);
    }

    bool VulkanGpuResourceProvider::image_descriptor_matches(const ImageRecord& record,
                                                              const FrameGraphResourceInfo& info) const noexcept
    {
        const bool matches_dimensions = record.width == info.width && record.height == info.height
            && record.depth == info.depth && record.array_layers == info.array_layers
            && record.mip_levels == info.mip_levels;
        const bool matches_usage = record.usage == info.usage && record.dimension == info.dimension
            && record.sample_count == info.sample_count && record.format == info.format;
        return matches_dimensions && matches_usage;
    }
}

