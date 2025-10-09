#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"

#include <algorithm>
#include <utility>

#include "engine/rendering/frame_graph.hpp"

namespace engine::rendering::resources
{
    namespace
    {
        std::uintptr_t pointer_to_value(const void* pointer) noexcept
        {
            return reinterpret_cast<std::uintptr_t>(pointer);
        }
    }

    RecordingGpuResourceProvider::RecordingGpuResourceProvider(GraphicsApi api) : api_(api)
    {
    }

    GraphicsApi RecordingGpuResourceProvider::api() const noexcept
    {
        return api_;
    }

    void RecordingGpuResourceProvider::begin_frame()
    {
        ++frames_begun_;
        acquired_.clear();
        released_.clear();
    }

    void RecordingGpuResourceProvider::end_frame()
    {
        ++frames_completed_;
    }

    QueueNativeHandle RecordingGpuResourceProvider::queue_handle(QueueType queue) const
    {
        if (const auto it = queues_.find(queue); it != queues_.end())
        {
            return it->second;
        }

        QueueNativeHandle handle{};
        handle.api = api_;
        handle.queue = queue;
        handle.value = next_queue_value_++;
        queues_.insert_or_assign(queue, handle);
        return handle;
    }

    CommandBufferNativeHandle RecordingGpuResourceProvider::allocate_command_buffer(
        QueueType queue, std::string_view label, CommandBufferHandle handle)
    {
        CommandBufferNativeHandle native{};
        native.api = api_;
        native.queue = queue;
        native.value = next_command_buffer_value_++;
        native.index = handle.index;
        native.label = std::string{label};
        command_buffers_.insert_or_assign(handle.index, native);
        return native;
    }

    void RecordingGpuResourceProvider::recycle_command_buffer(CommandBufferHandle handle)
    {
        command_buffers_.erase(handle.index);
    }

    FenceNativeHandle RecordingGpuResourceProvider::resolve_fence(const Fence& fence)
    {
        FenceNativeHandle native{};
        native.api = api_;
        native.value = pointer_to_value(&fence);
        return native;
    }

    TimelineSemaphoreNativeHandle RecordingGpuResourceProvider::resolve_semaphore(
        const TimelineSemaphore& semaphore)
    {
        TimelineSemaphoreNativeHandle native{};
        native.api = api_;
        native.value = pointer_to_value(&semaphore);
        return native;
    }

    void RecordingGpuResourceProvider::on_transient_acquire(FrameGraphResourceHandle handle,
                                                            const FrameGraphResourceInfo& info)
    {
        acquired_.push_back(ResourceEventRecord{handle, info});
    }

    void RecordingGpuResourceProvider::on_transient_release(FrameGraphResourceHandle handle,
                                                            const FrameGraphResourceInfo& info)
    {
        released_.push_back(ResourceEventRecord{handle, info});
    }

    std::size_t RecordingGpuResourceProvider::frames_begun() const noexcept
    {
        return frames_begun_;
    }

    std::size_t RecordingGpuResourceProvider::frames_completed() const noexcept
    {
        return frames_completed_;
    }

    const std::vector<RecordingGpuResourceProvider::ResourceEventRecord>&
    RecordingGpuResourceProvider::acquired() const noexcept
    {
        return acquired_;
    }

    const std::vector<RecordingGpuResourceProvider::ResourceEventRecord>&
    RecordingGpuResourceProvider::released() const noexcept
    {
        return released_;
    }

    const std::unordered_map<std::size_t, CommandBufferNativeHandle>&
    RecordingGpuResourceProvider::command_buffers() const noexcept
    {
        return command_buffers_;
    }
}
