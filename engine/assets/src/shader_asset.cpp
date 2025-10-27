#include "engine/assets/shader_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/detail/reload_utils.hpp"
#include "engine/assets/validation.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] engine::Result<std::string, AssetLoadError> read_text(const std::filesystem::path& path)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                return make_asset_load_error(AssetLoadErrorCategory::IoFailure,
                                             "Failed to open shader file: " + path.generic_string());
            }

            std::string contents{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
            if (stream.bad())
            {
                return make_asset_load_error(AssetLoadErrorCategory::IoFailure,
                                             "Failed to read shader file: " + path.generic_string());
            }

            return contents;
        }

        [[nodiscard]] ShaderBinary compile_internal(std::string_view source,
                                                    const ShaderCompilationOptions& options)
        {
            (void)options; // Placeholder for future optimization flags.

            ShaderBinary binary{};
            binary.spirv.reserve((source.size() + 3U) / 4U);

            std::uint32_t word = 0;
            std::size_t byte_index = 0;
            for (unsigned char ch : source)
            {
                word |= static_cast<std::uint32_t>(ch) << (8U * (byte_index % 4U));
                ++byte_index;
                if (byte_index % 4U == 0U)
                {
                    binary.spirv.push_back(word);
                    word = 0;
                }
            }

            if (byte_index % 4U != 0U)
            {
                binary.spirv.push_back(word);
            }

            if (binary.spirv.empty())
            {
                // Ensure downstream consumers receive a non-empty payload even for empty shaders.
                binary.spirv.push_back(0U);
            }

            return binary;
        }
    } // namespace

    ShaderBinary ShaderCompiler::compile_glsl_to_spirv(std::string_view source,
                                                       const ShaderCompilationOptions& options)
    {
        return compile_internal(source, options);
    }

    ShaderCache::ShaderCache()
        : handle_validator_registration_(HandleValidatorRegistry::instance().register_shader_validator(
            [this](const ShaderHandle& handle)
            {
                std::scoped_lock lock{mutex_};
                return handle.is_valid(assets_);
            }))
    {
    }

    const ShaderAsset& ShaderCache::load(const ShaderAssetDescriptor& descriptor)
    {
        std::scoped_lock lock{mutex_};

        const auto identifier = descriptor.handle.id();
        if (identifier.empty())
        {
            throw std::invalid_argument("Shader handle identifier cannot be empty");
        }

        ShaderAsset* asset = nullptr;
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

        const auto current_write = detail::checked_last_write_time(descriptor.source, "shader");
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

    bool ShaderCache::contains(const ShaderHandle& handle) const
    {
        std::scoped_lock lock{mutex_};
        return handle.is_valid(assets_);
    }

    const ShaderAsset& ShaderCache::get(const ShaderHandle& handle) const
    {
        std::scoped_lock lock{mutex_};
        if (!handle.is_valid(assets_))
        {
            HandleValidationTelemetry::instance().record_failure(
                HandleValidationFailure{
                    std::string{"ShaderHandle"}, handle.id(), "ShaderCache::get", "Cache lookup rejected handle"
                });
#ifndef NDEBUG
            assert(false && "Shader asset handle not found");
#endif
            throw std::out_of_range("Shader asset handle not found");
        }
        HandleValidationTelemetry::instance().record_success("ShaderHandle", handle.id());
        return assets_.get(handle.raw_handle());
    }

    void ShaderCache::unload(const ShaderHandle& handle)
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

    void ShaderCache::register_hot_reload_callback(const ShaderHandle& handle, HotReloadCallback callback)
    {
        std::scoped_lock lock{mutex_};
        if (handle.is_bound() && handle.is_valid(assets_))
        {
            callbacks_[handle.raw_handle()].push_back(std::move(callback));
            return;
        }

        if (handle.id().empty())
        {
            throw std::invalid_argument("Shader handle identifier cannot be empty");
        }

        pending_callbacks_[handle.id()].push_back(std::move(callback));
    }

    void ShaderCache::poll()
    {
        watcher_.poll();

        std::scoped_lock lock{mutex_};
        assets_.for_each([&](const RawHandle& handle, ShaderAsset& asset)
        {
            if (asset.descriptor.source.empty())
            {
                return;
            }

            if (watch_handles_.find(handle) != watch_handles_.end())
            {
                return;
            }

            const auto current_write = detail::checked_last_write_time(asset.descriptor.source, "shader");
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

    engine::Result<void, AssetLoadError> ShaderCache::reload_asset(const RawHandle& handle,
                                                                   ShaderAsset& asset,
                                                                   bool notify)
    {
        const std::string identifier = asset.descriptor.handle.id();
        detail::record_hot_reload_attempt(notify, identifier);

        auto source = read_text(asset.descriptor.source);
        if (!source)
        {
            auto error = source.error();
            detail::record_hot_reload_failure(notify, identifier, error,
                                              "Verify the shader file exists and is readable by the runtime process.");
            return error;
        }

        ShaderBinary compiled = ShaderCompiler::compile_glsl_to_spirv(source.value(), asset.descriptor.options);

        std::filesystem::file_time_type last_write{};
        try
        {
            last_write = detail::checked_last_write_time(asset.descriptor.source, "shader");
        }
        catch (const std::runtime_error& ex)
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
            detail::record_hot_reload_failure(
                notify, identifier, error,
                "Ensure the shader file remains on disk and accessible during reload attempts.");
            return error;
        }

        asset.source = std::move(source.value());
        asset.binary = std::move(compiled);
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

    void ShaderCache::register_watch_locked(const RawHandle& handle, ShaderAsset& asset)
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

    void ShaderCache::unregister_watch_locked(const RawHandle& handle)
    {
        if (auto it = watch_handles_.find(handle); it != watch_handles_.end())
        {
            watcher_.unwatch(it->second);
            watch_handles_.erase(it);
        }
    }
} // namespace engine::assets