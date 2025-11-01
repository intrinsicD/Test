#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/detail/cache_common.hpp"
#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/mesh/halfedge_mesh.hpp"

#include "engine/core/memory/resource_pool.hpp"
#include "engine/platform/filesystem/watcher.hpp"

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine::assets
{
    struct MeshAssetDescriptor
    {
        MeshHandle handle;
        std::filesystem::path source;
        io::MeshFileFormat format_hint{io::MeshFileFormat::unknown};

        [[nodiscard]] static MeshAssetDescriptor from_file(const std::filesystem::path& path,
                                                           io::MeshFileFormat hint = io::MeshFileFormat::unknown)
        {
            return MeshAssetDescriptor{MeshHandle{path}, path, hint};
        }
    };

    struct MeshAsset
    {
        MeshAssetDescriptor descriptor{};
        geometry::Mesh mesh{};
        io::GeometryDetectionResult detection{};
        std::filesystem::file_time_type last_write{};
    };

    class MeshCache : public detail::AssetCacheLifecycle<
                          MeshCache,
                          MeshAsset,
                          MeshAssetDescriptor,
                          MeshHandle,
                          MeshHandleTag,
                          std::function<void(const MeshAsset&)>,
                          true>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            MeshCache,
            MeshAsset,
            MeshAssetDescriptor,
            MeshHandle,
            MeshHandleTag,
            std::function<void(const MeshAsset&)>,
            true>;

        MeshCache();

        using HotReloadCallback = std::function<void(const MeshAsset&)>;

        [[nodiscard]] const MeshAsset& load(const MeshAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const MeshHandle& handle) const;
        [[nodiscard]] const MeshAsset& get(const MeshHandle& handle) const;

        [[nodiscard]] AssetLoadFuture<MeshHandle> load_async(
            const AssetLoadRequest& request,
            core::threading::IoThreadPool& pool);
        [[nodiscard]] AssetLoadState async_state(std::string_view identifier) const;

        void unload(const MeshHandle& handle);
        void register_hot_reload_callback(const MeshHandle& handle, HotReloadCallback callback);
        void poll();

        using typename Base::RawHandle;

    protected:
        friend Base;
        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, MeshAsset& asset, bool notify);

    private:
        AssetAsyncQueue<MeshHandle> async_queue_{};
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets