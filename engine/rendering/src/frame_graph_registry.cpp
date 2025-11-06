#include "engine/rendering/frame_graph_registry.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
    constexpr std::string_view kBuiltinOrigin{"<builtin>"};
}

namespace engine::rendering
{
    FrameGraphNodeRegistry::FrameGraphNodeRegistry() = default;
    FrameGraphNodeRegistry::~FrameGraphNodeRegistry() = default;

    void FrameGraphNodeRegistry::register_builtin(NodeFactoryDescriptor descriptor)
    {
        register_node(std::move(descriptor), kBuiltinOrigin);
    }

    FrameGraphNodeRegistry::PluginRegistration::PluginRegistration(
        FrameGraphNodeRegistry* registry,
        std::shared_ptr<PluginState> state)
        : registry_{registry}
        , state_{std::move(state)}
    {
    }

    FrameGraphNodeRegistry::PluginRegistration::PluginRegistration(PluginRegistration&& other) noexcept
        : registry_{other.registry_}
        , state_{std::move(other.state_)}
    {
        other.registry_ = nullptr;
    }

    FrameGraphNodeRegistry::PluginRegistration& FrameGraphNodeRegistry::PluginRegistration::operator=(
        PluginRegistration&& other) noexcept
    {
        if (this != &other)
        {
            release();
            registry_ = other.registry_;
            state_ = std::move(other.state_);
            other.registry_ = nullptr;
        }
        return *this;
    }

    FrameGraphNodeRegistry::PluginRegistration::~PluginRegistration()
    {
        release();
    }

    void FrameGraphNodeRegistry::PluginRegistration::release()
    {
        if (registry_ && state_)
        {
            registry_->deactivate_plugin_state(state_);
            registry_ = nullptr;
            state_.reset();
        }
    }

    FrameGraphNodeRegistry::PluginRegistration FrameGraphNodeRegistry::register_plugin_nodes(
        std::string plugin_id,
        std::vector<NodeFactoryDescriptor> descriptors)
    {
        if (plugin_id.empty())
        {
            throw std::invalid_argument("plugin_id must not be empty");
        }
        if (plugin_id == kBuiltinOrigin)
        {
            throw std::invalid_argument("plugin_id collides with reserved builtin origin");
        }

        if (auto existing = plugin_states_.find(plugin_id); existing != plugin_states_.end())
        {
            if (auto previous = existing->second.lock())
            {
                if (previous->active)
                {
                    notify_listeners(PluginReloadEvent{PluginReloadEvent::Phase::Unloading, previous->plugin_id});
                    remove_nodes_from_origin(previous->plugin_id);
                    previous->active = false;
                }
            }
            plugin_states_.erase(existing);
        }
        else
        {
            remove_nodes_from_origin(plugin_id);
        }

        auto state = std::make_shared<PluginState>();
        state->plugin_id = plugin_id;

        for (auto& descriptor : descriptors)
        {
            register_node(std::move(descriptor), state->plugin_id);
        }

        plugin_states_.emplace(state->plugin_id, state);
        notify_listeners(PluginReloadEvent{PluginReloadEvent::Phase::Loaded, state->plugin_id});
        return PluginRegistration(this, std::move(state));
    }

    void FrameGraphNodeRegistry::add_hot_reload_listener(HotReloadCallback callback)
    {
        reload_listeners_.push_back(std::move(callback));
    }

    bool FrameGraphNodeRegistry::contains(std::string_view node_id) const noexcept
    {
        return nodes_.find(node_id) != nodes_.end();
    }

    const NodeDescriptor* FrameGraphNodeRegistry::find(std::string_view node_id) const noexcept
    {
        const auto it = nodes_.find(node_id);
        return it != nodes_.end() ? std::addressof(it->second.descriptor) : nullptr;
    }

    std::unique_ptr<INode> FrameGraphNodeRegistry::create(std::string_view node_id) const
    {
        const auto it = nodes_.find(node_id);
        if (it == nodes_.end())
        {
            throw std::out_of_range("Unknown frame-graph node id");
        }

        auto node = it->second.factory();
        if (!node)
        {
            throw std::runtime_error("Node factory returned null instance");
        }

        const auto& reflected = node->Reflect();
        if (reflected.id != it->second.descriptor.id)
        {
            throw std::runtime_error("Node factory returned mismatched descriptor");
        }

        return node;
    }

    void FrameGraphNodeRegistry::register_node(NodeFactoryDescriptor descriptor, std::string_view origin)
    {
        if (descriptor.descriptor.id.empty())
        {
            throw std::invalid_argument("Node descriptor id must not be empty");
        }
        if (!descriptor.factory)
        {
            throw std::invalid_argument("Node factory must not be null");
        }

        const std::string node_id = descriptor.descriptor.id;
        NodeEntry entry{std::move(descriptor.descriptor), std::move(descriptor.factory), std::string(origin)};

        const auto [unused, inserted] = nodes_.emplace(node_id, std::move(entry));
        (void)unused;
        if (!inserted)
        {
            throw std::invalid_argument("Frame-graph node already registered: " + node_id);
        }

        nodes_by_origin_[std::string(origin)].insert(node_id);
    }

    void FrameGraphNodeRegistry::remove_nodes_from_origin(std::string_view origin)
    {
        const auto origin_it = nodes_by_origin_.find(origin);
        if (origin_it == nodes_by_origin_.end())
        {
            return;
        }

        for (const auto& node_id : origin_it->second)
        {
            nodes_.erase(node_id);
        }

        nodes_by_origin_.erase(origin_it);
    }

    void FrameGraphNodeRegistry::notify_listeners(PluginReloadEvent event)
    {
        for (auto& listener : reload_listeners_)
        {
            listener(event);
        }
    }

    void FrameGraphNodeRegistry::deactivate_plugin_state(const std::shared_ptr<PluginState>& state)
    {
        if (!state || !state->active)
        {
            return;
        }

        const auto it = plugin_states_.find(state->plugin_id);
        if (it == plugin_states_.end() || it->second.lock() != state)
        {
            state->active = false;
            return;
        }

        notify_listeners(PluginReloadEvent{PluginReloadEvent::Phase::Unloading, state->plugin_id});
        remove_nodes_from_origin(state->plugin_id);
        state->active = false;
        plugin_states_.erase(it);
    }
}

