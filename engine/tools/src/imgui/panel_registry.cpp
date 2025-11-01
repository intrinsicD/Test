#include "engine/tools/imgui/panel_registry.hpp"

#include <algorithm>

namespace engine::tools::imgui
{
    bool PanelRegistry::register_panel(std::string identifier, PanelRenderCallback callback)
    {
        if (identifier.empty() || callback == nullptr)
        {
            return false;
        }
        const auto [it, inserted] = panels_.emplace(identifier, std::move(callback));
        if (!inserted)
        {
            return false;
        }
        insertion_order_.push_back(it->first);
        return true;
    }

    void PanelRegistry::unregister_panel(std::string_view identifier) noexcept
    {
        const auto it = panels_.find(std::string{identifier});
        if (it == panels_.end())
        {
            return;
        }
        panels_.erase(it);
        insertion_order_.erase(
            std::remove(insertion_order_.begin(), insertion_order_.end(), identifier),
            insertion_order_.end());
    }

    bool PanelRegistry::contains(std::string_view identifier) const noexcept
    {
        return panels_.find(std::string{identifier}) != panels_.end();
    }

    void PanelRegistry::render(std::string_view identifier, const PanelRenderContext& context) const
    {
        const auto it = panels_.find(std::string{identifier});
        if (it == panels_.end())
        {
            return;
        }
        it->second(context);
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
            it->second(context);
        }
    }

    std::vector<std::string> PanelRegistry::identifiers() const
    {
        return insertion_order_;
    }
}
