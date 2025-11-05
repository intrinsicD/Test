#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#if ENGINE_RENDERING_HAS_VULKAN
#    include <optional>
#    include "engine/rendering/backend/vulkan/resource_translation.hpp"
#endif

#include "engine/rendering/backend/vulkan/command_encoder.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering::backend::vulkan
{
    /**
     * \brief GPU resource provider that surfaces Vulkan-native handles.
     *
     * The provider focuses on deterministic resource lifecycle tracking so
     * command encoder and scheduler work can progress in environments without a
     * live Vulkan device. Handles are synthesised and metadata is retained so
     * higher layers can validate residency, retention, and barrier contracts.
     */
    class VulkanGpuResourceProvider final : public resources::IGpuResourceProvider
    {
    public:
        explicit VulkanGpuResourceProvider(std::uint64_t retention_frames = 0);

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
        [[nodiscard]] resources::GpuResourceUsage usage_snapshot() const noexcept override;

        void on_transient_acquire(FrameGraphResourceHandle handle, const FrameGraphResourceInfo& info) override;
        void on_transient_release(FrameGraphResourceHandle handle, const FrameGraphResourceInfo& info) override;

        struct ResourceEventRecord
        {
            FrameGraphResourceHandle handle{};
            FrameGraphResourceInfo info{};
        };

        [[nodiscard]] const std::vector<ResourceEventRecord>& acquired() const noexcept;
        [[nodiscard]] const std::vector<ResourceEventRecord>& released() const noexcept;

        struct BufferRecord
        {
            std::string name;
            ResourceUsage usage{ResourceUsage::None};
            std::uint64_t size_bytes{0};
            std::uint64_t handle{0};
            bool in_use{false};
            std::uint64_t last_used_frame{0};
#if ENGINE_RENDERING_HAS_VULKAN
            std::optional<VulkanBufferResourceDescription> description{};
#endif
        };

        [[nodiscard]] const BufferRecord* buffer(FrameGraphResourceHandle handle) const noexcept;
        [[nodiscard]] std::uint64_t active_buffer_bytes() const noexcept;

        struct ImageRecord
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
            std::uint64_t image{0};
            std::uint64_t view{0};
            bool depth_attachment{false};
            bool multisampled{false};
            bool in_use{false};
            std::uint64_t last_used_frame{0};
            std::uint64_t byte_size{0};
#if ENGINE_RENDERING_HAS_VULKAN
            std::optional<VulkanImageResourceDescription> description{};
#endif
        };

        [[nodiscard]] const ImageRecord* image(FrameGraphResourceHandle handle) const noexcept;
        [[nodiscard]] std::uint64_t active_image_bytes() const noexcept;
        [[nodiscard]] std::uint64_t active_memory_bytes() const noexcept;

        struct CommandBufferRecord
        {
            resources::CommandBufferNativeHandle native{};
            std::string label;
            QueueType queue{QueueType::Graphics};
            std::unique_ptr<VulkanCommandBuffer> buffer;
        };

        [[nodiscard]] VulkanCommandBuffer* command_buffer(CommandBufferHandle handle) noexcept;
        [[nodiscard]] const VulkanCommandBuffer* command_buffer(CommandBufferHandle handle) const noexcept;

        void set_retention_frames(std::uint64_t frames) noexcept;
        [[nodiscard]] std::uint64_t retention_frames() const noexcept;

#if ENGINE_RENDERING_HAS_VULKAN
        [[nodiscard]] const VulkanBufferResourceDescription*
        buffer_description(FrameGraphResourceHandle handle) const noexcept;
        [[nodiscard]] const VulkanImageResourceDescription*
        image_description(FrameGraphResourceHandle handle) const noexcept;
#endif

    private:
        struct QueueRecord
        {
            resources::QueueNativeHandle native{};
        };

        mutable std::unordered_map<QueueType, QueueRecord> queues_{};
        std::unordered_map<std::size_t, CommandBufferRecord> command_buffers_{};
        std::unordered_map<std::string, resources::FenceNativeHandle> fences_{};
        std::unordered_map<std::string, resources::TimelineSemaphoreNativeHandle> timelines_{};
        std::unordered_map<std::size_t, BufferRecord> buffers_{};
        std::unordered_map<std::size_t, ImageRecord> images_{};
        std::vector<ResourceEventRecord> acquired_{};
        std::vector<ResourceEventRecord> released_{};

        std::uint64_t current_frame_{0};
        std::uint64_t retention_frames_{0};
        mutable std::atomic<std::uint64_t> next_queue_id_{1};
        std::uint64_t next_command_buffer_id_{1};
        std::uint64_t next_buffer_id_{1};
        std::uint64_t next_image_id_{1};
        std::uint64_t next_view_id_{1};
        std::uint64_t next_fence_id_{1};
        std::uint64_t next_timeline_id_{1};
        std::uint64_t active_buffer_bytes_{0};
        std::uint64_t active_image_bytes_{0};

        [[nodiscard]] static bool is_buffer_resource(const FrameGraphResourceInfo& info) noexcept;
        [[nodiscard]] static bool is_image_resource(const FrameGraphResourceInfo& info) noexcept;
        [[nodiscard]] static bool is_depth_format(ResourceFormat format) noexcept;
        [[nodiscard]] static std::uint32_t sample_count_value(ResourceSampleCount count) noexcept;
        [[nodiscard]] static std::uint32_t bytes_per_pixel(ResourceFormat format) noexcept;
        [[nodiscard]] static std::uint64_t estimate_image_size(const FrameGraphResourceInfo& info) noexcept;

        void allocate_buffer(std::size_t index, const FrameGraphResourceInfo& info);
        void allocate_image(std::size_t index, const FrameGraphResourceInfo& info);
        void destroy_buffer(BufferRecord& record) noexcept;
        void destroy_image(ImageRecord& record) noexcept;
        [[nodiscard]] bool buffer_descriptor_matches(const BufferRecord& record,
                                                     const FrameGraphResourceInfo& info) const noexcept;
        [[nodiscard]] bool image_descriptor_matches(const ImageRecord& record,
                                                    const FrameGraphResourceInfo& info) const noexcept;
    };
}

