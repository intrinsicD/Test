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
} // namespace engine::io

