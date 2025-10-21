#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/graph/graph.hpp"

#include "engine/core/memory/resource_pool.hpp"
#include "engine/platform/filesystem/watcher.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets {

struct GraphAssetDescriptor {
    GraphHandle handle{};
    std::filesystem::path source{};
    io::GraphFileFormat format_hint{io::GraphFileFormat::unknown};

    [[nodiscard]] static GraphAssetDescriptor from_file(const std::filesystem::path& path,
                                                        io::GraphFileFormat hint = io::GraphFileFormat::unknown)
    {
        return GraphAssetDescriptor{GraphHandle{path}, path, hint};
    }
};

struct GraphAsset {
    GraphAssetDescriptor descriptor{};
    geometry::Graph graph{};
    io::GeometryDetectionResult detection{};
    std::filesystem::file_time_type last_write{};
};

class GraphCache {
public:
    GraphCache();

    using HotReloadCallback = std::function<void(const GraphAsset&)>;

    [[nodiscard]] const GraphAsset& load(const GraphAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const GraphHandle& handle) const;
    [[nodiscard]] const GraphAsset& get(const GraphHandle& handle) const;

    void unload(const GraphHandle& handle);
    void register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback);
    void poll();

private:
    using Pool = core::memory::ResourcePool<GraphAsset, GraphHandleTag>;
    using RawHandle = typename Pool::handle_type;
    using HandleHasher = typename Pool::handle_hasher;

    engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, GraphAsset& asset, bool notify);
    void register_watch_locked(const RawHandle& handle, GraphAsset& asset);
    void unregister_watch_locked(const RawHandle& handle);

    Pool assets_{};
    std::unordered_map<std::string, RawHandle> bindings_{};
    std::unordered_map<std::string, std::vector<HotReloadCallback>> pending_callbacks_{};
    std::unordered_map<RawHandle, std::vector<HotReloadCallback>, HandleHasher> callbacks_{};
    std::unordered_map<RawHandle, platform::filesystem::FilesystemWatcher::WatchHandle, HandleHasher> watch_handles_{};
    platform::filesystem::FilesystemWatcher watcher_{};
    mutable std::mutex mutex_{};
    std::shared_ptr<void> handle_validator_registration_{};
};

}  // namespace engine::assets

