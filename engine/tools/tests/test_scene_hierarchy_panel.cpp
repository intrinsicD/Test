#include <gtest/gtest.h>

#include <vector>

#include "engine/scene/scene.hpp"
#include "engine/scene/selection/selection_engine.hpp"
#include "engine/tools/editor/scene_hierarchy_panel.hpp"
#include "engine/tools/imgui/panel_registry.hpp"

namespace
{
    using engine::scene::Scene;
    using engine::tools::editor::HierarchyPanelModel;
    using engine::tools::editor::SceneHierarchyPanel;
    using engine::tools::editor::register_scene_hierarchy_panel;
    using engine::tools::imgui::PanelRegistry;
    namespace selection = engine::scene::selection;
}

TEST(SceneHierarchyPanel, TraversesRootsAndChildrenDeterministically)
{
    Scene scene{};
    auto root_a = scene.create_entity("RootA");
    auto root_b = scene.create_entity("RootB");
    auto child_a = scene.create_entity("ChildA");
    auto child_b = scene.create_entity("ChildB");

    scene.set_parent(child_a, root_a);
    scene.set_parent(child_b, root_a);

    HierarchyPanelModel model{&scene};

    std::vector<entt::entity> roots;
    model.for_each_root([&](entt::entity entity) {
        roots.push_back(entity);
    });

    ASSERT_EQ(roots.size(), 2U);
    EXPECT_EQ(roots[0], root_b.id());
    EXPECT_EQ(roots[1], root_a.id());

    std::vector<entt::entity> children;
    model.for_each_child(root_a.id(), [&](entt::entity entity) {
        children.push_back(entity);
    });

    ASSERT_EQ(children.size(), 2U);
    EXPECT_EQ(children[0], child_b.id());
    EXPECT_EQ(children[1], child_a.id());
}

TEST(SceneHierarchyPanel, SynchronizeSelectionClearsDestroyedEntities)
{
    Scene scene{};
    auto entity = scene.create_entity("Transient");

    HierarchyPanelModel model{&scene};
    model.set_selection(entity.id());
    ASSERT_EQ(model.selection(), entity.id());

    scene.destroy_entity(entity);

    model.synchronize_selection(entity.id());
    EXPECT_EQ(model.selection(), entt::null);
}

TEST(SceneHierarchyPanel, RegistrationHandleUnregistersPanel)
{
    PanelRegistry registry{};
    SceneHierarchyPanel panel{};

    {
        auto handle = register_scene_hierarchy_panel(registry, panel, "tools.scene_hierarchy");
        EXPECT_TRUE(handle.is_valid());
        EXPECT_TRUE(registry.contains("tools.scene_hierarchy"));
    }

    EXPECT_FALSE(registry.contains("tools.scene_hierarchy"));
}

TEST(SceneHierarchyPanel, PropagatesSelectionToSelectionEngine)
{
    Scene scene{};
    auto entity = scene.create_entity("Selectable");

    SceneHierarchyPanel panel{};
    panel.set_scene(&scene);

    selection::SelectionEngine engine{};
    panel.bind_selection_engine(&engine, selection::SelectionSource::Script);
    panel.set_selection(entity.id());

    const auto history = engine.ordered_selection();
    ASSERT_EQ(history.size(), 1U);
    EXPECT_EQ(history.back().hit.entity, entity.id());
    EXPECT_EQ(history.back().source, selection::SelectionSource::Script);
}

TEST(SceneHierarchyPanel, SynchronizesSelectionFromSelectionEngine)
{
    Scene scene{};
    auto entity = scene.create_entity("Synced");

    SceneHierarchyPanel panel{};
    panel.set_scene(&scene);

    selection::SelectionEngine engine{};
    panel.bind_selection_engine(&engine, selection::SelectionSource::Cursor);

    selection::SelectionEvent event{};
    event.hit.entity = entity.id();
    event.hit.distance = 0.25F;
    engine.push_selection(event);

    EXPECT_EQ(panel.model().selection(), entity.id());
}

