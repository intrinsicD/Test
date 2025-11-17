#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering
{
    /**
     * \brief Pool that recycles command buffer handles across queue families.
     */
    class CommandBufferPool
    {
    public:
        struct AcquireResult
        {
            CommandBufferHandle handle{};
            resources::CommandBufferNativeHandle native{};
            bool recycled{false};
        };

        struct Metrics
        {
            std::uint64_t requests{0};
            std::uint64_t hits{0};
            std::uint64_t trims{0};
            std::size_t live_handles{0};
            std::size_t pooled_handles{0};

            [[nodiscard]] double hit_rate() const noexcept
            {
                if (requests == 0)
                {
                    return 0.0;
                }
                return static_cast<double>(hits) / static_cast<double>(requests);
            }
        };

        explicit CommandBufferPool(resources::IGpuResourceProvider& provider,
                                   std::uint64_t max_age_frames = 30) noexcept;

        void begin_frame(std::uint64_t frame_index) noexcept;
        void end_frame();

        [[nodiscard]] AcquireResult acquire(QueueType queue, std::string_view label);
        void release(CommandBufferHandle handle);
        void trim_unused();

        void set_max_age_frames(std::uint64_t frames) noexcept;
        [[nodiscard]] std::uint64_t max_age_frames() const noexcept;

        [[nodiscard]] Metrics metrics() const noexcept;

    private:
        static constexpr std::size_t queue_count = 3;

        struct BufferMetadata
        {
            std::size_t queue_index{0};
            std::uint64_t last_frame_used{0};
            bool in_use{false};
        };

        [[nodiscard]] static constexpr std::size_t queue_to_index(QueueType queue) noexcept
        {
            switch (queue)
            {
            case QueueType::Graphics:
                return 0;
            case QueueType::Compute:
                return 1;
            case QueueType::Transfer:
                return 2;
            }
            return 0;
        }

        resources::IGpuResourceProvider& provider_;
        std::array<std::vector<CommandBufferHandle>, queue_count> available_{};
        std::unordered_map<std::size_t, BufferMetadata> metadata_{};
        std::size_t next_handle_index_{0};
        std::uint64_t current_frame_{0};
        std::uint64_t max_age_frames_{30};
        std::uint64_t requests_{0};
        std::uint64_t hits_{0};
        std::uint64_t trims_{0};
    };
}
