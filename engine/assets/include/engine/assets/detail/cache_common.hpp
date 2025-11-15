#pragma once

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/validation.hpp"

#include "engine/core/memory/resource_pool.hpp"
#include "engine/platform/filesystem/watcher.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets::detail
{
    struct AssetCacheLabels
    {
        std::string asset_display_name{};
        std::string asset_file_label{};
        std::string handle_display_name{};
        std::string cache_display_name{};
    };

    template <typename Derived,
              typename Asset,
              typename Descriptor,
              typename Handle,
              typename HandleTag,
              typename HotReloadCallback,
              bool HotReloadEnabled>
    class AssetCacheLifecycle
    {
    protected:
        using Pool = core::memory::ResourcePool<Asset, HandleTag>;
        using RawHandle = typename Pool::handle_type;
        using HandleHasher = typename Pool::handle_hasher;

        struct AcquireResult
        {
            RawHandle handle{};
            Asset* asset{nullptr};
            bool inserted{false};
            std::string identifier{};
        };

        struct ReloadDecision
        {
            bool should_reload{false};
            std::filesystem::file_time_type current_write{};
        };

        explicit AssetCacheLifecycle(AssetCacheLabels labels)
            : labels_{std::move(labels)}
        {
        }

        [[nodiscard]] AcquireResult acquire_asset_slot(const Descriptor& descriptor)
        {
            AcquireResult result{};
            result.identifier = descriptor.handle.id();
            if (result.identifier.empty())
            {
                throw std::invalid_argument(labels_.asset_display_name + " handle identifier cannot be empty");
            }

            const auto lookup = bindings_.find(result.identifier);
            if (lookup == bindings_.end())
            {
                auto [acquired_handle, slot] = assets_.acquire();
                result.handle = acquired_handle;
                result.asset = &slot;
                bindings_.emplace(result.identifier, result.handle);
                result.inserted = true;
            }
            else
            {
                result.handle = lookup->second;
                result.asset = &assets_.get(result.handle);
            }

            return result;
        }

        void bind_descriptor(const Descriptor& descriptor, RawHandle handle, Asset& asset)
        {
            asset.descriptor = descriptor;
            descriptor.handle.bind(handle);
        }

        void merge_pending_callbacks(const std::string& identifier, RawHandle handle)
        {
            if constexpr (HotReloadEnabled)
            {
                if (auto pending = pending_callbacks_.find(identifier); pending != pending_callbacks_.end())
                {
                    auto& target = callbacks_[handle];
                    auto& pending_list = pending->second;
                    target.insert(target.end(),
                                  std::make_move_iterator(pending_list.begin()),
                                  std::make_move_iterator(pending_list.end()));
                    pending_callbacks_.erase(pending);
                }
            }
            else
            {
                (void)identifier;
                (void)handle;
            }
        }

        [[nodiscard]] ReloadDecision evaluate_reload(const Descriptor& descriptor,
                                                     const Asset& asset,
                                                     bool inserted) const
        {
            if (descriptor.source.empty())
            {
                return ReloadDecision{inserted, {}};
            }

            const auto current_write = detail::checked_last_write_time(descriptor.source, labels_.asset_file_label);
            const bool needs_reload = inserted || asset.last_write != current_write;
            return ReloadDecision{needs_reload, current_write};
        }

        [[nodiscard]] bool contains_handle(const Handle& handle) const
        {
            return handle.is_valid(assets_);
        }

        [[nodiscard]] const Asset& get_asset_checked(const Handle& handle) const
        {
            if (!handle.is_valid(assets_))
            {
                HandleValidationTelemetry::instance().record_failure(
                    HandleValidationFailure{
                        std::string{labels_.handle_display_name},
                        handle.id(),
                        labels_.cache_display_name + "::get",
                        "Cache lookup rejected handle"});
                const std::string not_found_message = labels_.asset_display_name + " asset handle not found";
#ifndef NDEBUG
                std::fputs((not_found_message + '\n').c_str(), stderr);
                std::abort();
#else
                throw std::out_of_range(not_found_message);
#endif
            }

            HandleValidationTelemetry::instance().record_success(labels_.handle_display_name.c_str(), handle.id());
            return assets_.get(handle.raw_handle());
        }

        void release_handle(const Handle& handle)
        {
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

            if constexpr (HotReloadEnabled)
            {
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
            }

            assets_.release(raw);
            bindings_.erase(identifier);
            handle.reset_binding();
        }

        void register_hot_reload_callback_internal(const Handle& handle, HotReloadCallback callback)
        {
            if constexpr (HotReloadEnabled)
            {
                if (handle.is_bound() && handle.is_valid(assets_))
                {
                    callbacks_[handle.raw_handle()].push_back(std::move(callback));
                    return;
                }

                if (handle.id().empty())
                {
                    throw std::invalid_argument(labels_.asset_display_name + " handle identifier cannot be empty");
                }

                pending_callbacks_[handle.id()].push_back(std::move(callback));
            }
            else
            {
                (void)handle;
                (void)callback;
                throw std::logic_error(labels_.asset_display_name + " cache does not support hot reload callbacks");
            }
        }

        void poll_assets()
        {
            if constexpr (!HotReloadEnabled)
            {
                watcher_.poll();
                return;
            }

            watcher_.poll();

            std::scoped_lock lock{mutex_};
            assets_.for_each([this](const RawHandle& handle, Asset& asset)
            {
                if (asset.descriptor.source.empty())
                {
                    return;
                }

                if (watch_handles_.find(handle) != watch_handles_.end())
                {
                    return;
                }

                const auto current_write = detail::checked_last_write_time(asset.descriptor.source, labels_.asset_file_label);
                if (current_write != asset.last_write)
                {
                    if (auto reload = static_cast<Derived*>(this)->reload_asset(handle, asset, true); !reload.has_value())
                    {
                        return;
                    }
                    register_watch_locked(handle, asset);
                }
            });
        }

        template <typename Visitor>
        void for_each_asset(Visitor&& visitor) const
        {
            std::scoped_lock lock{mutex_};
            assets_.for_each([&](const RawHandle& handle, const Asset& asset)
            {
                std::invoke(visitor, handle, asset);
            });
        }

        void register_watch_locked(const RawHandle& handle, Asset& asset)
        {
            if constexpr (!HotReloadEnabled)
            {
                (void)handle;
                (void)asset;
                return;
            }

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
                if (auto reload = static_cast<Derived*>(this)->reload_asset(handle, tracked, true); !reload.has_value())
                {
                    return;
                }
            };

            const auto watch_handle = watcher_.watch_file(asset.descriptor.source, std::move(callback));
            watch_handles_.emplace(handle, watch_handle);
        }

        void unregister_watch_locked(const RawHandle& handle)
        {
            if constexpr (HotReloadEnabled)
            {
                if (auto it = watch_handles_.find(handle); it != watch_handles_.end())
                {
                    watcher_.unwatch(it->second);
                    watch_handles_.erase(it);
                }
            }
            else
            {
                (void)handle;
            }
        }

        Pool assets_{};
        std::unordered_map<std::string, RawHandle> bindings_{};
        std::unordered_map<std::string, std::vector<HotReloadCallback>> pending_callbacks_{};
        std::unordered_map<RawHandle, std::vector<HotReloadCallback>, HandleHasher> callbacks_{};
        std::unordered_map<RawHandle, platform::filesystem::FilesystemWatcher::WatchHandle, HandleHasher> watch_handles_{};
        platform::filesystem::FilesystemWatcher watcher_{};
        mutable std::mutex mutex_{};

    private:
        AssetCacheLabels labels_{};
    };
} // namespace engine::assets::detail
