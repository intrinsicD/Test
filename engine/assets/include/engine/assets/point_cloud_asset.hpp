#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/detail/cache_common.hpp"
#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/point_cloud/point_cloud.hpp"

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
    struct PointCloudAssetDescriptor
    {
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

    struct PointCloudAsset
    {
        PointCloudAssetDescriptor descriptor{};
        geometry::PointCloud point_cloud{};
        io::GeometryDetectionResult detection{};
        std::filesystem::file_time_type last_write{};
    };

    class PointCloudCache : public detail::AssetCacheLifecycle<
                                PointCloudCache,
                                PointCloudAsset,
                                PointCloudAssetDescriptor,
                                PointCloudHandle,
                                PointCloudHandleTag,
                                std::function<void(const PointCloudAsset&)>,
                                true>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            PointCloudCache,
            PointCloudAsset,
            PointCloudAssetDescriptor,
            PointCloudHandle,
            PointCloudHandleTag,
            std::function<void(const PointCloudAsset&)>,
            true>;

        PointCloudCache();

        using HotReloadCallback = std::function<void(const PointCloudAsset&)>;

        using Base::for_each_asset;

        [[nodiscard]] const PointCloudAsset& load(const PointCloudAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const PointCloudHandle& handle) const;
        [[nodiscard]] const PointCloudAsset& get(const PointCloudHandle& handle) const;

        [[nodiscard]] AssetLoadFuture<PointCloudHandle> load_async(
            const AssetLoadRequest& request,
            core::threading::IoThreadPool& pool);
        [[nodiscard]] AssetLoadState async_state(std::string_view identifier) const;

        void unload(const PointCloudHandle& handle);
        void register_hot_reload_callback(const PointCloudHandle& handle, HotReloadCallback callback);
        void poll();

        using typename Base::RawHandle;

    protected:
        friend Base;
        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle,
                                                         PointCloudAsset& asset,
                                                         bool notify);

    private:
        AssetAsyncQueue<PointCloudHandle> async_queue_{};
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets