#include "engine/rendering/selection_outline_renderer.hpp"

#include <utility>

#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/selection_outline_strategy.hpp"

namespace engine::rendering
{
    void SelectionOutlineRenderer::set_selection_engine(scene::selection::SelectionEngine* engine) noexcept
    {
        selection_engine_ = engine;
    }

    scene::selection::SelectionEngine* SelectionOutlineRenderer::selection_engine() const noexcept
    {
        return selection_engine_;
    }

    scene::selection::visualization::OutlineConfig& SelectionOutlineRenderer::config() noexcept
    {
        return config_;
    }

    const scene::selection::visualization::OutlineConfig& SelectionOutlineRenderer::config() const noexcept
    {
        return config_;
    }

    void SelectionOutlineRenderer::set_enabled(bool enabled) noexcept
    {
        enabled_ = enabled;
    }

    bool SelectionOutlineRenderer::enabled() const noexcept
    {
        return enabled_;
    }

    void SelectionOutlineRenderer::add_pass(FrameGraph& graph,
                                            FrameGraphResourceHandle color,
                                            FrameGraphResourceHandle depth)
    {
        if (!color.valid() || !depth.valid())
        {
            return;
        }

        ensure_strategy();
        if (strategy_ == nullptr)
        {
            return;
        }

        OutlineContext context{};
        context.color = color;
        context.depth = depth;
        context.selection_engine = &selection_engine_;
        context.config = &config_;
        context.enabled = &enabled_;
        context.quality = config_.quality;

        strategy_->add_pass(graph, context);
    }

    void SelectionOutlineRenderer::set_strategy_override(std::string name)
    {
        strategy_override_ = std::move(name);
        strategy_.reset();
    }

    const std::string& SelectionOutlineRenderer::strategy_override() const noexcept
    {
        return strategy_override_;
    }

    std::string_view SelectionOutlineRenderer::strategy_name() const noexcept
    {
        return strategy_name_;
    }

    std::vector<std::string> SelectionOutlineRenderer::available_strategies()
    {
        return SelectionOutlineStrategyFactory::available_strategies();
    }

    void SelectionOutlineRenderer::ensure_strategy()
    {
        const auto desired_mode = config_.thickness_mode;
        const auto desired_quality = config_.quality;

        if (!strategy_override_.empty())
        {
            if (!strategy_ || strategy_name_ != strategy_override_)
            {
                auto candidate = SelectionOutlineStrategyFactory::create_by_name(strategy_override_);
                if (candidate)
                {
                    strategy_ = std::move(candidate);
                    strategy_name_ = strategy_override_;
                }
                else
                {
                    strategy_.reset();
                    strategy_name_.clear();
                }
            }
            return;
        }

        if (strategy_ && strategy_->supports_mode(desired_mode) && strategy_->supports_quality(desired_quality))
        {
            return;
        }

        auto candidate = SelectionOutlineStrategyFactory::create(desired_quality, desired_mode);
        if (candidate)
        {
            strategy_name_ = candidate->name();
            strategy_ = std::move(candidate);
        }
    }
} // namespace engine::rendering
