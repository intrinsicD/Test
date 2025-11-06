#include "engine/rendering/frame_graph_registry.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace engine::rendering
{
    namespace
    {
        class TestNode final : public INode
        {
        public:
            explicit TestNode(NodeDescriptor descriptor)
                : descriptor_{std::move(descriptor)}
            {
            }

            [[nodiscard]] const NodeDescriptor& Reflect() const override
            {
                return descriptor_;
            }

            void Compile(NodeContext&) override
            {
            }

            void Execute(NodeContext&) override
            {
            }

        private:
            NodeDescriptor descriptor_{};
        };

        [[nodiscard]] NodeFactoryDescriptor make_factory(std::string id)
        {
            NodeDescriptor descriptor{};
            descriptor.id = std::move(id);
            return NodeFactoryDescriptor{
                descriptor,
                [descriptor]() mutable -> std::unique_ptr<INode>
                {
                    return std::make_unique<TestNode>(descriptor);
                },
            };
        }
    } // namespace

    TEST(FrameGraphNodeRegistry, RegistersBuiltinFactories)
    {
        FrameGraphNodeRegistry registry{};
        registry.register_builtin(make_factory("rendering.test.node"));

        EXPECT_TRUE(registry.contains("rendering.test.node"));

        auto instance = registry.create("rendering.test.node");
        EXPECT_EQ(instance->Reflect().id, "rendering.test.node");
    }

    TEST(FrameGraphNodeRegistry, RejectsDuplicateRegistration)
    {
        FrameGraphNodeRegistry registry{};
        registry.register_builtin(make_factory("rendering.test.node"));

        EXPECT_THROW(registry.register_builtin(make_factory("rendering.test.node")), std::invalid_argument);
    }

    TEST(FrameGraphNodeRegistry, PluginRegistrationNotifiesLifecycle)
    {
        FrameGraphNodeRegistry registry{};
        std::vector<std::pair<PluginReloadEvent::Phase, std::string>> events;
        registry.add_hot_reload_listener(
            [&](const PluginReloadEvent& event) { events.emplace_back(event.phase, std::string(event.plugin_id)); });

        {
            auto handle = registry.register_plugin_nodes("plugin.example", {make_factory("plugin.node")});
            EXPECT_TRUE(registry.contains("plugin.node"));
            ASSERT_EQ(events.size(), 1U);
            EXPECT_EQ(events.front().first, PluginReloadEvent::Phase::Loaded);
            EXPECT_EQ(events.front().second, "plugin.example");
            (void)handle;
        }

        EXPECT_FALSE(registry.contains("plugin.node"));
        ASSERT_EQ(events.size(), 2U);
        EXPECT_EQ(events.back().first, PluginReloadEvent::Phase::Unloading);
        EXPECT_EQ(events.back().second, "plugin.example");
    }

    TEST(FrameGraphNodeRegistry, PluginReloadReplacesNodesWithoutDoubleUnload)
    {
        FrameGraphNodeRegistry registry{};
        std::vector<std::pair<PluginReloadEvent::Phase, std::string>> events;
        registry.add_hot_reload_listener(
            [&](const PluginReloadEvent& event) { events.emplace_back(event.phase, std::string(event.plugin_id)); });

        auto first_descriptor = make_factory("plugin.node");
        auto second_descriptor = make_factory("plugin.node");
        second_descriptor.descriptor.tags.push_back("updated");

        auto first = registry.register_plugin_nodes("plugin.reload", {std::move(first_descriptor)});
        EXPECT_TRUE(registry.contains("plugin.node"));
        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].first, PluginReloadEvent::Phase::Loaded);

        auto second = registry.register_plugin_nodes("plugin.reload", {std::move(second_descriptor)});
        EXPECT_TRUE(registry.contains("plugin.node"));
        ASSERT_EQ(events.size(), 3U);
        EXPECT_EQ(events[1].first, PluginReloadEvent::Phase::Unloading);
        EXPECT_EQ(events[2].first, PluginReloadEvent::Phase::Loaded);

        const auto* descriptor = registry.find("plugin.node");
        ASSERT_NE(descriptor, nullptr);
        ASSERT_EQ(descriptor->tags.size(), 1U);
        EXPECT_EQ(descriptor->tags.front(), "updated");

        first.release();
        EXPECT_EQ(events.size(), 3U);

        second.release();
        EXPECT_EQ(events.size(), 4U);
        EXPECT_EQ(events.back().first, PluginReloadEvent::Phase::Unloading);
    }
}

