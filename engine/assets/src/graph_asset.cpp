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
        : handle_validator_registration_(HandleValidatorRegistry::instance().register_graph_validator(
            [this](const GraphHandle& handle)
            {
                std::scoped_lock lock{mutex_};
                return handle.is_valid(assets_);
            }))
    {
    }

    const GraphAsset& GraphCache::load(const GraphAssetDescriptor& descriptor)
    {
        std::scoped_lock lock{mutex_};

        const auto identifier = descriptor.handle.id();
        if (identifier.empty())
        {
            throw std::invalid_argument("Graph handle identifier cannot be empty");
        }

        GraphAsset* asset = nullptr;
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

        const auto current_write = detail::checked_last_write_time(descriptor.source, "graph");
        const bool needs_reload = inserted || asset->last_write != current_write;
        if (needs_reload)
        {
            if (auto reload = reload_asset(handle, *asset, !inserted); !reload.has_value())
            {
                const auto message = reload.error().message();
                throw std::runtime_error(message.empty()
                                             ? std::string{to_string(reload.error().code())}
                                             : std::string{message});
            }
        }

        register_watch_locked(handle, *asset);

        return *asset;
    }

    bool GraphCache::contains(const GraphHandle& handle) const
    {
        std::scoped_lock lock{mutex_};
        return handle.is_valid(assets_);
    }

    const GraphAsset& GraphCache::get(const GraphHandle& handle) const
    {
        std::scoped_lock lock{mutex_};
        if (!handle.is_valid(assets_))
        {
            HandleValidationTelemetry::instance().record_failure(
                HandleValidationFailure{
                    std::string{"GraphHandle"}, handle.id(), "GraphCache::get", "Cache lookup rejected handle"
                });
#ifndef NDEBUG
            assert(false && "Graph asset handle not found");
#endif
            throw std::out_of_range("Graph asset handle not found");
        }
        HandleValidationTelemetry::instance().record_success("GraphHandle", handle.id());
        return assets_.get(handle.raw_handle());
    }

    void GraphCache::unload(const GraphHandle& handle)
    {
        std::scoped_lock lock{mutex_};
        if (!handle.is_bound())
        {
            return;
        }

        const auto raw = handle.raw_handle();
        if (!assets_.is_valid(raw))
        {
            handle.reset_binding();
            return;
        }

        const auto identifier = assets_.get(raw).descriptor.handle.id();

        unregister_watch_locked(raw);

        if (auto cb_it = callbacks_.find(raw); cb_it != callbacks_.end())
        {
            if (!identifier.empty())
            {
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
        std::scoped_lock lock{mutex_};
        if (handle.is_bound() && handle.is_valid(assets_))
        {
            callbacks_[handle.raw_handle()].push_back(std::move(callback));
            return;
        }

        if (handle.id().empty())
        {
            throw std::invalid_argument("Graph handle identifier cannot be empty");
        }

        pending_callbacks_[handle.id()].push_back(std::move(callback));
    }

    void GraphCache::poll()
    {
        watcher_.poll();

        std::scoped_lock lock{mutex_};
        assets_.for_each([&](const RawHandle& handle, GraphAsset& asset)
        {
            if (asset.descriptor.source.empty())
            {
                return;
            }

            if (watch_handles_.find(handle) != watch_handles_.end())
            {
                return;
            }

            const auto current_write =
                detail::checked_last_write_time(asset.descriptor.source, "graph");
            if (current_write != asset.last_write)
            {
                if (auto reload = reload_asset(handle, asset, true); !reload.has_value())
                {
                    return;
                }
                register_watch_locked(handle, asset);
            }
        });
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
            const auto cb_it = callbacks_.find(handle);
            if (cb_it != callbacks_.end())
            {
                for (const auto& callback : cb_it->second)
                {
                    callback(asset);
                }
            }
        }

        return {};
    }

    void GraphCache::register_watch_locked(const RawHandle& handle, GraphAsset& asset)
    {
        if (asset.descriptor.source.empty())
        {
            unregister_watch_locked(handle);
            return;
        }

        if (auto existing = watch_handles_.find(handle); existing != watch_handles_.end())
        {
            watcher_.unwatch(existing->second);
            watch_handles_.erase(existing);
        }

        auto callback = [this, handle](const platform::filesystem::WatchEvent& event)
        {
            if (event.type == platform::filesystem::WatchEventType::erased)
            {
                std::scoped_lock lock{mutex_};
                if (!assets_.is_valid(handle))
                {
                    return;
                }

                assets_.get(handle).last_write = event.timestamp;
                return;
            }

            std::scoped_lock lock{mutex_};
            if (!assets_.is_valid(handle))
            {
                return;
            }

            auto& tracked = assets_.get(handle);
            if (auto reload = reload_asset(handle, tracked, true); !reload.has_value())
            {
                return;
            }
        };

        const auto watch_handle = watcher_.watch_file(asset.descriptor.source, std::move(callback));
        watch_handles_.emplace(handle, watch_handle);
    }

    void GraphCache::unregister_watch_locked(const RawHandle& handle)
    {
        if (auto it = watch_handles_.find(handle); it != watch_handles_.end())
        {
            watcher_.unwatch(it->second);
            watch_handles_.erase(it);
        }
    }
} // namespace engine::assets