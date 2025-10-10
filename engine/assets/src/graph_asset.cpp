#include "engine/assets/graph_asset.hpp"

#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] std::filesystem::file_time_type safe_last_write_time(const std::filesystem::path& path)
        {
            std::error_code ec{};
            const auto time = std::filesystem::last_write_time(path, ec);
            if (ec)
            {
                throw std::runtime_error("Failed to query graph asset timestamp: " + ec.message());
            }
            return time;
        }
    } // namespace

    const GraphAsset& GraphCache::load(const GraphAssetDescriptor& descriptor)
    {
        auto [it, inserted] = assets_.try_emplace(descriptor.handle);
        auto& asset = it->second;
        asset.descriptor = descriptor;

        const auto current_write = safe_last_write_time(descriptor.source);
        const bool needs_reload = inserted || asset.last_write != current_write;
        if (needs_reload)
        {
            reload_asset(descriptor.handle, asset, !inserted);
        }

        return asset;
    }

    bool GraphCache::contains(const GraphHandle& handle) const
    {
        return assets_.find(handle) != assets_.end();
    }

    const GraphAsset& GraphCache::get(const GraphHandle& handle) const
    {
        const auto it = assets_.find(handle);
        if (it == assets_.end())
        {
            throw std::out_of_range("Graph asset handle not found");
        }
        return it->second;
    }

    void GraphCache::unload(const GraphHandle& handle)
    {
        assets_.erase(handle);
        callbacks_.erase(handle);
    }

    void GraphCache::register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback)
    {
        callbacks_[handle].push_back(std::move(callback));
    }

    void GraphCache::poll()
    {
        for (auto& [handle, asset] : assets_)
        {
            const auto current_write = safe_last_write_time(asset.descriptor.source);
            if (current_write != asset.last_write)
            {
                reload_asset(handle, asset, true);
            }
        }
    }

    void GraphCache::reload_asset(const GraphHandle& handle, GraphAsset& asset, bool notify)
    {
        const auto detection = io::detect_geometry_file(asset.descriptor.source);
        if (detection.kind != io::GeometryKind::graph)
        {
            throw std::runtime_error("Geometry file does not describe a graph");
        }

        const io::GraphFileFormat format = asset.descriptor.format_hint != io::GraphFileFormat::unknown
                                               ? asset.descriptor.format_hint
                                               : detection.graph_format;

        if (format == io::GraphFileFormat::unknown)
        {
            throw std::runtime_error("Unable to determine graph file format for asset");
        }

        asset.graph.interface.clear();
        io::read_graph(asset.descriptor.source, asset.graph.interface, format);
        asset.detection = detection;
        asset.last_write = safe_last_write_time(asset.descriptor.source);

        if (notify)
        {
            const auto cb_it = callbacks_.find(handle);
            if (cb_it != callbacks_.end())
            {
                for (const auto& callback : cb_it->second)
                {
                    callback(asset);
                }
            }
        }
    }
} // namespace engine::assets

