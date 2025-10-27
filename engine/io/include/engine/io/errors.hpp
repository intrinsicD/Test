#pragma once

#include "engine/core/diagnostics/error.hpp"
#include "engine/core/diagnostics/result.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace engine::io
{
    enum class GeometryIoError : int
    {
        file_not_found = 1,
        io_failure,
        invalid_argument,
        unsupported_format,
        plugin_missing
    };

    [[nodiscard]] constexpr std::string_view to_string(GeometryIoError error) noexcept
    {
        switch (error)
        {
        case GeometryIoError::file_not_found:
            return "file_not_found";
        case GeometryIoError::io_failure:
            return "io_failure";
        case GeometryIoError::invalid_argument:
            return "invalid_argument";
        case GeometryIoError::unsupported_format:
            return "unsupported_format";
        case GeometryIoError::plugin_missing:
            return "plugin_missing";
        }

        return "unknown";
    }

    class GeometryIoErrorCode final : public engine::EnumeratedErrorCode<GeometryIoError>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] GeometryIoErrorCode with_message(std::string message) const
        {
            GeometryIoErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline GeometryIoErrorCode make_geometry_io_error(GeometryIoError error,
                                                                    std::string message = {})
    {
        GeometryIoErrorCode code{"engine.io", error, to_string(error)};
        if (!message.empty())
        {
            code = code.with_message(std::move(message));
        }
        return code;
    }

    template <typename T>
    using GeometryIoResult = engine::Result<T, GeometryIoErrorCode>;

    enum class AnimationIoError : int
    {
        file_not_found = 1,
        io_failure,
        unsupported_format,
        decode_failure,
        serialization_failure
    };

    [[nodiscard]] constexpr std::string_view to_string(AnimationIoError error) noexcept
    {
        switch (error)
        {
        case AnimationIoError::file_not_found:
            return "file_not_found";
        case AnimationIoError::io_failure:
            return "io_failure";
        case AnimationIoError::unsupported_format:
            return "unsupported_format";
        case AnimationIoError::decode_failure:
            return "decode_failure";
        case AnimationIoError::serialization_failure:
            return "serialization_failure";
        }

        return "unknown";
    }

    class AnimationIoErrorCode final : public engine::EnumeratedErrorCode<AnimationIoError>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] AnimationIoErrorCode with_message(std::string message) const
        {
            AnimationIoErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline AnimationIoErrorCode make_animation_io_error(AnimationIoError error,
                                                                      std::string message = {})
    {
        AnimationIoErrorCode code{"engine.io", error, to_string(error)};
        if (!message.empty())
        {
            code = code.with_message(std::move(message));
        }
        return code;
    }

    template <typename T>
    using AnimationIoResult = engine::Result<T, AnimationIoErrorCode>;
} // namespace engine::io