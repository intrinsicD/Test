#pragma once

#include "engine/assets/handles.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine::assets {

enum class ShaderStage : std::uint8_t {
    vertex = 0,
    fragment,
    compute
};

struct ShaderCompilationOptions {
    bool optimize{false};
};

struct ShaderBinary {
    std::vector<std::uint32_t> spirv;
};

struct ShaderAssetDescriptor {
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

struct ShaderAsset {
    ShaderAssetDescriptor descriptor{};
    ShaderBinary binary{};
    std::string source{};
    std::filesystem::file_time_type last_write{};
};

class ShaderCompiler {
public:
    [[nodiscard]] static ShaderBinary compile_glsl_to_spirv(std::string_view source,
                                                            const ShaderCompilationOptions& options);
};

class ShaderCache {
public:
    using HotReloadCallback = std::function<void(const ShaderAsset&)>;

    [[nodiscard]] const ShaderAsset& load(const ShaderAssetDescriptor& descriptor);
    [[nodiscard]] bool contains(const ShaderHandle& handle) const;
    [[nodiscard]] const ShaderAsset& get(const ShaderHandle& handle) const;

    void unload(const ShaderHandle& handle);
    void register_hot_reload_callback(const ShaderHandle& handle, HotReloadCallback callback);
    void poll();

private:
    void reload_asset(const ShaderHandle& handle, ShaderAsset& asset, bool notify);

    std::unordered_map<ShaderHandle, ShaderAsset> assets_{};
    std::unordered_map<ShaderHandle, std::vector<HotReloadCallback>> callbacks_{};
};

}  // namespace engine::assets

