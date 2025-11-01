#include <gtest/gtest.h>

#include <string>
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
    ASSERT_TRUE(registry.contains("metrics"));

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
