#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "engine/geometry/shapes/ray.hpp"
#include "engine/math/vector.hpp"
#include "engine/scene/scene.hpp"

namespace engine::scene::selection
{
    enum class SelectionSource
    {
        Cursor,
        Marquee,
        Script,
    };

    struct SelectionHit
    {
        entt::entity entity{entt::null};
        math::vec3 position{0.0F, 0.0F, 0.0F};
        float distance{std::numeric_limits<float>::infinity()};

        [[nodiscard]] bool valid() const noexcept
        {
            return entity != entt::null;
        }
    };

    struct SelectionEvent
    {
        SelectionHit hit{};
        SelectionSource source{SelectionSource::Cursor};
        std::uint64_t sequence{0};
    };

    struct SelectionContext
    {
        scene::Scene* scene{nullptr};
        geometry::Ray cursor_ray{};
    };

    class SelectionStrategy
    {
    public:
        virtual ~SelectionStrategy() = default;
        [[nodiscard]] virtual SelectionHit try_pick(const SelectionContext& context) const = 0;
    };

    class SelectionEngine
    {
    public:
        using StrategyPtr = std::unique_ptr<SelectionStrategy>;
        using SelectionChangedCallback = std::function<void(const SelectionEvent&)>;
        using ListenerId = std::size_t;

        SelectionEngine();

        void register_strategy(StrategyPtr strategy, int priority);

        [[nodiscard]] std::optional<SelectionEvent> pick(
            const SelectionContext& context,
            SelectionSource source = SelectionSource::Cursor
        );

        void push_selection(SelectionEvent event);
        void clear_selection();

        [[nodiscard]] std::span<const SelectionEvent> ordered_selection() const noexcept;

        void set_max_history(std::size_t max_history) noexcept;
        void set_deduplicate(bool deduplicate) noexcept;

        ListenerId add_listener(SelectionChangedCallback callback);
        void remove_listener(ListenerId id);

    private:
        struct StrategyEntry
        {
            int priority{0};
            StrategyPtr strategy{};
        };

        struct ListenerEntry
        {
            ListenerId id{0};
            SelectionChangedCallback callback{};
        };

        std::vector<StrategyEntry> strategies_{};
        std::vector<SelectionEvent> selection_history_{};
        std::vector<ListenerEntry> listeners_{};
        std::size_t max_history_{64};
        bool deduplicate_{true};
        std::uint64_t sequence_counter_{0};
        ListenerId next_listener_id_{1};

        void notify_listeners(const SelectionEvent& event);
    };
} // namespace engine::scene::selection
