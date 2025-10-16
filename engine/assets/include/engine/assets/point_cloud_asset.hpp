#pragma once

#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/point_cloud/point_cloud.hpp"

#include "engine/core/memory/resource_pool.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets {

struct PointCloudAssetDescriptor {
    PointCloudHandle handle{};
    std::filesystem::path source{};
    io::PointCloudFileFormat format_hint{io::PointCloudFileFormat::unknown};

    [[nodiscard]] static PointCloudAssetDescriptor from_file(
        const std::filesystem::path& path,
        io::PointCloudFileFormat hint = io::PointCloudFileFormat::unknown)
    {
        return PointCloudAssetDescriptor{PointCloudHandle{path}, path, hint};
    }
};

struct PointCloudAsset {
    PointCloudAssetDescriptor descriptor{};
    geometry::PointCloud point_cloud{};
    io::GeometryDetectionResult detection{};
    std::filesystem::file_time_type last_write{};
};

class PointCloudCache {
public:
    using HotReloadCallback = std::function<void(const PointCloudAsset&)>;

    [[nodiscard]] const PointCloudAsset& load(const PointCloudAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const PointCloudHandle& handle) const;
    [[nodiscard]] const PointCloudAsset& get(const PointCloudHandle& handle) const;

    void unload(const PointCloudHandle& handle);
    void register_hot_reload_callback(const PointCloudHandle& handle, HotReloadCallback callback);
    void poll();

private:
    using Pool = core::memory::ResourcePool<PointCloudAsset, PointCloudHandleTag>;
    using RawHandle = typename Pool::handle_type;
    using HandleHasher = typename Pool::handle_hasher;

    void reload_asset(const RawHandle& handle, PointCloudAsset& asset, bool notify);

    Pool assets_{};
    std::unordered_map<std::string, RawHandle> bindings_{};
    std::unordered_map<std::string, std::vector<HotReloadCallback>> pending_callbacks_{};
    std::unordered_map<RawHandle, std::vector<HotReloadCallback>, HandleHasher> callbacks_{};
};

}  // namespace engine::assets

