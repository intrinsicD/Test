#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/detail/cache_common.hpp"
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

namespace engine::assets
{
    struct GraphAssetDescriptor
    {
        GraphHandle handle{};
        std::filesystem::path source{};
        io::GraphFileFormat format_hint{io::GraphFileFormat::unknown};

        [[nodiscard]] static GraphAssetDescriptor from_file(const std::filesystem::path& path,
                                                            io::GraphFileFormat hint = io::GraphFileFormat::unknown)
        {
            return GraphAssetDescriptor{GraphHandle{path}, path, hint};
        }
    };

    struct GraphAsset
    {
        GraphAssetDescriptor descriptor{};
        geometry::Graph graph{};
        io::GeometryDetectionResult detection{};
        std::filesystem::file_time_type last_write{};
    };

    class GraphCache : public detail::AssetCacheLifecycle<
                           GraphCache,
                           GraphAsset,
                           GraphAssetDescriptor,
                           GraphHandle,
                           GraphHandleTag,
                           std::function<void(const GraphAsset&)>,
                           true>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            GraphCache,
            GraphAsset,
            GraphAssetDescriptor,
            GraphHandle,
            GraphHandleTag,
            std::function<void(const GraphAsset&)>,
            true>;

        GraphCache();

        using HotReloadCallback = std::function<void(const GraphAsset&)>;

        using Base::for_each_asset;

        [[nodiscard]] const GraphAsset& load(const GraphAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const GraphHandle& handle) const;
        [[nodiscard]] const GraphAsset& get(const GraphHandle& handle) const;

        void unload(const GraphHandle& handle);
        void register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback);
        void poll();

        using typename Base::RawHandle;

    protected:
        friend Base;
        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, GraphAsset& asset, bool notify);

    private:
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets