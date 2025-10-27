#include "engine/rendering/backend/opengl/resource_provider.hpp"

#include <stdexcept>
#include <utility>

namespace engine::rendering::backend::opengl
{
    namespace
    {
        [[nodiscard]] resources::QueueNativeHandle make_queue_handle(QueueType queue, std::uint64_t id) noexcept
        {
            resources::QueueNativeHandle native{};
            native.api = resources::GraphicsApi::OpenGL;
            native.queue = queue;
            native.value = id;
            return native;
        }

        [[nodiscard]] resources::CommandBufferNativeHandle make_command_buffer_handle(
            QueueType queue, CommandBufferHandle handle, OpenGLCommandBuffer& buffer, std::uint64_t id)
        {
            resources::CommandBufferNativeHandle native{};
            native.api = resources::GraphicsApi::OpenGL;
            native.queue = queue;
            native.value = reinterpret_cast<std::uintptr_t>(&buffer);  // NOLINT
            native.label = std::string{buffer.label()};
            native.index = handle.index;
            if (native.label.empty())
            {
                native.label = "CommandBuffer";
            }
            if (native.value == 0U)
            {
                native.value = static_cast<std::uintptr_t>(id);
            }
            return native;
        }
    }  // namespace

    OpenGLGpuResourceProvider::OpenGLGpuResourceProvider() = default;

    resources::GraphicsApi OpenGLGpuResourceProvider::api() const noexcept
    {
        return resources::GraphicsApi::OpenGL;
    }

    void OpenGLGpuResourceProvider::begin_frame()
    {
        acquired_.clear();
        released_.clear();
    }

    void OpenGLGpuResourceProvider::end_frame()
    {
        // Intentionally left blank – the provider currently performs no GPU work.
    }

    resources::QueueNativeHandle OpenGLGpuResourceProvider::queue_handle(QueueType queue) const
    {
        auto it = queues_.find(queue);
        if (it != queues_.end())
        {
            return it->second.native;
        }

        QueueRecord record{};
        record.native = make_queue_handle(queue, next_queue_id_++);
        auto [inserted, success] = queues_.emplace(queue, record);
        static_cast<void>(success);
        return inserted->second.native;
    }

    resources::CommandBufferNativeHandle OpenGLGpuResourceProvider::allocate_command_buffer(
        QueueType queue, std::string_view label, CommandBufferHandle handle)
    {
        auto& record = command_buffers_[handle.index];
        if (record.buffer == nullptr)
        {
            record.buffer = std::make_unique<OpenGLCommandBuffer>();
        }

        record.buffer->reset(label, handle, queue);
        return make_command_buffer_handle(queue, handle, *record.buffer, next_command_buffer_id_++);
    }

    void OpenGLGpuResourceProvider::recycle_command_buffer(CommandBufferHandle handle)
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return;
        }
        if (it->second.buffer != nullptr)
        {
            it->second.buffer->draws.clear();
        }
    }

    resources::FenceNativeHandle OpenGLGpuResourceProvider::resolve_fence(const resources::Fence& fence)
    {
        const auto name = std::string{fence.name()};
        auto& record = fences_[name];
        if (record.native.value == 0U)
        {
            record.native.api = resources::GraphicsApi::OpenGL;
            record.native.value = next_fence_id_++;
        }
        return record.native;
    }

    resources::TimelineSemaphoreNativeHandle OpenGLGpuResourceProvider::resolve_semaphore(
        const resources::TimelineSemaphore& semaphore)
    {
        const auto name = std::string{semaphore.name()};
        auto& record = timelines_[name];
        if (record.native.value == 0U)
        {
            record.native.api = resources::GraphicsApi::OpenGL;
            record.native.value = next_timeline_id_++;
        }
        return record.native;
    }

    void OpenGLGpuResourceProvider::on_transient_acquire(FrameGraphResourceHandle handle,
                                                          const FrameGraphResourceInfo& info)
    {
        static_cast<void>(info);
        acquired_.push_back(handle);
    }

    void OpenGLGpuResourceProvider::on_transient_release(FrameGraphResourceHandle handle,
                                                          const FrameGraphResourceInfo& info)
    {
        static_cast<void>(info);
        released_.push_back(handle);
    }

    OpenGLCommandBuffer* OpenGLGpuResourceProvider::command_buffer(CommandBufferHandle handle) noexcept
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return nullptr;
        }
        return it->second.buffer.get();
    }

    const OpenGLCommandBuffer* OpenGLGpuResourceProvider::command_buffer(CommandBufferHandle handle) const noexcept
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return nullptr;
        }
        return it->second.buffer.get();
    }
}

