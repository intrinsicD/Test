#include "engine/io/importers/animation.hpp"

#include "engine/animation/api.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace engine::io::animation
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        [[nodiscard]] ClipFormat classify_extensions(const std::filesystem::path& path)
        {
            ClipFormat format = ClipFormat::unknown;
            auto current = path;
            for (int depth = 0; depth < 3 && !current.extension().empty(); ++depth)
            {
                const auto ext = to_lower(current.extension().string());
                if (ext == ".json")
                {
                    format = ClipFormat::json;
                    break;
                }
                if (ext == ".anim" || ext == ".clip")
                {
                    format = ClipFormat::json;
                    break;
                }
                current = current.stem();
            }
            return format;
        }

        [[nodiscard]] ClipFormat sniff_json_signature(const std::filesystem::path& path)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open animation file for detection: " + path.string());
            }
            char ch = '\0';
            while (stream.get(ch))
            {
                if (std::isspace(static_cast<unsigned char>(ch)) != 0)
                {
                    continue;
                }
                if (ch == '{' || ch == '[')
                {
                    return ClipFormat::json;
                }
                break;
            }
            return ClipFormat::unknown;
        }
    } // namespace

    ClipFormat detect_clip_format(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            throw std::runtime_error("Animation clip path does not exist: " + path.string());
        }

        if (const auto from_ext = classify_extensions(path); from_ext != ClipFormat::unknown)
        {
            return from_ext;
        }

        return sniff_json_signature(path);
    }

    engine::animation::AnimationClip load_clip(const std::filesystem::path& path, ClipFormat format)
    {
        const auto resolved = (format == ClipFormat::unknown) ? detect_clip_format(path) : format;
        switch (resolved)
        {
        case ClipFormat::json:
            return engine::animation::load_clip_json(path);
        case ClipFormat::unknown:
        default:
            throw std::runtime_error("Unsupported animation clip format for path: " + path.string());
        }
    }

    void save_clip(const engine::animation::AnimationClip& clip,
                   const std::filesystem::path& path,
                   ClipFormat format,
                   bool pretty)
    {
        const auto resolved = (format == ClipFormat::unknown) ? ClipFormat::json : format;
        switch (resolved)
        {
        case ClipFormat::json:
            engine::animation::save_clip_json(clip, path, pretty);
            break;
        case ClipFormat::unknown:
        default:
            throw std::runtime_error("Unsupported animation clip format for path: " + path.string());
        }
    }
} // namespace engine::io::animation
