#pragma once

#include <string>
#include <string_view>

namespace engine::scene::components
{
    struct Name
    {
        std::string value{};
    };

    [[nodiscard]] inline std::string_view view(const Name& name) noexcept
    {
        return name.value;
    }

    [[nodiscard]] inline bool operator==(const Name& name, std::string_view text) noexcept
    {
        return name.value == text;
    }

    [[nodiscard]] inline bool operator==(std::string_view text, const Name& name) noexcept
    {
        return name == text;
    }
} // namespace engine::scene::components
