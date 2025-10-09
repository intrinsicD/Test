#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    struct FrameGraphResourceInfo;
}

namespace engine::rendering::resources
{
    /// Enumeration of the graphics API exposed through native handles.
    enum class GraphicsApi
    {
        Unknown,
        Vulkan,
        DirectX12,
        Metal,
        OpenGL,
    };

    /// Native GPU queue identifier surfaced to backend adapters.
    struct QueueNativeHandle
    {
        GraphicsApi api{GraphicsApi::Unknown};
        std::uintptr_t value{0};
        QueueType queue{QueueType::Graphics};
    };

    /// Native command buffer identifier surfaced to backend adapters.
    struct CommandBufferNativeHandle
    {
        GraphicsApi api{GraphicsApi::Unknown};
        std::uintptr_t value{0};
        QueueType queue{QueueType::Graphics};
        std::string label{};
        std::size_t index{0};
    };

    /// Native fence object used to detect GPU completion of work.
    struct FenceNativeHandle
    {
        GraphicsApi api{GraphicsApi::Unknown};
        std::uintptr_t value{0};
    };

    /// Native timeline semaphore object used to sequence submissions.
    struct TimelineSemaphoreNativeHandle
    {
        GraphicsApi api{GraphicsApi::Unknown};
        std::uintptr_t value{0};
    };

    /**
     * \brief Backend-neutral interface that exposes GPU objects and lifetime hooks.
     */
    class IGpuResourceProvider
    {
    public:
        virtual ~IGpuResourceProvider() = default;

        /// Identify the API that backs the native handles returned by this provider.
        [[nodiscard]] virtual GraphicsApi api() const noexcept = 0;

        /// Called before the frame graph starts encoding work for a frame.
        virtual void begin_frame() = 0;

        /// Called once the frame graph has queued all work for the current frame.
        virtual void end_frame() = 0;

        /// Retrieve the native handle describing the queue that matches \p queue.
        [[nodiscard]] virtual QueueNativeHandle queue_handle(QueueType queue) const = 0;

        /// Allocate a command buffer suitable for \p queue and associate it with \p handle.
        [[nodiscard]] virtual CommandBufferNativeHandle allocate_command_buffer(
            QueueType queue, std::string_view label, CommandBufferHandle handle) = 0;

        /// Release the command buffer previously associated with \p handle back to the pool.
        virtual void recycle_command_buffer(CommandBufferHandle handle) = 0;

        /// Resolve \p fence to its native API handle.
        [[nodiscard]] virtual FenceNativeHandle resolve_fence(const Fence& fence) = 0;

        /// Resolve \p semaphore to its native API handle.
        [[nodiscard]] virtual TimelineSemaphoreNativeHandle resolve_semaphore(
            const TimelineSemaphore& semaphore) = 0;

        /// Notify the provider that a transient resource identified by \p handle became live.
        virtual void on_transient_acquire(FrameGraphResourceHandle handle,
                                          const FrameGraphResourceInfo& info) = 0;

        /// Notify the provider that a transient resource identified by \p handle became idle.
        virtual void on_transient_release(FrameGraphResourceHandle handle,
                                          const FrameGraphResourceInfo& info) = 0;
    };
}
