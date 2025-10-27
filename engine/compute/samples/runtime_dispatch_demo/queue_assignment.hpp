#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace engine::compute::samples
{
    [[nodiscard]] inline std::size_t deterministic_queue_index(
        std::string_view category,
        std::size_t queue_count) noexcept
    {
        if (queue_count <= 1U)
        {
            return 0U;
        }

        constexpr std::uint64_t kOffset = 1469598103934665603ULL;
        constexpr std::uint64_t kPrime = 1099511628211ULL;

        std::uint64_t hash = kOffset;
        for (unsigned char raw : category)
        {
            const unsigned char lowered = static_cast<unsigned char>(std::tolower(raw));
            hash ^= static_cast<std::uint64_t>(lowered);
            hash *= kPrime;
        }

        const std::uint64_t modulus = static_cast<std::uint64_t>(queue_count);
        return static_cast<std::size_t>(hash % modulus);
    }
} // namespace engine::compute::samples