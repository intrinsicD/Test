#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/api.hpp"
#include "engine/rendering/frame_graph_types.hpp"
#include "engine/scene/selection/selection_engine.hpp"
#include "engine/scene/selection/visualization/outline_config.hpp"

namespace engine::scene
{
    class Scene;
}

namespace engine::rendering
{
    class FrameGraph;

    struct OutlineContext
    {
        FrameGraphResourceHandle color;
        FrameGraphResourceHandle depth;
        scene::selection::SelectionEngine** selection_engine{nullptr};
        scene::selection::visualization::OutlineConfig* config{nullptr};
        bool* enabled{nullptr};
        scene::selection::visualization::OutlineQuality quality{
            scene::selection::visualization::OutlineQuality::Balanced};
    };

    class SelectionOutlineStrategy
    {
    public:
        virtual ~SelectionOutlineStrategy() = default;

        virtual void add_pass(FrameGraph& graph, const OutlineContext& context) = 0;
        [[nodiscard]] virtual bool supports_mode(
            scene::selection::visualization::ThicknessMode mode) const = 0;
        [[nodiscard]] virtual bool supports_quality(
            scene::selection::visualization::OutlineQuality quality) const = 0;
        [[nodiscard]] virtual const char* name() const = 0;
    };

    class ENGINE_RENDERING_API SelectionOutlineStrategyFactory
    {
    public:
        static std::unique_ptr<SelectionOutlineStrategy> create(
            scene::selection::visualization::OutlineQuality quality,
            scene::selection::visualization::ThicknessMode mode);

        static std::unique_ptr<SelectionOutlineStrategy> create_by_name(std::string_view name);

        static std::vector<std::string> available_strategies();
    };
} // namespace engine::rendering
