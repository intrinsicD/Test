#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/rendering/backend/opengl/command_encoder.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering::backend::opengl
{
    /**
     * \brief GPU resource provider that exposes OpenGL native handles.
     *
     * This initial implementation focuses on command buffer orchestration so that
     * command encoder work can progress independently of real GPU allocations.
     */
    class OpenGLGpuResourceProvider final : public resources::IGpuResourceProvider
    {
    public:
        OpenGLGpuResourceProvider();
        ~OpenGLGpuResourceProvider() override;

        [[nodiscard]] resources::GraphicsApi api() const noexcept override;

        void begin_frame() override;
        void end_frame() override;

        [[nodiscard]] resources::QueueNativeHandle queue_handle(QueueType queue) const override;
        [[nodiscard]] resources::CommandBufferNativeHandle allocate_command_buffer(
            QueueType queue, std::string_view label, CommandBufferHandle handle) override;
        void recycle_command_buffer(CommandBufferHandle handle) override;

        [[nodiscard]] resources::FenceNativeHandle resolve_fence(const resources::Fence& fence) override;
        [[nodiscard]] resources::TimelineSemaphoreNativeHandle resolve_semaphore(
            const resources::TimelineSemaphore& semaphore) override;

        void on_transient_acquire(FrameGraphResourceHandle handle, const FrameGraphResourceInfo& info) override;
        void on_transient_release(FrameGraphResourceHandle handle, const FrameGraphResourceInfo& info) override;

        [[nodiscard]] OpenGLCommandBuffer* command_buffer(CommandBufferHandle handle) noexcept;
        [[nodiscard]] const OpenGLCommandBuffer* command_buffer(CommandBufferHandle handle) const noexcept;

        struct TextureRecord
        {
            std::string name;
            ResourceFormat format{ResourceFormat::Unknown};
            ResourceUsage usage{ResourceUsage::None};
            ResourceDimension dimension{ResourceDimension::Unknown};
            ResourceSampleCount sample_count{ResourceSampleCount::Count1};
            std::uint32_t width{0};
            std::uint32_t height{0};
            std::uint32_t depth{0};
            std::uint32_t array_layers{0};
            std::uint32_t mip_levels{0};
            std::uint32_t handle{0};
            bool native_allocation{false};
            bool depth_attachment{false};
            bool in_use{false};
        };

        [[nodiscard]] const TextureRecord* texture(FrameGraphResourceHandle handle) const noexcept;

    private:
        struct QueueRecord
        {
            resources::QueueNativeHandle native{};
        };

        struct FenceRecord
        {
            resources::FenceNativeHandle native{};
        };

        struct TimelineRecord
        {
            resources::TimelineSemaphoreNativeHandle native{};
        };

        struct CommandBufferRecord
        {
            std::unique_ptr<OpenGLCommandBuffer> buffer;
        };

        mutable std::unordered_map<QueueType, QueueRecord> queues_{};
        std::unordered_map<std::size_t, CommandBufferRecord> command_buffers_{};
        std::unordered_map<std::string, FenceRecord> fences_{};
        std::unordered_map<std::string, TimelineRecord> timelines_{};
        std::vector<FrameGraphResourceHandle> acquired_{};
        std::vector<FrameGraphResourceHandle> released_{};
        std::unordered_map<std::size_t, TextureRecord> textures_{};
        mutable std::uint64_t next_queue_id_{1};
        std::uint64_t next_command_buffer_id_{1};
        std::uint64_t next_fence_id_{1};
        std::uint64_t next_timeline_id_{1};
        std::uint32_t next_texture_id_{1};

        void destroy_texture(TextureRecord& record) noexcept;
        void allocate_texture(std::size_t index, const FrameGraphResourceInfo& info);
        [[nodiscard]] bool texture_descriptor_matches(const TextureRecord& record,
                                                      const FrameGraphResourceInfo& info) const noexcept;
    };
}