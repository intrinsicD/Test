#pragma once

#include "engine/io/api.hpp"
#include "engine/io/errors.hpp"
#include "engine/animation/api.hpp"

#include <cstdint>
#include <filesystem>
#include <ostream>

namespace engine::io::animation
{
    enum class ClipFormat : std::uint8_t
    {
        unknown = 0U,
        json,
    };

    inline std::ostream& operator<<(std::ostream& os, ClipFormat format)
    {
        switch (format)
        {
        case ClipFormat::unknown:
            return os << "unknown";
        case ClipFormat::json:
            return os << "json";
        }

        return os;
    }

    [[nodiscard]] ENGINE_IO_API AnimationIoResult<ClipFormat>
    detect_clip_format(const std::filesystem::path& path);

    [[nodiscard]] ENGINE_IO_API AnimationIoResult<engine::animation::AnimationClip>
    load_clip(const std::filesystem::path& path, ClipFormat format = ClipFormat::unknown);

    [[nodiscard]] ENGINE_IO_API AnimationIoResult<void>
    save_clip(const engine::animation::AnimationClip& clip,
              const std::filesystem::path& path,
              ClipFormat format = ClipFormat::unknown,
              bool pretty = true);
}