#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace engine::tools::imgui
{
    /// Rendering context forwarded to panel callbacks. Extend as runtime exposes more state.
    struct PanelRenderContext
    {
        double delta_time{0.0};
    };

    using PanelRenderCallback = std::function<void(const PanelRenderContext&)>;

    /// Registry that stores Dear ImGui panels by identifier for reuse across tooling surfaces.
    class PanelRegistry
    {
    public:
        /// Register \p callback under \p identifier. Returns false when the identifier already exists.
        bool register_panel(std::string identifier, PanelRenderCallback callback);

        /// Remove the panel associated with \p identifier.
        void unregister_panel(std::string_view identifier) noexcept;

        /// Check whether a panel has been registered under \p identifier.
        [[nodiscard]] bool contains(std::string_view identifier) const noexcept;

        /// Invoke the panel callback for \p identifier if present.
        void render(std::string_view identifier, const PanelRenderContext& context) const;

        /// Invoke all registered panels in deterministic registration order.
        void render_all(const PanelRenderContext& context) const;

        /// Return the identifiers in deterministic registration order.
        [[nodiscard]] std::vector<std::string> identifiers() const;

    private:
        std::unordered_map<std::string, PanelRenderCallback> panels_{};
        std::vector<std::string> insertion_order_{};
    };
}
