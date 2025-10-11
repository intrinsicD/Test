#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace engine::assets::detail {

/// Shared helper that normalises filesystem timestamp queries for asset caches.
[[nodiscard]] inline std::filesystem::file_time_type checked_last_write_time(
    const std::filesystem::path& path,
    std::string_view asset_kind)
{
    std::error_code ec{};
    const auto time = std::filesystem::last_write_time(path, ec);
    if (ec) {
        throw std::runtime_error(
            std::string{"Failed to query "} + std::string{asset_kind} +
            " asset timestamp: " + ec.message());
    }
    return time;
}

}  // namespace engine::assets::detail

