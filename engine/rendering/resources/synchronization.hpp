#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace engine::rendering
{
    struct FrameGraphResourceHandle;
}

namespace engine::rendering::resources
{
    /// Pipeline stages recognised by the synchronisation primitives.
    enum class PipelineStage
    {
        Graphics,
        Compute,
        Transfer,
    };

    /// Access type granted to a resource before or after a barrier executes.
    enum class Access
    {
        None,
        Read,
        Write,
    };

    /// Description of a resource barrier issued around a render pass execution.
    struct Barrier
    {
        FrameGraphResourceHandle resource{};
        PipelineStage source_stage{PipelineStage::Graphics};
        PipelineStage destination_stage{PipelineStage::Graphics};
        Access source_access{Access::None};
        Access destination_access{Access::None};
    };

    /// Fence used to coordinate CPU/GPU completion of submissions.
    class Fence
    {
    public:
        explicit Fence(std::string name, std::uint64_t initial_value = 0)
            : name_(std::move(name)), value_(initial_value)
        {
        }

        void signal(std::uint64_t value)
        {
            value_ = std::max(value_, value);
        }

        [[nodiscard]] std::uint64_t value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] std::string_view name() const noexcept
        {
            return name_;
        }

    private:
        std::string name_;
        std::uint64_t value_;
    };

    /// Timeline semaphore used to serialise GPU submissions.
    class TimelineSemaphore
    {
    public:
        explicit TimelineSemaphore(std::string name, std::uint64_t initial_value = 0)
            : name_(std::move(name)), value_(initial_value), last_wait_value_(initial_value)
        {
        }

        void signal(std::uint64_t value)
        {
            value_ = std::max(value_, value);
        }

        void wait(std::uint64_t value)
        {
            last_wait_value_ = std::max(last_wait_value_, value);
        }

        [[nodiscard]] std::string_view name() const noexcept
        {
            return name_;
        }

        [[nodiscard]] std::uint64_t value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] std::uint64_t last_wait_value() const noexcept
        {
            return last_wait_value_;
        }

    private:
        std::string name_;
        std::uint64_t value_;
        std::uint64_t last_wait_value_;
    };

    /// Wait operation used when submitting GPU work.
    struct SemaphoreWait
    {
        TimelineSemaphore* semaphore{nullptr};
        std::uint64_t value{0};
    };

    /// Signal operation used when submitting GPU work.
    struct SemaphoreSignal
    {
        TimelineSemaphore* semaphore{nullptr};
        std::uint64_t value{0};
    };
}

