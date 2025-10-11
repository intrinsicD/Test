#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace engine::assets {

/// Strongly typed identifier used to reference assets stored in caches.
template <typename Tag>
class AssetHandle {
public:
    using tag_type = Tag;

    AssetHandle() = default;

    explicit AssetHandle(std::string identifier) : identifier_(std::move(identifier)) {}

    explicit AssetHandle(const std::filesystem::path& path)
        : identifier_(path.generic_string()) {}

    [[nodiscard]] const std::string& id() const noexcept { return identifier_; }

    [[nodiscard]] bool empty() const noexcept { return identifier_.empty(); }

    explicit operator bool() const noexcept { return !empty(); }

    friend bool operator==(const AssetHandle& lhs, const AssetHandle& rhs) noexcept {
        return lhs.identifier_ == rhs.identifier_;
    }

    friend bool operator!=(const AssetHandle& lhs, const AssetHandle& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool operator<(const AssetHandle& lhs, const AssetHandle& rhs) noexcept {
        return lhs.identifier_ < rhs.identifier_;
    }

private:
    std::string identifier_{};
};

struct MeshTag;
struct GraphTag;
struct PointCloudTag;
struct TextureTag;
struct ShaderTag;
struct MaterialTag;

using MeshHandle = AssetHandle<MeshTag>;
using GraphHandle = AssetHandle<GraphTag>;
using PointCloudHandle = AssetHandle<PointCloudTag>;
using TextureHandle = AssetHandle<TextureTag>;
using ShaderHandle = AssetHandle<ShaderTag>;
using MaterialHandle = AssetHandle<MaterialTag>;

}  // namespace engine::assets

namespace std {

template <typename Tag>
struct hash<engine::assets::AssetHandle<Tag>> {
    std::size_t operator()(const engine::assets::AssetHandle<Tag>& handle) const noexcept {
        return std::hash<std::string>()(handle.id());
    }
};

}  // namespace std

