#include "engine/assets/shader_asset.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
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
                throw std::runtime_error("Failed to query shader asset timestamp: " + ec.message());
            }
            return time;
        }

        [[nodiscard]] std::string read_text(const std::filesystem::path& path)
        {
            std::ifstream stream{path};
            if (!stream)
            {
                throw std::runtime_error("Failed to open shader file: " + path.generic_string());
            }

            return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
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

    const ShaderAsset& ShaderCache::load(const ShaderAssetDescriptor& descriptor)
    {
        auto [it, inserted] = assets_.try_emplace(descriptor.handle);
        ShaderAsset& asset = it->second;
        asset.descriptor = descriptor;

        const auto current_write = safe_last_write_time(descriptor.source);
        const bool needs_reload = inserted || asset.last_write != current_write;
        if (needs_reload)
        {
            reload_asset(descriptor.handle, asset, !inserted);
        }

        return asset;
    }

    bool ShaderCache::contains(const ShaderHandle& handle) const
    {
        return assets_.find(handle) != assets_.end();
    }

    const ShaderAsset& ShaderCache::get(const ShaderHandle& handle) const
    {
        const auto it = assets_.find(handle);
        if (it == assets_.end())
        {
            throw std::out_of_range("Shader asset handle not found");
        }
        return it->second;
    }

    void ShaderCache::unload(const ShaderHandle& handle)
    {
        assets_.erase(handle);
        callbacks_.erase(handle);
    }

    void ShaderCache::register_hot_reload_callback(const ShaderHandle& handle, HotReloadCallback callback)
    {
        callbacks_[handle].push_back(std::move(callback));
    }

    void ShaderCache::poll()
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

    void ShaderCache::reload_asset(const ShaderHandle& handle, ShaderAsset& asset, bool notify)
    {
        asset.source = read_text(asset.descriptor.source);
        asset.binary = ShaderCompiler::compile_glsl_to_spirv(asset.source, asset.descriptor.options);
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

