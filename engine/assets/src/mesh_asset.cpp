#include "engine/assets/mesh_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"

#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>

namespace engine::assets {

// TODO(engine-assets): Consolidate duplicated cache lifecycle logic across asset caches.

const MeshAsset& MeshCache::load(const MeshAssetDescriptor& descriptor)
{
    std::scoped_lock lock{mutex_};
    const auto identifier = descriptor.handle.id();
    if (identifier.empty())
    {
        throw std::invalid_argument("Mesh handle identifier cannot be empty");
    }

    MeshAsset* asset = nullptr;
    RawHandle handle{};
    bool inserted = false;

    const auto lookup = bindings_.find(identifier);
    if (lookup == bindings_.end())
    {
        auto [acquired_handle, slot] = assets_.acquire();
        handle = acquired_handle;
        asset = &slot;
        bindings_.emplace(identifier, handle);
        inserted = true;
    }
    else
    {
        handle = lookup->second;
        asset = &assets_.get(handle);
    }

    asset->descriptor = descriptor;
    descriptor.handle.bind(handle);

    if (auto pending = pending_callbacks_.find(identifier); pending != pending_callbacks_.end())
    {
        auto& target = callbacks_[handle];
        auto& pending_list = pending->second;
        target.insert(target.end(),
                      std::make_move_iterator(pending_list.begin()),
                      std::make_move_iterator(pending_list.end()));
        pending_callbacks_.erase(pending);
    }

    const auto current_write = detail::checked_last_write_time(descriptor.source, "mesh");
    const bool needs_reload = inserted || asset->last_write != current_write;
    if (needs_reload)
    {
        reload_asset(handle, *asset, !inserted);
    }

    return *asset;
}

bool MeshCache::contains(const MeshHandle& handle) const
{
    std::scoped_lock lock{mutex_};
    return handle.is_valid(assets_);
}

const MeshAsset& MeshCache::get(const MeshHandle& handle) const
{
    std::scoped_lock lock{mutex_};
    if (!handle.is_valid(assets_)) {
        throw std::out_of_range("Mesh asset handle not found");
    }
    return assets_.get(handle.raw_handle());
}

void MeshCache::unload(const MeshHandle& handle)
{
    std::scoped_lock lock{mutex_};
    if (!handle.is_bound()) {
        return;
    }

    const auto raw = handle.raw_handle();
    if (!assets_.is_valid(raw)) {
        handle.reset_binding();
        return;
    }

    const auto identifier = assets_.get(raw).descriptor.handle.id();

    if (auto cb_it = callbacks_.find(raw); cb_it != callbacks_.end()) {
        if (!identifier.empty()) {
            auto& pending = pending_callbacks_[identifier];
            pending.insert(pending.end(),
                           std::make_move_iterator(cb_it->second.begin()),
                           std::make_move_iterator(cb_it->second.end()));
        }
        callbacks_.erase(cb_it);
    }

    assets_.release(raw);
    bindings_.erase(identifier);
    handle.reset_binding();
}

void MeshCache::register_hot_reload_callback(const MeshHandle& handle, HotReloadCallback callback)
{
    std::scoped_lock lock{mutex_};
    if (handle.is_bound() && handle.is_valid(assets_)) {
        callbacks_[handle.raw_handle()].push_back(std::move(callback));
        return;
    }

    if (handle.id().empty()) {
        throw std::invalid_argument("Mesh handle identifier cannot be empty");
    }

    pending_callbacks_[handle.id()].push_back(std::move(callback));
}

void MeshCache::poll()
{
    std::scoped_lock lock{mutex_};
    assets_.for_each([&](const RawHandle& handle, MeshAsset& asset) {
        const auto current_write =
            detail::checked_last_write_time(asset.descriptor.source, "mesh");
        if (current_write != asset.last_write) {
            reload_asset(handle, asset, true);
        }
    });
}

AssetLoadFuture<MeshHandle> MeshCache::load_async(const AssetLoadRequest& request,
                                                  core::threading::IoThreadPool& pool)
{
    if (request.identifier.empty())
    {
        throw std::invalid_argument("Asset load request identifier cannot be empty");
    }

    MeshAssetDescriptor descriptor{};
    descriptor.handle = MeshHandle{request.identifier};
    descriptor.source = std::filesystem::path{request.identifier};

    return async_queue_.schedule(
        request.identifier,
        request.priority,
        request.allow_blocking_fallback,
        [this, descriptor](detail::AssetLoadPromise<MeshHandle>& promise) -> AssetLoadResult<MeshHandle> {
            (void)promise;
            try
            {
                const auto& asset = this->load(descriptor);
                return AssetLoadResult<MeshHandle>{asset.descriptor.handle};
            }
            catch (const std::exception& ex)
            {
                return AssetLoadResult<MeshHandle>{
                    make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what())};
            }
        },
        pool);
}

AssetLoadState MeshCache::async_state(std::string_view identifier) const
{
    return async_queue_.state(identifier);
}

void MeshCache::reload_asset(const RawHandle& handle, MeshAsset& asset, bool notify)
{
    // mutex_ is expected to be held by the caller.
    const auto detection_result = io::detect_geometry_file(asset.descriptor.source);
    if (!detection_result) {
        throw std::runtime_error("Geometry file detection failed: " +
                                 std::string(detection_result.error().message()));
    }

    const auto& detection = detection_result.value();
    if (detection.kind != io::GeometryKind::mesh) {
        throw std::runtime_error("Geometry file does not describe a mesh");
    }

    const io::MeshFileFormat format = asset.descriptor.format_hint != io::MeshFileFormat::unknown
                                          ? asset.descriptor.format_hint
                                          : detection.mesh_format;

    if (format == io::MeshFileFormat::unknown) {
        throw std::runtime_error("Unable to determine mesh file format for asset");
    }

    asset.mesh.interface.clear();
    if (auto result = io::read_mesh(asset.descriptor.source, asset.mesh.interface, format); !result) {
        throw std::runtime_error("Failed to read mesh: " + std::string(result.error().message()));
    }
    asset.detection = detection;
    asset.last_write = detail::checked_last_write_time(asset.descriptor.source, "mesh");

    if (notify) {
        const auto cb_it = callbacks_.find(handle);
        if (cb_it != callbacks_.end()) {
            for (const auto& callback : cb_it->second) {
                callback(asset);
            }
        }
    }
}

}  // namespace engine::assets

