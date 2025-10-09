#pragma once

#include <cstddef>
#include <limits>

namespace engine::rendering
{
    struct FrameGraphResourceHandle
    {
        std::size_t index{std::numeric_limits<std::size_t>::max()};

        [[nodiscard]] bool valid() const noexcept
        {
            return index != std::numeric_limits<std::size_t>::max();
        }

        friend bool operator==(FrameGraphResourceHandle lhs, FrameGraphResourceHandle rhs) noexcept
        {
            return lhs.index == rhs.index;
        }
    };
}  // namespace engine::rendering
