#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/detail/cache_common.hpp"
#include "engine/assets/handles.hpp"

#include "engine/core/memory/resource_pool.hpp"
#include "engine/platform/filesystem/watcher.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine::assets
{
    enum class ShaderStage : std::uint8_t
    {
        vertex = 0,
        fragment,
        compute
    };

    struct ShaderCompilationOptions
    {
        bool optimize{false};
    };

    struct ShaderBinary
    {
        std::vector<std::uint32_t> spirv;
    };

    struct ShaderAssetDescriptor
    {
        ShaderHandle handle;
        std::filesystem::path source;
        ShaderStage stage{ShaderStage::vertex};
        ShaderCompilationOptions options{};

        [[nodiscard]] static ShaderAssetDescriptor from_file(const std::filesystem::path& path,
                                                             ShaderStage stage = ShaderStage::vertex,
                                                             ShaderCompilationOptions options = {})
        {
            return ShaderAssetDescriptor{ShaderHandle{path}, path, stage, options};
        }
    };

    struct ShaderAsset
    {
        ShaderAssetDescriptor descriptor{};
        ShaderBinary binary{};
        std::string source{};
        std::filesystem::file_time_type last_write{};
    };

    class ShaderCompiler
    {
    public:
        [[nodiscard]] static ShaderBinary compile_glsl_to_spirv(std::string_view source,
                                                                const ShaderCompilationOptions& options);
    };

    class ShaderCache : public detail::AssetCacheLifecycle<
                            ShaderCache,
                            ShaderAsset,
                            ShaderAssetDescriptor,
                            ShaderHandle,
                            ShaderHandleTag,
                            std::function<void(const ShaderAsset&)>,
                            true>
    {
    public:
        using Base = detail::AssetCacheLifecycle<
            ShaderCache,
            ShaderAsset,
            ShaderAssetDescriptor,
            ShaderHandle,
            ShaderHandleTag,
            std::function<void(const ShaderAsset&)>,
            true>;

        ShaderCache();

        using HotReloadCallback = std::function<void(const ShaderAsset&)>;

        [[nodiscard]] const ShaderAsset& load(const ShaderAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const ShaderHandle& handle) const;
        [[nodiscard]] const ShaderAsset& get(const ShaderHandle& handle) const;

        void unload(const ShaderHandle& handle);
        void register_hot_reload_callback(const ShaderHandle& handle, HotReloadCallback callback);
        void poll();

    private:
        using typename Base::RawHandle;

    protected:
        friend Base;
        engine::Result<void, AssetLoadError> reload_asset(const RawHandle& handle, ShaderAsset& asset, bool notify);

    private:
        std::shared_ptr<void> handle_validator_registration_{};
    };
} // namespace engine::assets