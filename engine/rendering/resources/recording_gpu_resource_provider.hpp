#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering::resources
{
    /// GPU resource provider that records events for test validation.
    class RecordingGpuResourceProvider final : public IGpuResourceProvider
    {
    public:
        explicit RecordingGpuResourceProvider(GraphicsApi api = GraphicsApi::Vulkan);

        [[nodiscard]] GraphicsApi api() const noexcept override;

        void begin_frame() override;
        void end_frame() override;

        [[nodiscard]] QueueNativeHandle queue_handle(QueueType queue) const override;
        [[nodiscard]] CommandBufferNativeHandle allocate_command_buffer(
            QueueType queue, std::string_view label, CommandBufferHandle handle) override;
        void recycle_command_buffer(CommandBufferHandle handle) override;
        [[nodiscard]] FenceNativeHandle resolve_fence(const Fence& fence) override;
        [[nodiscard]] TimelineSemaphoreNativeHandle resolve_semaphore(
            const TimelineSemaphore& semaphore) override;

        void on_transient_acquire(FrameGraphResourceHandle handle,
                                  const FrameGraphResourceInfo& info) override;
        void on_transient_release(FrameGraphResourceHandle handle,
                                  const FrameGraphResourceInfo& info) override;

        struct ResourceEventRecord
        {
            FrameGraphResourceHandle handle{};
            FrameGraphResourceInfo info{};
        };

        [[nodiscard]] std::size_t frames_begun() const noexcept;
        [[nodiscard]] std::size_t frames_completed() const noexcept;
        [[nodiscard]] const std::vector<ResourceEventRecord>& acquired() const noexcept;
        [[nodiscard]] const std::vector<ResourceEventRecord>& released() const noexcept;
        [[nodiscard]] const std::unordered_map<std::size_t, CommandBufferNativeHandle>&
            command_buffers() const noexcept;

    private:
        GraphicsApi api_{GraphicsApi::Unknown};
        mutable std::unordered_map<QueueType, QueueNativeHandle> queues_{};
        std::unordered_map<std::size_t, CommandBufferNativeHandle> command_buffers_{};
        mutable std::size_t next_queue_value_{1};
        std::size_t next_command_buffer_value_{1};
        std::size_t frames_begun_{0};
        std::size_t frames_completed_{0};
        std::vector<ResourceEventRecord> acquired_{};
        std::vector<ResourceEventRecord> released_{};
    };
}
