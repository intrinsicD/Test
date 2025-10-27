#include "engine/platform/filesystem/watcher.hpp"

#include <system_error>

namespace engine::platform::filesystem
{
    namespace
    {
        struct PathState
        {
            bool exists{false};
            std::filesystem::file_time_type last_write{};
        };

        FilesystemWatcher::path_type normalise_path(FilesystemWatcher::path_type path)
        {
            if (path.empty())
            {
                throw std::invalid_argument("FilesystemWatcher cannot watch an empty path");
            }

            std::error_code ec{};
            auto absolute = std::filesystem::absolute(path, ec);
            if (ec)
            {
                absolute = path;
            }

            return absolute.lexically_normal();
        }

        PathState capture_state_impl(const std::filesystem::path& path) noexcept
        {
            PathState state{};

            std::error_code ec{};
            state.exists = std::filesystem::exists(path, ec);
            if (!state.exists || ec)
            {
                state.exists = false;
                return state;
            }

            const auto last_write = std::filesystem::last_write_time(path, ec);
            if (ec)
            {
                state.exists = false;
                return state;
            }

            state.last_write = last_write;
            return state;
        }
    } // namespace

    FilesystemWatcher::WatchHandle FilesystemWatcher::watch_file(path_type path, Callback callback)
    {
        if (!callback)
        {
            throw std::invalid_argument("FilesystemWatcher requires a valid callback");
        }

        auto normalised = normalise_path(std::move(path));
        const auto state = capture_state_impl(normalised);

        std::scoped_lock lock{mutex_};
        const auto handle_value = next_handle_++;
        watchers_.emplace(handle_value,
                          WatchedFile{std::move(normalised), std::move(callback), state.last_write, state.exists});
        return WatchHandle{handle_value};
    }

    bool FilesystemWatcher::unwatch(WatchHandle handle)
    {
        if (!handle.is_valid())
        {
            return false;
        }

        std::scoped_lock lock{mutex_};
        return watchers_.erase(handle.value()) > 0;
    }

    void FilesystemWatcher::clear()
    {
        std::scoped_lock lock{mutex_};
        watchers_.clear();
    }

    void FilesystemWatcher::poll()
    {
        struct DispatchEntry
        {
            Callback callback;
            WatchEvent event;
        };

        std::vector<DispatchEntry> dispatch{};

        {
            std::scoped_lock lock{mutex_};
            dispatch.reserve(watchers_.size());
            for (auto& [handle, watched] : watchers_)
            {
                const auto state = capture_state_impl(watched.path);

                bool should_dispatch = false;
                WatchEventType type = WatchEventType::modified;

                if (!watched.known_exists && state.exists)
                {
                    type = WatchEventType::created;
                    should_dispatch = true;
                }
                else if (watched.known_exists && state.exists && watched.last_write != state.last_write)
                {
                    type = WatchEventType::modified;
                    should_dispatch = true;
                }
                else if (watched.known_exists && !state.exists)
                {
                    type = WatchEventType::erased;
                    should_dispatch = true;
                }

                watched.known_exists = state.exists;
                if (state.exists)
                {
                    watched.last_write = state.last_write;
                }

                if (should_dispatch)
                {
                    auto timestamp = state.exists ? state.last_write : std::filesystem::file_time_type{};
                    dispatch.push_back(DispatchEntry{watched.callback, WatchEvent{type, watched.path, timestamp}});
                }
            }
        }

        for (auto& entry : dispatch)
        {
            if (entry.callback)
            {
                entry.callback(entry.event);
            }
        }
    }
} // namespace engine::platform::filesystem