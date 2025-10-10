#include "engine/assets/point_cloud_asset.hpp"

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
                throw std::runtime_error("Failed to query point cloud asset timestamp: " + ec.message());
            }
            return time;
        }
    } // namespace

    const PointCloudAsset& PointCloudCache::load(const PointCloudAssetDescriptor& descriptor)
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

    bool PointCloudCache::contains(const PointCloudHandle& handle) const
    {
        return assets_.find(handle) != assets_.end();
    }

    const PointCloudAsset& PointCloudCache::get(const PointCloudHandle& handle) const
    {
        const auto it = assets_.find(handle);
        if (it == assets_.end())
        {
            throw std::out_of_range("Point cloud asset handle not found");
        }
        return it->second;
    }

    void PointCloudCache::unload(const PointCloudHandle& handle)
    {
        assets_.erase(handle);
        callbacks_.erase(handle);
    }

    void PointCloudCache::register_hot_reload_callback(const PointCloudHandle& handle, HotReloadCallback callback)
    {
        callbacks_[handle].push_back(std::move(callback));
    }

    void PointCloudCache::poll()
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

    void PointCloudCache::reload_asset(const PointCloudHandle& handle, PointCloudAsset& asset, bool notify)
    {
        const auto detection = io::detect_geometry_file(asset.descriptor.source);
        if (detection.kind != io::GeometryKind::point_cloud)
        {
            throw std::runtime_error("Geometry file does not describe a point cloud");
        }

        const io::PointCloudFileFormat format = asset.descriptor.format_hint != io::PointCloudFileFormat::unknown
                                                    ? asset.descriptor.format_hint
                                                    : detection.point_cloud_format;

        if (format == io::PointCloudFileFormat::unknown)
        {
            throw std::runtime_error("Unable to determine point cloud file format for asset");
        }

        asset.point_cloud.interface.clear();
        io::read_point_cloud(asset.descriptor.source, asset.point_cloud.interface, format);
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

