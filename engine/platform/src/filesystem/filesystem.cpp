#include "engine/platform/filesystem/filesystem.hpp"

#include <fstream>
#include <system_error>

namespace engine::platform::filesystem {

namespace {

/// \brief Normalises the configured root to an absolute, lexical path.
[[nodiscard]] Filesystem::path_type normalise_root(Filesystem::path_type root) {
    if (root.empty()) {
        root = std::filesystem::current_path();
    }
    root = std::filesystem::absolute(root);
    return root.lexically_normal();
}

}  // namespace

Filesystem::Filesystem(path_type root)
    : root_{normalise_root(std::move(root))} {}

const Filesystem::path_type& Filesystem::root() const noexcept {
    return root_;
}

std::optional<Filesystem::path_type> Filesystem::try_resolve(std::string_view path) const noexcept {
    path_type candidate{path};
    path_type resolved = candidate.is_absolute() ? candidate : root_ / candidate;
    resolved = resolved.lexically_normal();
    if (!contains(resolved)) {
        return std::nullopt;
    }
    return resolved;
}

bool Filesystem::exists(std::string_view path) const noexcept {
    const auto resolved = try_resolve(path);
    if (!resolved) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(*resolved, ec);
}

bool Filesystem::is_file(std::string_view path) const noexcept {
    const auto resolved = try_resolve(path);
    if (!resolved) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_regular_file(*resolved, ec);
}

bool Filesystem::is_directory(std::string_view path) const noexcept {
    const auto resolved = try_resolve(path);
    if (!resolved) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_directory(*resolved, ec);
}

std::optional<std::vector<std::byte>> Filesystem::read_binary(std::string_view path) const {
    const auto resolved = try_resolve(path);
    if (!resolved) {
        return std::nullopt;
    }

    std::error_code ec;
    if (!std::filesystem::is_regular_file(*resolved, ec)) {
        return std::nullopt;
    }

    const auto file_size = std::filesystem::file_size(*resolved, ec);
    if (ec) {
        return std::nullopt;
    }

    std::vector<std::byte> data(static_cast<std::size_t>(file_size));
    std::ifstream stream(*resolved, std::ios::binary);
    if (!stream) {
        return std::nullopt;
    }

    if (!data.empty()) {
        stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!stream) {
            return std::nullopt;
        }
    }

    return data;
}

std::optional<std::string> Filesystem::read_text(std::string_view path) const {
    const auto resolved = try_resolve(path);
    if (!resolved) {
        return std::nullopt;
    }

    std::error_code ec;
    if (!std::filesystem::is_regular_file(*resolved, ec)) {
        return std::nullopt;
    }

    std::ifstream stream(*resolved, std::ios::in | std::ios::binary);
    if (!stream) {
        return std::nullopt;
    }

    stream.seekg(0, std::ios::end);
    const auto length = stream.tellg();
    if (length < 0) {
        return std::nullopt;
    }

    std::string contents(static_cast<std::size_t>(length), '\0');
    stream.seekg(0, std::ios::beg);
    stream.read(contents.data(), static_cast<std::streamsize>(length));
    if (!stream) {
        return std::nullopt;
    }

    return contents;
}

bool Filesystem::contains(const path_type& path) const noexcept {
    auto root_iter = root_.begin();
    auto path_iter = path.begin();
    for (; root_iter != root_.end(); ++root_iter, ++path_iter) {
        if (path_iter == path.end() || *root_iter != *path_iter) {
            return false;
        }
    }
    return true;
}

bool VirtualFilesystem::mount(std::string alias, Filesystem filesystem) {
    if (alias.empty()) {
        return false;
    }

    if (!alias.empty() && alias.back() == '/') {
        alias.pop_back();
    }

    mounts_.insert_or_assign(std::move(alias), std::move(filesystem));
    return true;
}

bool VirtualFilesystem::unmount(std::string_view alias) noexcept {
    return mounts_.erase(std::string{alias}) > 0;
}

bool VirtualFilesystem::is_mounted(std::string_view alias) const noexcept {
    return mounts_.find(std::string{alias}) != mounts_.end();
}

bool VirtualFilesystem::exists(std::string_view virtual_path) const noexcept {
    const auto split = split_virtual_path(virtual_path);
    if (!split) {
        return false;
    }
    const auto* mount = find_mount(split->first);
    if (mount == nullptr) {
        return false;
    }
    return mount->exists(split->second);
}

std::optional<std::vector<std::byte>> VirtualFilesystem::read_binary(std::string_view virtual_path) const {
    const auto split = split_virtual_path(virtual_path);
    if (!split) {
        return std::nullopt;
    }
    const auto* mount = find_mount(split->first);
    if (mount == nullptr) {
        return std::nullopt;
    }
    return mount->read_binary(split->second);
}

std::optional<std::string> VirtualFilesystem::read_text(std::string_view virtual_path) const {
    const auto split = split_virtual_path(virtual_path);
    if (!split) {
        return std::nullopt;
    }
    const auto* mount = find_mount(split->first);
    if (mount == nullptr) {
        return std::nullopt;
    }
    return mount->read_text(split->second);
}

const Filesystem* VirtualFilesystem::find_mount(std::string_view alias) const {
    const auto it = mounts_.find(std::string{alias});
    if (it == mounts_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::optional<std::pair<std::string_view, std::string_view>>
VirtualFilesystem::split_virtual_path(std::string_view virtual_path) noexcept {
    const auto separator = virtual_path.find(':');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    const auto alias = virtual_path.substr(0, separator);
    if (alias.empty()) {
        return std::nullopt;
    }

    if (separator + 1 >= virtual_path.size() || virtual_path[separator + 1] != '/') {
        return std::nullopt;
    }

    auto subpath = virtual_path.substr(separator + 2);
    if (subpath.empty()) {
        subpath = std::string_view{"."};
    }

    return std::pair<std::string_view, std::string_view>{alias, subpath};
}

}  // namespace engine::platform::filesystem

