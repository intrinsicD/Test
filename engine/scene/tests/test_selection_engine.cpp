#include <gtest/gtest.h>

#include <optional>

#include "engine/geometry/shapes/ray.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/selection/bounding_box_strategy.hpp"
#include "engine/scene/selection/selection_engine.hpp"

namespace
{
    class StubStrategy final : public engine::scene::selection::SelectionStrategy
    {
    public:
        explicit StubStrategy(std::optional<entt::entity> entity) : entity_(entity) {}

        [[nodiscard]] engine::scene::selection::SelectionHit try_pick(
            const engine::scene::selection::SelectionContext&
        ) const override
        {
            engine::scene::selection::SelectionHit hit{};
            if (entity_)
            {
                hit.entity = *entity_;
                hit.distance = 1.0F;
                hit.position = {1.0F, 0.0F, 0.0F};
            }
            return hit;
        }

    private:
        std::optional<entt::entity> entity_{};
    };
}

TEST(SelectionEngine, RespectsStrategyPriorityOrder)
{
    engine::scene::Scene scene;
    const auto preferred = scene.create_entity().id();
    const auto fallback = scene.create_entity().id();

    engine::scene::selection::SelectionEngine engine;
    engine.register_strategy(std::make_unique<StubStrategy>(fallback), 2);
    engine.register_strategy(std::make_unique<StubStrategy>(preferred), 1);

    engine::scene::selection::SelectionContext context{};
    context.scene = &scene;
    context.cursor_ray = engine::geometry::Ray{{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};

    const auto result = engine.pick(context);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->hit.entity, preferred);
    EXPECT_EQ(engine.ordered_selection().size(), 1U);
}

TEST(SelectionEngine, DeduplicatesByDefault)
{
    engine::scene::selection::SelectionEngine engine;
    engine::scene::selection::SelectionEvent event{};
    event.hit.entity = static_cast<entt::entity>(42);
    event.hit.distance = 0.0F;

    engine.push_selection(event);
    engine.push_selection(event);
    ASSERT_EQ(engine.ordered_selection().size(), 1U);

    engine.set_deduplicate(false);
    engine.push_selection(event);
    EXPECT_EQ(engine.ordered_selection().size(), 2U);
}

TEST(BoundingBoxSelectionStrategy, FallsBackToWorldTransform)
{
    engine::scene::Scene scene;
    auto entity = scene.create_entity();
    auto& transform = scene.registry().emplace<engine::scene::components::WorldTransform>(entity.id());
    transform.value.translation = {0.0F, 0.0F, 0.0F};
    transform.value.scale = {2.0F, 2.0F, 2.0F};

    engine::scene::selection::BoundingBoxSelectionStrategy strategy;
    engine::scene::selection::SelectionContext context{};
    context.scene = &scene;
    context.cursor_ray = engine::geometry::Ray{{0.0F, 0.0F, -5.0F}, {0.0F, 0.0F, 1.0F}};

    const auto hit = strategy.try_pick(context);
    ASSERT_TRUE(hit.valid());
    EXPECT_EQ(hit.entity, entity.id());
    EXPECT_NEAR(hit.distance, 4.0F, 1e-3F);
}
