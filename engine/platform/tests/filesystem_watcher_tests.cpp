#include <gtest/gtest.h>

#include "engine/platform/filesystem/watcher.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    using namespace engine::platform::filesystem;

    class TempDirectory
    {
    public:
        TempDirectory()
        {
            const auto base = std::filesystem::temp_directory_path();
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = base / ("engine_platform_watcher_" + std::to_string(stamp));
            std::filesystem::create_directories(path_);
        }

        TempDirectory(const TempDirectory&) = delete;
        TempDirectory& operator=(const TempDirectory&) = delete;
        TempDirectory(TempDirectory&&) = delete;
        TempDirectory& operator=(TempDirectory&&) = delete;

        ~TempDirectory()
        {
            std::error_code ec{};
            std::filesystem::remove_all(path_, ec);
        }

        [[nodiscard]] std::filesystem::path path() const
        {
            return path_;
        }

    private:
        std::filesystem::path path_{};
    };

    void write_text_file(const std::filesystem::path& path, const std::string& contents)
    {
        std::ofstream stream(path, std::ios::out | std::ios::binary);
        stream << contents;
    }

    TEST(FilesystemWatcher, ReportsModifications)
    {
        TempDirectory directory;
        const auto file_path = directory.path() / "asset.txt";
        write_text_file(file_path, "initial");

        FilesystemWatcher watcher;
        std::vector<WatchEvent> events;
        const auto handle = watcher.watch_file(file_path, [&](const WatchEvent& event)
        {
            events.push_back(event);
        });
        EXPECT_TRUE(handle.is_valid());

        watcher.poll();
        EXPECT_TRUE(events.empty());

        const auto baseline = std::filesystem::last_write_time(file_path);
        std::filesystem::last_write_time(file_path, baseline + std::chrono::seconds(1));

        watcher.poll();
        ASSERT_EQ(events.size(), 1u);
        EXPECT_EQ(events.front().type, WatchEventType::modified);
        EXPECT_EQ(events.front().path, std::filesystem::absolute(file_path).lexically_normal());
    }

    TEST(FilesystemWatcher, DetectsTimestampCoalescedUpdates)
    {
        TempDirectory directory;
        const auto file_path = directory.path() / "asset.txt";
        write_text_file(file_path, "baseline");

        FilesystemWatcher watcher;
        std::vector<WatchEvent> events;
        [[maybe_unused]] const auto handle = watcher.watch_file(file_path, [&](const WatchEvent& event)
        {
            events.push_back(event);
        });

        watcher.poll();
        EXPECT_TRUE(events.empty());

        const auto baseline = std::filesystem::last_write_time(file_path);
        write_text_file(file_path, "baseline updated payload");
        std::filesystem::last_write_time(file_path, baseline);

        watcher.poll();
        ASSERT_EQ(events.size(), 1u);
        EXPECT_EQ(events.back().type, WatchEventType::modified);
        EXPECT_EQ(events.back().path, std::filesystem::absolute(file_path).lexically_normal());
    }

    TEST(FilesystemWatcher, TracksCreationAndDeletion)
    {
        TempDirectory directory;
        const auto file_path = directory.path() / "dynamic.txt";

        FilesystemWatcher watcher;
        std::vector<WatchEvent> events;
        [[maybe_unused]] const auto handle = watcher.watch_file(file_path, [&](const WatchEvent& event)
        {
            events.push_back(event);
        });

        watcher.poll();
        EXPECT_TRUE(events.empty());

        write_text_file(file_path, "payload");
        watcher.poll();
        ASSERT_EQ(events.size(), 1u);
        EXPECT_EQ(events.back().type, WatchEventType::created);
        EXPECT_EQ(events.back().path, std::filesystem::absolute(file_path).lexically_normal());

        events.clear();
        std::error_code ec{};
        std::filesystem::remove(file_path, ec);
        watcher.poll();
        ASSERT_EQ(events.size(), 1u);
        EXPECT_EQ(events.back().type, WatchEventType::erased);
    }

    TEST(FilesystemWatcher, SupportsUnwatchAndClear)
    {
        TempDirectory directory;
        const auto file_path = directory.path() / "transient.txt";
        write_text_file(file_path, "initial");

        FilesystemWatcher watcher;
        std::vector<WatchEvent> events;
        auto handle = watcher.watch_file(file_path, [&](const WatchEvent& event)
        {
            events.push_back(event);
        });
        ASSERT_TRUE(handle.is_valid());
        watcher.unwatch(handle);

        const auto baseline = std::filesystem::last_write_time(file_path);
        std::filesystem::last_write_time(file_path, baseline + std::chrono::seconds(1));

        watcher.poll();
        EXPECT_TRUE(events.empty());

        handle = watcher.watch_file(file_path, [&](const WatchEvent& event)
        {
            events.push_back(event);
        });
        watcher.clear();

        const auto updated = std::filesystem::last_write_time(file_path);
        std::filesystem::last_write_time(file_path, updated + std::chrono::seconds(1));
        watcher.poll();
        EXPECT_TRUE(events.empty());
    }
} // namespace