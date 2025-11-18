#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/api.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/rendering/selection_outline_strategy.hpp"
#include "engine/scene/selection/selection_engine.hpp"
#include "engine/scene/selection/visualization/outline_config.hpp"

namespace engine::rendering
{
    class FrameGraph;

    /**
     * \brief High-level facade that wires selection outlines into a frame-graph.
     *
     * The renderer stores configuration and the selection engine pointer so passes
     * can sample the latest outline preferences without recompiling the graph.
     */
    class ENGINE_RENDERING_API SelectionOutlineRenderer
    {
    public:
        SelectionOutlineRenderer() = default;

        void set_selection_engine(scene::selection::SelectionEngine* engine) noexcept;
        [[nodiscard]] scene::selection::SelectionEngine* selection_engine() const noexcept;

        [[nodiscard]] scene::selection::visualization::OutlineConfig& config() noexcept;
        [[nodiscard]] const scene::selection::visualization::OutlineConfig& config() const noexcept;

        void set_enabled(bool enabled) noexcept;
        [[nodiscard]] bool enabled() const noexcept;

        void add_pass(FrameGraph& graph,
                      FrameGraphResourceHandle color,
                      FrameGraphResourceHandle depth);

        void set_strategy_override(std::string name);
        [[nodiscard]] const std::string& strategy_override() const noexcept;
        [[nodiscard]] std::string_view strategy_name() const noexcept;
        static std::vector<std::string> available_strategies();

    private:
        void ensure_strategy();

        scene::selection::SelectionEngine* selection_engine_{nullptr};
        scene::selection::visualization::OutlineConfig config_{};
        bool enabled_{true};
        std::unique_ptr<SelectionOutlineStrategy> strategy_{};
        std::string strategy_name_{};
        std::string strategy_override_{};
    };
} // namespace engine::rendering
