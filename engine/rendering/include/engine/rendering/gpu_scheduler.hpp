#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

#include "../../rendering/resources/synchronization.hpp"

namespace engine::rendering
{
    class RenderPass;

    /// Queue families that the scheduler can target when dispatching work.
    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
    };

    /// Handle referencing a backend-specific command buffer.
    struct CommandBufferHandle
    {
        std::size_t index{std::numeric_limits<std::size_t>::max()};

        [[nodiscard]] bool valid() const noexcept
        {
            return index != std::numeric_limits<std::size_t>::max();
        }

        friend bool operator==(CommandBufferHandle lhs, CommandBufferHandle rhs) noexcept
        {
            return lhs.index == rhs.index;
        }

        friend bool operator!=(CommandBufferHandle lhs, CommandBufferHandle rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

    /// Submission payload describing the GPU work encoded by a render pass.
    struct GpuSubmitInfo
    {
        std::string_view pass_name;
        QueueType queue{QueueType::Graphics};
        CommandBufferHandle command_buffer{};
        std::vector<resources::Barrier> begin_barriers;
        std::vector<resources::Barrier> end_barriers;
        std::vector<resources::SemaphoreWait> waits;
        std::vector<resources::SemaphoreSignal> signals;
        resources::Fence* fence{nullptr};
        std::uint64_t fence_value{0};
    };

    /**
     * \brief Abstract interface that manages GPU submissions for the renderer.
     */
    class IGpuScheduler
    {
    public:
        virtual ~IGpuScheduler() = default;

        /// Select the queue that should execute \p pass.
        virtual QueueType select_queue(const RenderPass& pass) = 0;

        /// Allocate a command buffer compatible with \p queue.
        virtual CommandBufferHandle request_command_buffer(QueueType queue, std::string_view pass_name) = 0;

        /// Submit the recorded work described by \p info.
        virtual void submit(const GpuSubmitInfo& info) = 0;

        /// Recycle a command buffer after the GPU work has been enqueued.
        virtual void recycle(CommandBufferHandle handle) = 0;
    };
}

