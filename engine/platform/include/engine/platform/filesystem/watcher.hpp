#pragma once

#include "engine/platform/api.hpp"

#include <filesystem>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace engine::platform::filesystem {

enum class WatchEventType
{
    created,
    modified,
    erased,
};

struct WatchEvent
{
    WatchEventType type{WatchEventType::modified};
    std::filesystem::path path{};
    std::filesystem::file_time_type timestamp{};
};

class ENGINE_PLATFORM_API FilesystemWatcher
{
public:
    using path_type = std::filesystem::path;
    using Callback = std::function<void(const WatchEvent&)>;

    class WatchHandle
    {
    public:
        WatchHandle() = default;
        explicit WatchHandle(std::size_t value) noexcept : value_{value} {}

        [[nodiscard]] bool is_valid() const noexcept { return value_ != 0; }
        [[nodiscard]] std::size_t value() const noexcept { return value_; }

        friend bool operator==(const WatchHandle&, const WatchHandle&) = default;

    private:
        std::size_t value_{0};
    };

    [[nodiscard]] WatchHandle watch_file(path_type path, Callback callback);

    bool unwatch(WatchHandle handle);

    void clear();

    void poll();

private:
    struct WatchedFile
    {
        path_type path{};
        Callback callback{};
        std::filesystem::file_time_type last_write{};
        bool known_exists{false};
    };

    std::mutex mutex_{};
    std::size_t next_handle_{1};
    std::unordered_map<std::size_t, WatchedFile> watchers_{};
};

}  // namespace engine::platform::filesystem

