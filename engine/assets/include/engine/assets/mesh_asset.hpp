#pragma once

#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace engine::assets {

struct MeshAssetDescriptor {
    MeshHandle handle;
    std::filesystem::path source;
    io::MeshFileFormat format_hint{io::MeshFileFormat::unknown};

    [[nodiscard]] static MeshAssetDescriptor from_file(const std::filesystem::path& path,
                                                       io::MeshFileFormat hint = io::MeshFileFormat::unknown)
    {
        return MeshAssetDescriptor{MeshHandle{path}, path, hint};
    }
};

struct MeshAsset {
    MeshAssetDescriptor descriptor{};
    geometry::Mesh mesh{};
    io::GeometryDetectionResult detection{};
    std::filesystem::file_time_type last_write{};
};

class MeshCache {
public:
    using HotReloadCallback = std::function<void(const MeshAsset&)>;

    [[nodiscard]] const MeshAsset& load(const MeshAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const MeshHandle& handle) const;
    [[nodiscard]] const MeshAsset& get(const MeshHandle& handle) const;

    void unload(const MeshHandle& handle);
    void register_hot_reload_callback(const MeshHandle& handle, HotReloadCallback callback);
    void poll();

private:
    void reload_asset(const MeshHandle& handle, MeshAsset& asset, bool notify);

    std::unordered_map<MeshHandle, MeshAsset> assets_{};
    std::unordered_map<MeshHandle, std::vector<HotReloadCallback>> callbacks_{};
};

}  // namespace engine::assets

