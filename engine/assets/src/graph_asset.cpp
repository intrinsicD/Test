#include "engine/assets/graph_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/detail/reload_utils.hpp"
#include "engine/assets/validation.hpp"

#include <cassert>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>

namespace engine::assets
{
    GraphCache::GraphCache()
        : Base(detail::AssetCacheLabels{
              "Graph",
              "graph",
              "GraphHandle",
              "GraphCache"})
        , handle_validator_registration_(HandleValidatorRegistry::instance().register_graph_validator(
            [this](const GraphHandle& handle)
            {
                std::scoped_lock lock{this->mutex_};
                return handle.is_valid(this->assets_);
            }))
    {
    }

    const GraphAsset& GraphCache::load(const GraphAssetDescriptor& descriptor)
    {
        std::scoped_lock lock{this->mutex_};

        auto acquisition = this->acquire_asset_slot(descriptor);
        this->bind_descriptor(descriptor, acquisition.handle, *acquisition.asset);
        this->merge_pending_callbacks(acquisition.identifier, acquisition.handle);

        const auto decision = this->evaluate_reload(descriptor, *acquisition.asset, acquisition.inserted);
        if (decision.should_reload)
        {
            if (auto reload = reload_asset(acquisition.handle, *acquisition.asset, !acquisition.inserted); !reload.has_value())
            {
                const auto message = reload.error().message();
                throw std::runtime_error(message.empty()
                                             ? std::string{to_string(reload.error().code())}
                                             : std::string{message});
            }
        }

        this->register_watch_locked(acquisition.handle, *acquisition.asset);

        return *acquisition.asset;
    }

    bool GraphCache::contains(const GraphHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->contains_handle(handle);
    }

    const GraphAsset& GraphCache::get(const GraphHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->get_asset_checked(handle);
    }

    void GraphCache::unload(const GraphHandle& handle)
    {
        std::scoped_lock lock{this->mutex_};
        this->release_handle(handle);
    }

    void GraphCache::register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback)
    {
        std::scoped_lock lock{this->mutex_};
        this->register_hot_reload_callback_internal(handle, std::move(callback));
    }

    void GraphCache::poll()
    {
        this->poll_assets();
    }

    engine::Result<void, AssetLoadError> GraphCache::reload_asset(const RawHandle& handle,
                                                                  GraphAsset& asset,
                                                                  bool notify)
    {
        const std::string identifier = asset.descriptor.handle.id();
        detail::record_hot_reload_attempt(notify, identifier);

        const auto detection_result = io::detect_geometry_file(asset.descriptor.source);
        if (!detection_result)
        {
            auto error = detail::make_geometry_asset_error(
                asset.descriptor.source, "detect_geometry_file", detection_result.error());
            detail::record_hot_reload_failure(notify, identifier, error,
                                              detail::geometry_error_hint(detection_result.error()));
            return error;
        }

        const auto& detection = detection_result.value();
        if (detection.kind != io::GeometryKind::graph)
        {
            auto error = make_asset_load_error(
                AssetLoadErrorCategory::ValidationError,
                "Geometry file does not describe a graph");
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Ensure the watched file encodes a graph (e.g., .ply edge list).");
            return error;
        }

        const io::GraphFileFormat format = asset.descriptor.format_hint != io::GraphFileFormat::unknown
                                               ? asset.descriptor.format_hint
                                               : detection.graph_format;

        if (format == io::GraphFileFormat::unknown)
        {
            auto error = make_asset_load_error(
                AssetLoadErrorCategory::ValidationError,
                "Unable to determine graph file format for asset");
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Provide a graph format hint or use a supported graph extension.");
            return error;
        }

        geometry::Graph loaded_graph{};
        if (auto result = io::read_graph(asset.descriptor.source, loaded_graph.interface, format); !result)
        {
            auto error = detail::make_geometry_asset_error(
                asset.descriptor.source, "read_graph", result.error());
            detail::record_hot_reload_failure(notify, identifier, error,
                                              detail::geometry_error_hint(result.error()));
            return error;
        }

        std::filesystem::file_time_type last_write{};
        try
        {
            last_write = detail::checked_last_write_time(asset.descriptor.source, "graph");
        }
        catch (const std::runtime_error& ex)
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Verify filesystem permissions and ensure the graph file remains accessible.");
            return error;
        }

        asset.graph = std::move(loaded_graph);
        asset.detection = detection;
        asset.last_write = last_write;

        if (notify)
        {
            const auto cb_it = this->callbacks_.find(handle);
            if (cb_it != this->callbacks_.end())
            {
                for (const auto& callback : cb_it->second)
                {
                    callback(asset);
                }
            }
        }

        return {};
    }
} // namespace engine::assets