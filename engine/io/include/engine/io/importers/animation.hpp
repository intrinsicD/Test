#pragma once

#include "engine/io/api.hpp"
#include "engine/animation/api.hpp"

#include <cstdint>
#include <filesystem>

namespace engine::io::animation
{
    enum class ClipFormat : std::uint8_t
    {
        unknown = 0U,
        json,
    };

    std::ostream & operator<<(std::ostream& os, ClipFormat format)
    {
        switch (format)
        {
            case ClipFormat::unknown:
                return os << "unknown";
            case ClipFormat::json:
                return os << "json";
        }
    }

    [[nodiscard]] ENGINE_IO_API ClipFormat detect_clip_format(const std::filesystem::path& path);

    [[nodiscard]] ENGINE_IO_API engine::animation::AnimationClip load_clip(const std::filesystem::path& path,
                                                                           ClipFormat format = ClipFormat::unknown);

    ENGINE_IO_API void save_clip(const engine::animation::AnimationClip& clip,
                                 const std::filesystem::path& path,
                                 ClipFormat format = ClipFormat::unknown,
                                 bool pretty = true);
}
