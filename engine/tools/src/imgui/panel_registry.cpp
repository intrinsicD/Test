#include "engine/tools/imgui/panel_registry.hpp"

#include <algorithm>
#include <iterator>

namespace engine::tools::imgui
{
    bool PanelRegistry::register_panel(std::string identifier, PanelRenderCallback callback)
    {
        if (identifier.empty() || callback == nullptr)
        {
            return false;
        }
        auto [it, inserted] = panels_.try_emplace(std::move(identifier), PanelEntry{});
        if (!inserted)
        {
            return false;
        }
        insertion_order_.push_back(it->first);
        auto order_it = std::prev(insertion_order_.end());
        it->second.callback = std::move(callback);
        it->second.order_iterator = order_it;
        return true;
    }

    void PanelRegistry::unregister_panel(std::string_view identifier) noexcept
    {
        const auto it = panels_.find(identifier);
        if (it == panels_.end())
        {
            return;
        }
        insertion_order_.erase(it->second.order_iterator);
        panels_.erase(it);
    }

    bool PanelRegistry::contains(std::string_view identifier) const noexcept
    {
        return panels_.find(identifier) != panels_.end();
    }

    void PanelRegistry::render(std::string_view identifier, const PanelRenderContext& context) const
    {
        const auto it = panels_.find(identifier);
        if (it == panels_.end())
        {
            return;
        }
        if (it->second.callback != nullptr)
        {
            it->second.callback(context);
        }
    }

    void PanelRegistry::render_all(const PanelRenderContext& context) const
    {
        for (const auto& identifier : insertion_order_)
        {
            const auto it = panels_.find(identifier);
            if (it == panels_.end())
            {
                continue;
            }
            if (it->second.callback != nullptr)
            {
                it->second.callback(context);
            }
        }
    }

    std::vector<std::string> PanelRegistry::identifiers() const
    {
        return {insertion_order_.begin(), insertion_order_.end()};
    }
}
