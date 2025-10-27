#include "engine/io/importers/animation.hpp"

#include "engine/io/errors.hpp"

#include "engine/animation/api.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace engine::io::animation
{
    namespace
    {
        [[nodiscard]] std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
            {
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

        [[nodiscard]] AnimationIoResult<ClipFormat> sniff_json_signature(const std::filesystem::path& path)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                return make_animation_io_error(AnimationIoError::io_failure,
                                               "Failed to open animation clip for detection: " + path.string());
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

    AnimationIoResult<ClipFormat> detect_clip_format(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return make_animation_io_error(AnimationIoError::file_not_found,
                                           "Animation clip path does not exist: " + path.string());
        }

        if (const auto from_ext = classify_extensions(path); from_ext != ClipFormat::unknown)
        {
            return from_ext;
        }

        return sniff_json_signature(path);
    }

    AnimationIoResult<engine::animation::AnimationClip> load_clip(const std::filesystem::path& path,
                                                                  ClipFormat format)
    {
        ClipFormat resolved = format;
        if (resolved == ClipFormat::unknown)
        {
            auto detection = detect_clip_format(path);
            if (!detection)
            {
                return detection.error();
            }
            resolved = detection.value();
        }

        switch (resolved)
        {
        case ClipFormat::json:
            try
            {
                return engine::animation::load_clip_json(path);
            }
            catch (const std::system_error& error)
            {
                return make_animation_io_error(AnimationIoError::io_failure, error.what());
            }
            catch (const std::exception& error)
            {
                return make_animation_io_error(AnimationIoError::decode_failure, error.what());
            }
        case ClipFormat::unknown:
        default:
            return make_animation_io_error(AnimationIoError::unsupported_format,
                                           "Unsupported animation clip format for path: " + path.string());
        }
    }

    AnimationIoResult<void> save_clip(const engine::animation::AnimationClip& clip,
                                      const std::filesystem::path& path,
                                      ClipFormat format,
                                      bool pretty)
    {
        const auto resolved = (format == ClipFormat::unknown) ? ClipFormat::json : format;
        switch (resolved)
        {
        case ClipFormat::json:
            try
            {
                engine::animation::save_clip_json(clip, path, pretty);
                return {};
            }
            catch (const std::system_error& error)
            {
                return make_animation_io_error(AnimationIoError::io_failure, error.what());
            }
            catch (const std::exception& error)
            {
                return make_animation_io_error(AnimationIoError::serialization_failure, error.what());
            }
        case ClipFormat::unknown:
        default:
            return make_animation_io_error(AnimationIoError::unsupported_format,
                                           "Unsupported animation clip format for path: " + path.string());
        }
    }
} // namespace engine::io::animation