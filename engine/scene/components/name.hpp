#pragma once

#include <iomanip>
#include <istream>
#include <ostream>
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

    namespace serialization
    {
        inline void encode(std::ostream& output, const Name& name)
        {
            output << std::quoted(name.value);
        }

        inline Name decode_name(std::istream& input)
        {
            Name name{};
            input >> std::quoted(name.value);
            return name;
        }
    } // namespace serialization
} // namespace engine::scene::components
