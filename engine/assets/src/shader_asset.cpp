#include "engine/assets/shader_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace engine::assets {

namespace {

[[nodiscard]] std::string read_text(const std::filesystem::path& path)
{
    std::ifstream stream{path};
    if (!stream) {
        throw std::runtime_error("Failed to open shader file: " + path.generic_string());
    }

    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

[[nodiscard]] ShaderBinary compile_internal(std::string_view source,
                                            const ShaderCompilationOptions& options)
{
    (void)options;  // Placeholder for future optimization flags.

    ShaderBinary binary{};
    binary.spirv.reserve((source.size() + 3U) / 4U);

    std::uint32_t word = 0;
    std::size_t byte_index = 0;
    for (unsigned char ch : source) {
        word |= static_cast<std::uint32_t>(ch) << (8U * (byte_index % 4U));
        ++byte_index;
        if (byte_index % 4U == 0U) {
            binary.spirv.push_back(word);
            word = 0;
        }
    }

    if (byte_index % 4U != 0U) {
        binary.spirv.push_back(word);
    }

    if (binary.spirv.empty()) {
        // Ensure downstream consumers receive a non-empty payload even for empty shaders.
        binary.spirv.push_back(0U);
    }

    return binary;
}

}  // namespace

ShaderBinary ShaderCompiler::compile_glsl_to_spirv(std::string_view source,
                                                   const ShaderCompilationOptions& options)
{
    return compile_internal(source, options);
}

const ShaderAsset& ShaderCache::load(const ShaderAssetDescriptor& descriptor)
{
    const auto identifier = descriptor.handle.id();
    if (identifier.empty()) {
        throw std::invalid_argument("Shader handle identifier cannot be empty");
    }

    ShaderAsset* asset = nullptr;
    RawHandle handle{};
    bool inserted = false;

    const auto lookup = bindings_.find(identifier);
    if (lookup == bindings_.end()) {
        auto [acquired_handle, slot] = assets_.acquire();
        handle = acquired_handle;
        asset = &slot;
        bindings_.emplace(identifier, handle);
        inserted = true;
    } else {
        handle = lookup->second;
        asset = &assets_.get(handle);
    }

    asset->descriptor = descriptor;
    descriptor.handle.bind(handle);

    if (auto pending = pending_callbacks_.find(identifier); pending != pending_callbacks_.end()) {
        auto& target = callbacks_[handle];
        auto& pending_list = pending->second;
        target.insert(target.end(),
                      std::make_move_iterator(pending_list.begin()),
                      std::make_move_iterator(pending_list.end()));
        pending_callbacks_.erase(pending);
    }

    const auto current_write = detail::checked_last_write_time(descriptor.source, "shader");
    const bool needs_reload = inserted || asset->last_write != current_write;
    if (needs_reload) {
        reload_asset(handle, *asset, !inserted);
    }

    return *asset;
}

bool ShaderCache::contains(const ShaderHandle& handle) const
{
    return handle.is_valid(assets_);
}

const ShaderAsset& ShaderCache::get(const ShaderHandle& handle) const
{
    if (!handle.is_valid(assets_)) {
        throw std::out_of_range("Shader asset handle not found");
    }
    return assets_.get(handle.raw_handle());
}

void ShaderCache::unload(const ShaderHandle& handle)
{
    if (!handle.is_bound()) {
        return;
    }

    const auto raw = handle.raw_handle();
    if (!assets_.is_valid(raw)) {
        handle.reset_binding();
        return;
    }

    const auto identifier = assets_.get(raw).descriptor.handle.id();

    if (auto cb_it = callbacks_.find(raw); cb_it != callbacks_.end()) {
        if (!identifier.empty()) {
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
    if (handle.is_bound() && handle.is_valid(assets_)) {
        callbacks_[handle.raw_handle()].push_back(std::move(callback));
        return;
    }

    if (handle.id().empty()) {
        throw std::invalid_argument("Shader handle identifier cannot be empty");
    }

    pending_callbacks_[handle.id()].push_back(std::move(callback));
}

void ShaderCache::poll()
{
    assets_.for_each([&](const RawHandle& handle, ShaderAsset& asset) {
        const auto current_write = detail::checked_last_write_time(asset.descriptor.source, "shader");
        if (current_write != asset.last_write) {
            reload_asset(handle, asset, true);
        }
    });
}

void ShaderCache::reload_asset(const RawHandle& handle, ShaderAsset& asset, bool notify)
{
    asset.source = read_text(asset.descriptor.source);
    asset.binary = ShaderCompiler::compile_glsl_to_spirv(asset.source, asset.descriptor.options);
    asset.last_write = detail::checked_last_write_time(asset.descriptor.source, "shader");

    if (notify) {
        const auto cb_it = callbacks_.find(handle);
        if (cb_it != callbacks_.end()) {
            for (const auto& callback : cb_it->second) {
                callback(asset);
            }
        }
    }
}

}  // namespace engine::assets

