#pragma once

#include "engine/platform/api.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

namespace engine::platform::filesystem
{
    /// \brief Provides sandboxed access to a directory on the host filesystem.
    class ENGINE_PLATFORM_API Filesystem
    {
    public:
        using path_type = std::filesystem::path;

        /// \brief Constructs a filesystem rooted at the given directory.
        explicit Filesystem(path_type root);

        /// \brief Returns the canonicalised root path used to scope lookups.
        [[nodiscard]] const path_type& root() const noexcept;

        /// \brief Resolves a relative path into the sandbox. Returns std::nullopt if
        /// the candidate escapes the root.
        [[nodiscard]] std::optional<path_type> try_resolve(std::string_view path) const noexcept;

        /// \brief Checks whether a file or directory exists within the sandbox.
        [[nodiscard]] bool exists(std::string_view path) const noexcept;

        /// \brief Checks whether the resolved path refers to a regular file.
        [[nodiscard]] bool is_file(std::string_view path) const noexcept;

        /// \brief Checks whether the resolved path refers to a directory.
        [[nodiscard]] bool is_directory(std::string_view path) const noexcept;

        /// \brief Reads an entire file as binary data. Returns std::nullopt on
        /// failure or when the path resolves outside the sandbox.
        [[nodiscard]] std::optional<std::vector<std::byte>> read_binary(std::string_view path) const;

        /// \brief Reads an entire file as text. Returns std::nullopt on failure or
        /// when the path resolves outside the sandbox.
        [[nodiscard]] std::optional<std::string> read_text(std::string_view path) const;

    private:
        [[nodiscard]] bool contains(const path_type& path) const noexcept;

        path_type root_;
    };

    /// \brief Aggregates multiple filesystem providers under mount aliases.
    class ENGINE_PLATFORM_API VirtualFilesystem
    {
    public:
        using path_type = Filesystem::path_type;

        /// \brief Mounts the given filesystem under the provided alias.
        /// Existing mounts with the same alias are replaced. Returns true when the
        /// alias is valid and the mount was stored.
        bool mount(std::string alias, Filesystem filesystem);

        /// \brief Removes the filesystem mounted at the given alias.
        bool unmount(std::string_view alias) noexcept;

        /// \brief Checks whether an alias is currently mounted.
        [[nodiscard]] bool is_mounted(std::string_view alias) const noexcept;

        /// \brief Checks whether the provided virtual path resolves to an existing
        /// entry in one of the mounted filesystems.
        [[nodiscard]] bool exists(std::string_view virtual_path) const noexcept;

        /// \brief Reads a file from one of the mounts as binary data.
        [[nodiscard]] std::optional<std::vector<std::byte>> read_binary(std::string_view virtual_path) const;

        /// \brief Reads a file from one of the mounts as text data.
        [[nodiscard]] std::optional<std::string> read_text(std::string_view virtual_path) const;

    private:
        using MountMap = std::unordered_map<std::string, Filesystem>;

        [[nodiscard]] const Filesystem* find_mount(std::string_view alias) const;

        static std::optional<std::pair<std::string_view, std::string_view>>
        split_virtual_path(std::string_view virtual_path) noexcept;

        MountMap mounts_{};
    };

    std::string generate_random_suffix(); //TODO refactor this? remove it? its needed in multiple tests...
} // namespace engine::platform::filesystem
