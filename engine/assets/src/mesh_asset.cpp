#include "engine/assets/mesh_asset.hpp"

#include <filesystem>
#include <stdexcept>
#include <system_error>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] std::filesystem::file_time_type safe_last_write_time(const std::filesystem::path& path)
        {
            std::error_code ec;
            const auto time = std::filesystem::last_write_time(path, ec);
            if (ec)
            {
                throw std::runtime_error("Failed to query mesh asset timestamp: " + ec.message());
            }
            return time;
        }
    } // namespace

    const MeshAsset& MeshCache::load(const MeshAssetDescriptor& descriptor)
    {
        auto [it, inserted] = assets_.try_emplace(descriptor.handle);
        MeshAsset& asset = it->second;
        asset.descriptor = descriptor;

        const auto current_write = safe_last_write_time(descriptor.source);
        const bool needs_reload = inserted || asset.last_write != current_write;
        if (needs_reload)
        {
            reload_asset(descriptor.handle, asset, !inserted);
        }

        return asset;
    }

    bool MeshCache::contains(const MeshHandle& handle) const
    {
        return assets_.find(handle) != assets_.end();
    }

    const MeshAsset& MeshCache::get(const MeshHandle& handle) const
    {
        const auto it = assets_.find(handle);
        if (it == assets_.end())
        {
            throw std::out_of_range("Mesh asset handle not found");
        }
        return it->second;
    }

    void MeshCache::unload(const MeshHandle& handle)
    {
        assets_.erase(handle);
        callbacks_.erase(handle);
    }

    void MeshCache::register_hot_reload_callback(const MeshHandle& handle, HotReloadCallback callback)
    {
        callbacks_[handle].push_back(std::move(callback));
    }

    void MeshCache::poll()
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

    void MeshCache::reload_asset(const MeshHandle& handle, MeshAsset& asset, bool notify)
    {
        const auto detection = io::detect_geometry_file(asset.descriptor.source);
        if (detection.kind != io::GeometryKind::mesh)
        {
            throw std::runtime_error("Geometry file does not describe a mesh");
        }

        const io::MeshFileFormat format = asset.descriptor.format_hint != io::MeshFileFormat::unknown
                                              ? asset.descriptor.format_hint
                                              : detection.mesh_format;

        if (format == io::MeshFileFormat::unknown)
        {
            throw std::runtime_error("Unable to determine mesh file format for asset");
        }

        asset.mesh.interface.clear();
        io::read_mesh(asset.descriptor.source, asset.mesh.interface, format);
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

