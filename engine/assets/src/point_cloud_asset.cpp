#include "engine/assets/point_cloud_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/detail/reload_utils.hpp"
#include "engine/assets/validation.hpp"

#include <cassert>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>

namespace engine::assets
{
    PointCloudCache::PointCloudCache()
        : Base(detail::AssetCacheLabels{
              "Point cloud",
              "point cloud",
              "PointCloudHandle",
              "PointCloudCache"})
        , handle_validator_registration_(HandleValidatorRegistry::instance().register_point_cloud_validator(
            [this](const PointCloudHandle& handle)
            {
                std::scoped_lock lock{this->mutex_};
                return handle.is_valid(this->assets_);
            }))
    {
    }

    const PointCloudAsset& PointCloudCache::load(const PointCloudAssetDescriptor& descriptor)
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
                throw AssetLoadException(reload.error());
            }
        }

        this->register_watch_locked(acquisition.handle, *acquisition.asset);

        return *acquisition.asset;
    }

    bool PointCloudCache::contains(const PointCloudHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->contains_handle(handle);
    }

    const PointCloudAsset& PointCloudCache::get(const PointCloudHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->get_asset_checked(handle);
    }

    void PointCloudCache::unload(const PointCloudHandle& handle)
    {
        std::scoped_lock lock{this->mutex_};
        this->release_handle(handle);
    }

    void PointCloudCache::register_hot_reload_callback(const PointCloudHandle& handle,
                                                       HotReloadCallback callback)
    {
        std::scoped_lock lock{this->mutex_};
        this->register_hot_reload_callback_internal(handle, std::move(callback));
    }

    void PointCloudCache::poll()
    {
        this->poll_assets();
    }

    AssetLoadFuture<PointCloudHandle> PointCloudCache::load_async(const AssetLoadRequest& request,
                                                                  core::threading::IoThreadPool& pool)
    {
        if (request.identifier.empty())
        {
            throw std::invalid_argument("Asset load request identifier cannot be empty");
        }

        PointCloudAssetDescriptor descriptor{};
        descriptor.handle = PointCloudHandle{request.identifier};
        descriptor.source = std::filesystem::path{request.identifier};

        return async_queue_.schedule(
            request.identifier,
            request.priority,
            request.allow_blocking_fallback,
            [this, descriptor](detail::AssetLoadPromise<PointCloudHandle>& promise) -> AssetLoadResult<PointCloudHandle>
            {
                const auto& identifier = descriptor.handle.id();

                if (promise.cancellation_requested())
                {
                    return detail::cancel_pending_request<PointCloudHandle>(
                        promise, identifier, "geometry detection");
                }

                if (!descriptor.source.empty())
                {
                    if (promise.cancellation_requested())
                    {
                        return detail::cancel_pending_request<PointCloudHandle>(
                            promise, identifier, "geometry detection");
                    }

                    if (const auto detection = io::detect_geometry_file(descriptor.source); !detection)
                    {
                        auto error = detail::make_geometry_asset_error(
                            descriptor.source, "load_async.detect_geometry_file", detection.error());
                        AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                        return AssetLoadResult<PointCloudHandle>{error};
                    }
                }

                if (promise.cancellation_requested())
                {
                    return detail::cancel_pending_request<PointCloudHandle>(
                        promise, identifier, "point cloud decode");
                }

                const auto make_error = [&descriptor](AssetLoadErrorCategory category, const char* reason)
                {
                    std::string message;
                    if (!descriptor.source.empty())
                    {
                        message = descriptor.source.generic_string();
                    }
                    else
                    {
                        message = descriptor.handle.id();
                    }

                    if (reason != nullptr && reason[0] != '\0')
                    {
                        if (!message.empty())
                        {
                            message.append(": ");
                        }

                        message.append(reason);
                    }

                    return make_asset_load_error(category, std::move(message));
                };

                try
                {
                    const auto& asset = this->load(descriptor);

                    if (promise.cancellation_requested())
                    {
                        return detail::cancel_pending_request<PointCloudHandle>(
                            promise, identifier, "point cloud decode");
                    }

                    return AssetLoadResult<PointCloudHandle>{asset.descriptor.handle};
                }
                catch (const AssetLoadException& ex)
                {
                    auto error = ex.error();
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::invalid_argument& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::ValidationError, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::out_of_range& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::ValidationError, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::filesystem::filesystem_error& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::IoFailure, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::system_error& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::IoFailure, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::runtime_error& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::IoFailure, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
                catch (const std::exception& ex)
                {
                    auto error = make_error(AssetLoadErrorCategory::DecodeError, ex.what());
                    AssetHotReloadTelemetry::instance().record_failure(error, identifier);
                    return AssetLoadResult<PointCloudHandle>{error};
                }
            },
            pool);
    }

    AssetLoadState PointCloudCache::async_state(std::string_view identifier) const
    {
        return async_queue_.state(identifier);
    }

    engine::Result<void, AssetLoadError> PointCloudCache::reload_asset(const RawHandle& handle,
                                                                       PointCloudAsset& asset,
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
        if (detection.kind != io::GeometryKind::point_cloud)
        {
            auto error = make_asset_load_error(
                AssetLoadErrorCategory::ValidationError,
                "Geometry file does not describe a point cloud");
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Ensure the watched file contains point cloud data (e.g., .ply, .pcd).");
            return error;
        }

        const io::PointCloudFileFormat format = asset.descriptor.format_hint != io::PointCloudFileFormat::unknown
                                                    ? asset.descriptor.format_hint
                                                    : detection.point_cloud_format;

        if (format == io::PointCloudFileFormat::unknown)
        {
            auto error = make_asset_load_error(
                AssetLoadErrorCategory::ValidationError,
                "Unable to determine point cloud file format for asset");
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Provide a format hint or use a supported point cloud extension.");
            return error;
        }

        geometry::PointCloud loaded_cloud{};
        if (auto result =
                io::read_point_cloud(asset.descriptor.source, loaded_cloud.interface, format);
            !result)
        {
            auto error = detail::make_geometry_asset_error(
                asset.descriptor.source, "read_point_cloud", result.error());
            detail::record_hot_reload_failure(notify, identifier, error,
                                              detail::geometry_error_hint(result.error()));
            return error;
        }

        std::filesystem::file_time_type last_write{};
        try
        {
            last_write = detail::checked_last_write_time(asset.descriptor.source, "point cloud");
        }
        catch (const std::runtime_error& ex)
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Verify filesystem permissions and ensure the point cloud file remains accessible.");
            return error;
        }

        asset.point_cloud = std::move(loaded_cloud);
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