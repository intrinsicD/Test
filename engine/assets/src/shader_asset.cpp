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
        : Base(detail::AssetCacheLabels{
              "Shader",
              "shader",
              "ShaderHandle",
              "ShaderCache"})
        , handle_validator_registration_(HandleValidatorRegistry::instance().register_shader_validator(
            [this](const ShaderHandle& handle)
            {
                std::scoped_lock lock{this->mutex_};
                return handle.is_valid(this->assets_);
            }))
    {
    }

    const ShaderAsset& ShaderCache::load(const ShaderAssetDescriptor& descriptor)
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

    bool ShaderCache::contains(const ShaderHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->contains_handle(handle);
    }

    const ShaderAsset& ShaderCache::get(const ShaderHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->get_asset_checked(handle);
    }

    void ShaderCache::unload(const ShaderHandle& handle)
    {
        std::scoped_lock lock{this->mutex_};
        this->release_handle(handle);
    }

    void ShaderCache::register_hot_reload_callback(const ShaderHandle& handle, HotReloadCallback callback)
    {
        std::scoped_lock lock{this->mutex_};
        this->register_hot_reload_callback_internal(handle, std::move(callback));
    }

    void ShaderCache::poll()
    {
        this->poll_assets();
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