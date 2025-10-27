#pragma once

#include "engine/assets/async.hpp"
#include "engine/io/errors.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace engine::assets::detail
{
    [[nodiscard]] inline AssetLoadErrorCategory map_geometry_error(
        const io::GeometryIoErrorCode& error) noexcept
    {
        switch (error.code())
        {
        case io::GeometryIoError::file_not_found:
        case io::GeometryIoError::io_failure:
            return AssetLoadErrorCategory::IoFailure;
        case io::GeometryIoError::invalid_argument:
        case io::GeometryIoError::plugin_missing:
            return AssetLoadErrorCategory::ValidationError;
        case io::GeometryIoError::unsupported_format:
            return AssetLoadErrorCategory::DecodeError;
        }

        return AssetLoadErrorCategory::ValidationError;
    }

    [[nodiscard]] inline std::string geometry_error_hint(const io::GeometryIoErrorCode& error)
    {
        switch (error.code())
        {
        case io::GeometryIoError::file_not_found:
            return "Verify the source asset path exists and is accessible to the runtime process.";
        case io::GeometryIoError::io_failure:
            return "Check filesystem permissions and ensure the asset is not locked by another application.";
        case io::GeometryIoError::invalid_argument:
            return "Ensure the asset descriptor references a valid file path and format hint.";
        case io::GeometryIoError::unsupported_format:
            return "Confirm the asset uses a supported format or provide an explicit format hint.";
        case io::GeometryIoError::plugin_missing:
            return "Enable or install the required IO plugin for this asset format.";
        }

        return "Inspect recent reload logs for additional diagnostics.";
    }

    [[nodiscard]] inline AssetLoadError make_geometry_asset_error(const std::filesystem::path& path,
                                                                  std::string_view context,
                                                                  const io::GeometryIoErrorCode& error)
    {
        std::string message;
        if (!path.empty())
        {
            message = path.generic_string();
        }

        if (!context.empty())
        {
            if (!message.empty())
            {
                message.append(": ");
            }
            message.append(context);
        }

        if (error.has_message())
        {
            if (!message.empty())
            {
                message.append(": ");
            }
            message.append(std::string{error.message()});
        }

        return make_asset_load_error(map_geometry_error(error), std::move(message)).with_geometry_error(error);
    }

    inline void record_hot_reload_attempt(bool notify, std::string_view identifier)
    {
        if (!notify)
        {
            return;
        }

        AssetHotReloadTelemetry::instance().record_attempt(identifier);
    }

    inline void record_hot_reload_failure(bool notify,
                                          std::string_view identifier,
                                          const AssetLoadError& error,
                                          std::string_view hint = {})
    {
        if (!notify)
        {
            return;
        }

        AssetHotReloadTelemetry::instance().record_failure(error, identifier, hint);
    }

    inline void record_hot_reload_cancelled()
    {
        AssetHotReloadTelemetry::instance().record_cancelled();
    }

    inline void record_hot_reload_rejected()
    {
        AssetHotReloadTelemetry::instance().record_rejected();
    }
} // namespace engine::assets::detail