#include "engine/tools/imgui/panel_registry.hpp"

#include <algorithm>
#include <iterator>

namespace engine::tools::imgui
{
    PanelRegistry::RegistrationHandle::RegistrationHandle(
        PanelRegistry* registry,
        std::string identifier
    ) noexcept
        : registry_(registry)
        , identifier_(std::move(identifier))
    {
    }

    PanelRegistry::RegistrationHandle::RegistrationHandle(RegistrationHandle&& other) noexcept
        : registry_(other.registry_)
        , identifier_(std::move(other.identifier_))
    {
        other.registry_ = nullptr;
        other.identifier_.clear();
    }

    PanelRegistry::RegistrationHandle& PanelRegistry::RegistrationHandle::operator=(RegistrationHandle&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        release();

        registry_ = other.registry_;
        identifier_ = std::move(other.identifier_);

        other.registry_ = nullptr;
        other.identifier_.clear();

        return *this;
    }

    PanelRegistry::RegistrationHandle::~RegistrationHandle()
    {
        release();
    }

    bool PanelRegistry::RegistrationHandle::is_valid() const noexcept
    {
        return registry_ != nullptr;
    }

    void PanelRegistry::RegistrationHandle::release() noexcept
    {
        if (!registry_)
        {
            return;
        }

        registry_->unregister_panel(identifier_);
        registry_ = nullptr;
        identifier_.clear();
    }

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

    PanelRegistry::RegistrationHandle PanelRegistry::register_scoped_panel(
        std::string identifier,
        PanelRenderCallback callback
    )
    {
        auto registration_identifier = identifier;

        if (!register_panel(std::move(identifier), std::move(callback)))
        {
            return {};
        }

        return RegistrationHandle{this, std::move(registration_identifier)};
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
