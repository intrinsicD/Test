#include "engine/scene/selection/selection_engine.hpp"

#include <algorithm>
#include <utility>

namespace engine::scene::selection
{
    namespace
    {
        template <typename T, typename Predicate>
        void erase_if(std::vector<T>& container, Predicate&& predicate)
        {
            const auto it = std::remove_if(container.begin(), container.end(), std::forward<Predicate>(predicate));
            container.erase(it, container.end());
        }
    } // namespace

    SelectionEngine::SelectionEngine() = default;

    void SelectionEngine::register_strategy(StrategyPtr strategy, int priority)
    {
        if (!strategy)
        {
            return;
        }

        strategies_.push_back(StrategyEntry{priority, std::move(strategy)});
        std::sort(strategies_.begin(), strategies_.end(), [](const StrategyEntry& lhs, const StrategyEntry& rhs) {
            return lhs.priority < rhs.priority;
        });
    }

    std::optional<SelectionEvent> SelectionEngine::pick(const SelectionContext& context, SelectionSource source)
    {
        for (const auto& entry : strategies_)
        {
            const SelectionHit hit = entry.strategy->try_pick(context);
            if (hit.valid())
            {
                SelectionEvent event{};
                event.hit = hit;
                event.source = source;
                event.sequence = ++sequence_counter_;
                push_selection(event);
                return event;
            }
        }

        return std::nullopt;
    }

    void SelectionEngine::push_selection(SelectionEvent event)
    {
        if (!event.hit.valid())
        {
            return;
        }

        if (event.sequence == 0)
        {
            event.sequence = ++sequence_counter_;
        }

        if (deduplicate_)
        {
            erase_if(selection_history_, [&](const SelectionEvent& existing) {
                return existing.hit.entity == event.hit.entity;
            });
        }

        selection_history_.push_back(event);
        if (selection_history_.size() > max_history_)
        {
            const auto excess = selection_history_.size() - max_history_;
            selection_history_.erase(selection_history_.begin(), selection_history_.begin() + static_cast<std::ptrdiff_t>(excess));
        }

        notify_listeners(selection_history_.back());
    }

    void SelectionEngine::clear_selection()
    {
        selection_history_.clear();
    }

    std::span<const SelectionEvent> SelectionEngine::ordered_selection() const noexcept
    {
        return selection_history_;
    }

    void SelectionEngine::set_max_history(std::size_t max_history) noexcept
    {
        max_history_ = std::max<std::size_t>(1, max_history);
        if (selection_history_.size() > max_history_)
        {
            selection_history_.erase(selection_history_.begin(), selection_history_.begin() + static_cast<std::ptrdiff_t>(selection_history_.size() - max_history_));
        }
    }

    void SelectionEngine::set_deduplicate(bool deduplicate) noexcept
    {
        deduplicate_ = deduplicate;
    }

    SelectionEngine::ListenerId SelectionEngine::add_listener(SelectionChangedCallback callback)
    {
        if (!callback)
        {
            return 0;
        }

        const ListenerId id = next_listener_id_++;
        listeners_.push_back(ListenerEntry{id, std::move(callback)});
        return id;
    }

    void SelectionEngine::remove_listener(ListenerId id)
    {
        erase_if(listeners_, [id](const ListenerEntry& entry) {
            return entry.id == id;
        });
    }

    void SelectionEngine::notify_listeners(const SelectionEvent& event)
    {
        for (const auto& listener : listeners_)
        {
            if (listener.callback)
            {
                listener.callback(event);
            }
        }
    }
} // namespace engine::scene::selection
