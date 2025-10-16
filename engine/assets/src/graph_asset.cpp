#include "engine/assets/graph_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"

#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>

namespace engine::assets {

const GraphAsset& GraphCache::load(const GraphAssetDescriptor& descriptor)
{
    const auto identifier = descriptor.handle.id();
    if (identifier.empty()) {
        throw std::invalid_argument("Graph handle identifier cannot be empty");
    }

    GraphAsset* asset = nullptr;
    RawHandle handle{};
    bool inserted = false;

    const auto lookup = bindings_.find(identifier);
    if (lookup == bindings_.end()) {
        auto [acquired_handle, slot] = assets_.acquire();
        handle = acquired_handle;
        asset = &slot;
        bindings_.emplace(identifier, handle);
        inserted = true;
    } else {
        handle = lookup->second;
        asset = &assets_.get(handle);
    }

    asset->descriptor = descriptor;
    descriptor.handle.bind(handle);

    if (auto pending = pending_callbacks_.find(identifier); pending != pending_callbacks_.end()) {
        auto& target = callbacks_[handle];
        auto& pending_list = pending->second;
        target.insert(target.end(),
                      std::make_move_iterator(pending_list.begin()),
                      std::make_move_iterator(pending_list.end()));
        pending_callbacks_.erase(pending);
    }

    const auto current_write = detail::checked_last_write_time(descriptor.source, "graph");
    const bool needs_reload = inserted || asset->last_write != current_write;
    if (needs_reload) {
        reload_asset(handle, *asset, !inserted);
    }

    return *asset;
}

bool GraphCache::contains(const GraphHandle& handle) const
{
    return handle.is_valid(assets_);
}

const GraphAsset& GraphCache::get(const GraphHandle& handle) const
{
    if (!handle.is_valid(assets_)) {
        throw std::out_of_range("Graph asset handle not found");
    }
    return assets_.get(handle.raw_handle());
}

void GraphCache::unload(const GraphHandle& handle)
{
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

void GraphCache::register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback)
{
    if (handle.is_bound() && handle.is_valid(assets_)) {
        callbacks_[handle.raw_handle()].push_back(std::move(callback));
        return;
    }

    if (handle.id().empty()) {
        throw std::invalid_argument("Graph handle identifier cannot be empty");
    }

    pending_callbacks_[handle.id()].push_back(std::move(callback));
}

void GraphCache::poll()
{
    assets_.for_each([&](const RawHandle& handle, GraphAsset& asset) {
        const auto current_write =
            detail::checked_last_write_time(asset.descriptor.source, "graph");
        if (current_write != asset.last_write) {
            reload_asset(handle, asset, true);
        }
    });
}

void GraphCache::reload_asset(const RawHandle& handle, GraphAsset& asset, bool notify)
{
    const auto detection_result = io::detect_geometry_file(asset.descriptor.source);
    if (!detection_result) {
        throw std::runtime_error("Geometry file detection failed: " +
                                 std::string(detection_result.error().message()));
    }

    const auto& detection = detection_result.value();
    if (detection.kind != io::GeometryKind::graph) {
        throw std::runtime_error("Geometry file does not describe a graph");
    }

    const io::GraphFileFormat format = asset.descriptor.format_hint != io::GraphFileFormat::unknown
                                           ? asset.descriptor.format_hint
                                           : detection.graph_format;

    if (format == io::GraphFileFormat::unknown) {
        throw std::runtime_error("Unable to determine graph file format for asset");
    }

    asset.graph.interface.clear();
    if (auto result = io::read_graph(asset.descriptor.source, asset.graph.interface, format); !result) {
        throw std::runtime_error("Failed to read graph: " + std::string(result.error().message()));
    }
    asset.detection = detection;
    asset.last_write = detail::checked_last_write_time(asset.descriptor.source, "graph");

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

