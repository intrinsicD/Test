#include <gtest/gtest.h>

#include "engine/scene/api.hpp"
#include "engine/scene/scene.hpp"

TEST(SceneModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::scene::module_name(), "scene");
    EXPECT_STREQ(engine_scene_module_name(), "scene");
}

namespace {

struct Position {
    float x{};
    float y{};
    float z{};
};

}  // namespace


TEST(Scene, CreateAndManipulateEntity) {
    engine::scene::Scene scene{"test"};
    EXPECT_EQ(scene.name(), "test");

    auto entity = scene.create_entity();
    EXPECT_TRUE(entity.valid());
    EXPECT_TRUE(scene.valid(entity.id()));

    auto& position = entity.emplace<Position>(1.0F, 2.0F, 3.0F);
    EXPECT_FLOAT_EQ(position.x, 1.0F);
    EXPECT_FLOAT_EQ(position.y, 2.0F);
    EXPECT_FLOAT_EQ(position.z, 3.0F);

    const auto& const_position = entity.get<Position>();
    EXPECT_FLOAT_EQ(const_position.x, 1.0F);

    EXPECT_TRUE(entity.has<Position>());

    auto view = scene.view<Position>();
    ASSERT_EQ(view.size(), 1);

    std::size_t count = 0;
    for (auto [id, pos] : view.each()) {
        EXPECT_EQ(id, entity.id());
        EXPECT_FLOAT_EQ(pos.y, 2.0F);
        ++count;
    }
    EXPECT_EQ(count, 1U);

    entity.remove<Position>();
    EXPECT_FALSE(entity.has<Position>());

    entity.destroy();
    EXPECT_FALSE(entity.valid());
}
