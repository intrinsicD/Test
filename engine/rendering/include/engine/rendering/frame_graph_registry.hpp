#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/rendering/frame_graph_node.hpp"

namespace engine::rendering
{
    struct TransparentStringHash
    {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view value) const noexcept
        {
            return std::hash<std::string_view>{}(value);
        }

        [[nodiscard]] std::size_t operator()(const std::string& value) const noexcept
        {
            return std::hash<std::string>{}(value);
        }

        [[nodiscard]] std::size_t operator()(const char* value) const noexcept
        {
            return std::hash<std::string_view>{}(value);
        }
    };

    /**
     * \brief Factory descriptor used to register frame-graph planner nodes.
     */
    struct NodeFactoryDescriptor
    {
        NodeDescriptor descriptor;
        std::function<std::unique_ptr<INode>()> factory;
    };

    /**
     * \brief Event emitted when plugin-provided nodes are hot reloaded.
     */
    struct PluginReloadEvent
    {
        enum class Phase
        {
            Unloading,
            Loaded,
        };

        Phase phase{Phase::Loaded};
        std::string_view plugin_id;
    };

    /**
     * \brief Registry responsible for managing built-in and plugin-provided planner nodes.
     */
    class FrameGraphNodeRegistry
    {
    private:
        struct PluginState;

    public:
        FrameGraphNodeRegistry();
        FrameGraphNodeRegistry(const FrameGraphNodeRegistry&) = delete;
        FrameGraphNodeRegistry(FrameGraphNodeRegistry&&) noexcept = delete;
        FrameGraphNodeRegistry& operator=(const FrameGraphNodeRegistry&) = delete;
        FrameGraphNodeRegistry& operator=(FrameGraphNodeRegistry&&) noexcept = delete;
        ~FrameGraphNodeRegistry();

        void register_builtin(NodeFactoryDescriptor descriptor);

        class PluginRegistration
        {
        public:
            PluginRegistration() = default;
            PluginRegistration(FrameGraphNodeRegistry* registry, std::shared_ptr<PluginState> state);
            PluginRegistration(const PluginRegistration&) = delete;
            PluginRegistration(PluginRegistration&& other) noexcept;
            PluginRegistration& operator=(const PluginRegistration&) = delete;
            PluginRegistration& operator=(PluginRegistration&& other) noexcept;
            ~PluginRegistration();

            void release();

        private:
            FrameGraphNodeRegistry* registry_{nullptr};
            std::shared_ptr<PluginState> state_{};
        };

        [[nodiscard]] PluginRegistration register_plugin_nodes(
            std::string plugin_id,
            std::vector<NodeFactoryDescriptor> descriptors);

        using HotReloadCallback = std::function<void(const PluginReloadEvent&)>;
        void add_hot_reload_listener(HotReloadCallback callback);

        [[nodiscard]] bool contains(std::string_view node_id) const noexcept;
        [[nodiscard]] const NodeDescriptor* find(std::string_view node_id) const noexcept;
        [[nodiscard]] std::unique_ptr<INode> create(std::string_view node_id) const;

    private:
        struct NodeEntry
        {
            NodeDescriptor descriptor;
            std::function<std::unique_ptr<INode>()> factory;
            std::string origin;
        };

        struct PluginState
        {
            std::string plugin_id;
            bool active{true};
        };

        void register_node(NodeFactoryDescriptor descriptor, std::string_view origin);
        void remove_nodes_from_origin(std::string_view origin);
        void notify_listeners(PluginReloadEvent event);
        void deactivate_plugin_state(const std::shared_ptr<PluginState>& state);

        std::unordered_map<std::string, NodeEntry, TransparentStringHash, std::equal_to<>> nodes_{};
        std::unordered_map<std::string,
            std::unordered_set<std::string, TransparentStringHash, std::equal_to<>>,
            TransparentStringHash,
            std::equal_to<>> nodes_by_origin_{};
        std::unordered_map<std::string, std::weak_ptr<PluginState>, TransparentStringHash, std::equal_to<>> plugin_states_{};
        std::vector<HotReloadCallback> reload_listeners_{};
    };
} // namespace engine::rendering

