#pragma once

#include "engine/core/memory/resource_pool.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace engine::assets {

/// Shared state for asset handles. Multiple handle instances can reference the
/// same underlying identifier while caches bind them to generational handles as
/// resources are loaded.
template <typename Tag>
class ResourceHandle {
public:
    using pool_handle_type = core::memory::GenerationalHandle<Tag>;

    ResourceHandle() = default;

    explicit ResourceHandle(std::string identifier)
        : state_(std::make_shared<State>(State{std::move(identifier), {}}))
    {
    }

    explicit ResourceHandle(const std::filesystem::path& path)
        : ResourceHandle(path.generic_string())
    {
    }

    [[nodiscard]] bool empty() const noexcept { return id().empty(); }

    [[nodiscard]] const std::string& id() const noexcept
    {
        static const std::string empty_identifier{};
        return state_ ? state_->identifier : empty_identifier;
    }

    [[nodiscard]] pool_handle_type raw_handle() const noexcept
    {
        return state_ ? state_->handle : pool_handle_type{};
    }

    [[nodiscard]] bool is_bound() const noexcept
    {
        return state_ && state_->handle.is_valid();
    }

    template <typename Resource>
    [[nodiscard]] bool is_valid(const core::memory::ResourcePool<Resource, Tag>& pool) const noexcept
    {
        return state_ && pool.is_valid(state_->handle);
    }

    explicit operator bool() const noexcept { return !empty(); }

    /// Bind the handle to a generational slot. The method is const so caches can
    /// update handles that appear within const descriptors.
    void bind(pool_handle_type handle) const
    {
        ensure_state();
        state_->handle = handle;
    }

    /// Reset the bound generational handle while preserving the identifier.
    void reset_binding() const
    {
        if (state_) {
            state_->handle = {};
        }
    }

    friend bool operator==(const ResourceHandle& lhs, const ResourceHandle& rhs) noexcept
    {
        return lhs.id() == rhs.id();
    }

    friend bool operator!=(const ResourceHandle& lhs, const ResourceHandle& rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    struct State {
        std::string identifier{};
        pool_handle_type handle{};
    };

    void ensure_state() const
    {
        if (!state_) {
            state_ = std::make_shared<State>();
        }
    }

    mutable std::shared_ptr<State> state_{};
};

struct MeshHandleTag;
struct GraphHandleTag;
struct PointCloudHandleTag;
struct TextureHandleTag;
struct ShaderHandleTag;
struct MaterialHandleTag;

using MeshHandle = ResourceHandle<MeshHandleTag>;
using GraphHandle = ResourceHandle<GraphHandleTag>;
using PointCloudHandle = ResourceHandle<PointCloudHandleTag>;
using TextureHandle = ResourceHandle<TextureHandleTag>;
using ShaderHandle = ResourceHandle<ShaderHandleTag>;
using MaterialHandle = ResourceHandle<MaterialHandleTag>;

}  // namespace engine::assets

namespace std {

template <typename Tag>
struct hash<engine::assets::ResourceHandle<Tag>> {
    [[nodiscard]] std::size_t operator()(const engine::assets::ResourceHandle<Tag>& handle) const noexcept
    {
        return std::hash<std::string>()(handle.id());
    }
};

}  // namespace std

