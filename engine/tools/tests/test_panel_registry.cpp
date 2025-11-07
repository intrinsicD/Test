#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/tools/imgui/panel_registry.hpp"

namespace
{
    using engine::tools::imgui::PanelRegistry;
    using engine::tools::imgui::PanelRenderContext;
}

TEST(PanelRegistry, RegistersAndInvokesPanels)
{
    PanelRegistry registry{};
    int render_count = 0;
    ASSERT_TRUE(registry.register_panel("metrics", [&](const PanelRenderContext& ctx) {
        render_count += static_cast<int>(ctx.delta_time);
    }));
    const std::string identifier = "metrics";
    std::string_view identifier_view = identifier;
    ASSERT_TRUE(registry.contains(identifier_view));

    registry.render("metrics", PanelRenderContext{1.0});
    EXPECT_EQ(render_count, 1);

    registry.render_all(PanelRenderContext{2.0});
    EXPECT_EQ(render_count, 3);
}

TEST(PanelRegistry, RejectsDuplicateIdentifiers)
{
    PanelRegistry registry{};
    ASSERT_TRUE(registry.register_panel("diagnostics", [](const PanelRenderContext&) {}));
    EXPECT_FALSE(registry.register_panel("diagnostics", [](const PanelRenderContext&) {}));
}

TEST(PanelRegistry, UnregistersPanels)
{
    PanelRegistry registry{};
    ASSERT_TRUE(registry.register_panel("scene", [](const PanelRenderContext&) {}));
    registry.unregister_panel("scene");
    EXPECT_FALSE(registry.contains("scene"));
    registry.render("scene", PanelRenderContext{0.0});
}

TEST(PanelRegistry, RenderAllSkipsUnregisteredPanels)
{
    PanelRegistry registry{};
    int render_count = 0;
    ASSERT_TRUE(registry.register_panel("visible", [&](const PanelRenderContext&) { render_count += 1; }));
    ASSERT_TRUE(registry.register_panel("transient", [&](const PanelRenderContext&) { render_count += 100; }));

    registry.unregister_panel("transient");
    registry.render_all(PanelRenderContext{0.0});
    EXPECT_EQ(render_count, 1);
}

TEST(PanelRegistry, PreservesRegistrationOrder)
{
    PanelRegistry registry{};
    ASSERT_TRUE(registry.register_panel("first", [](const PanelRenderContext&) {}));
    ASSERT_TRUE(registry.register_panel("second", [](const PanelRenderContext&) {}));
    ASSERT_TRUE(registry.register_panel("third", [](const PanelRenderContext&) {}));

    const auto identifiers = registry.identifiers();
    ASSERT_EQ(identifiers.size(), 3U);
    EXPECT_EQ(identifiers[0], "first");
    EXPECT_EQ(identifiers[1], "second");
    EXPECT_EQ(identifiers[2], "third");
}

TEST(PanelRegistry, RegistrationHandleUnregistersOnScopeExit)
{
    PanelRegistry registry{};

    {
        auto handle = registry.register_scoped_panel("scoped", [](const PanelRenderContext&) {});
        ASSERT_TRUE(handle);
        EXPECT_TRUE(registry.contains("scoped"));
    }

    EXPECT_FALSE(registry.contains("scoped"));
}

TEST(PanelRegistry, RegistrationHandleSupportsMoveSemantics)
{
    PanelRegistry registry{};

    auto handle = registry.register_scoped_panel("movable", [](const PanelRenderContext&) {});
    ASSERT_TRUE(handle);

    PanelRegistry::RegistrationHandle moved_handle = std::move(handle);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_TRUE(registry.contains("movable"));

    moved_handle.release();
    EXPECT_FALSE(moved_handle.is_valid());
    EXPECT_FALSE(registry.contains("movable"));
}
