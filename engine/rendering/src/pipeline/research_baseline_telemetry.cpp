#include "engine/rendering/pipeline/research_baseline_telemetry.hpp"

#include <algorithm>
#include <utility>

namespace engine::rendering
{
    namespace
    {
        constexpr std::size_t kForwardIndex = 0;
        constexpr std::size_t kDeferredIndex = 1;
    }

    ResearchBaselineTelemetry& ResearchBaselineTelemetry::instance() noexcept
    {
        static ResearchBaselineTelemetry telemetry{};
        return telemetry;
    }

    std::size_t ResearchBaselineTelemetry::shading_mode_index(ResearchShadingMode mode) noexcept
    {
        return mode == ResearchShadingMode::Deferred ? kDeferredIndex : kForwardIndex;
    }

    std::size_t ResearchBaselineTelemetry::overlay_index(ResearchOverlay overlay) noexcept
    {
        return static_cast<std::size_t>(overlay);
    }

    void ResearchBaselineTelemetry::update_overlay(
        std::array<bool, static_cast<std::size_t>(ResearchOverlay::Count)>& overlays,
        std::array<std::uint64_t, static_cast<std::size_t>(ResearchOverlay::Count)>& counts,
        ResearchOverlay overlay,
        bool enabled) noexcept
    {
        const auto index = overlay_index(overlay);
        overlays[index] = enabled;
        if (enabled)
        {
            ++counts[index];
        }
    }

    void ResearchBaselineTelemetry::set_shading_mode(ResearchShadingMode mode) noexcept
    {
        std::scoped_lock lock{mutex_};
        active_mode_ = mode;
        const auto index = shading_mode_index(mode);
        ++mode_selection_counts_[index];
    }

    void ResearchBaselineTelemetry::set_overlays(const ResearchBaselineOptions& options) noexcept
    {
        std::scoped_lock lock{mutex_};
        overlays_enabled_.fill(false);
        update_overlay(overlays_enabled_, overlay_selection_counts_, ResearchOverlay::Normals,
                       options.enable_normals_overlay);
        update_overlay(overlays_enabled_, overlay_selection_counts_, ResearchOverlay::Uv,
                       options.enable_uv_overlay);
        update_overlay(overlays_enabled_, overlay_selection_counts_, ResearchOverlay::Material,
                       options.enable_material_overlay);
        update_overlay(overlays_enabled_, overlay_selection_counts_, ResearchOverlay::LightVolume,
                       options.enable_light_volume_overlay);
    }

    void ResearchBaselineTelemetry::record_pass(std::string_view name,
                                                PassPhase phase,
                                                std::uint64_t draw_calls,
                                                double gpu_time_ms) noexcept
    {
        const double clamped_time = std::max(gpu_time_ms, 0.0);
        std::scoped_lock lock{mutex_};

        auto [iterator, inserted] = passes_.try_emplace(std::string{name});
        auto& telemetry = iterator->second;
        telemetry.name = std::string{name};
        telemetry.phase = phase;
        telemetry.invocation_count += 1U;
        telemetry.total_draw_calls += draw_calls;
        telemetry.last_draw_calls = draw_calls;
        telemetry.last_gpu_time_ms = clamped_time;
        telemetry.max_gpu_time_ms = std::max(telemetry.max_gpu_time_ms, clamped_time);
    }

    ResearchBaselineTelemetrySnapshot ResearchBaselineTelemetry::snapshot() const noexcept
    {
        std::scoped_lock lock{mutex_};
        ResearchBaselineTelemetrySnapshot snapshot{};
        snapshot.active_mode = active_mode_;
        snapshot.mode_selection_counts = mode_selection_counts_;
        snapshot.overlays_enabled = overlays_enabled_;
        snapshot.overlay_selection_counts = overlay_selection_counts_;
        snapshot.passes.reserve(passes_.size());
        for (const auto& [name, telemetry] : passes_)
        {
            (void)name;
            snapshot.passes.push_back(telemetry);
        }
        return snapshot;
    }

    void ResearchBaselineTelemetry::reset_for_testing() noexcept
    {
        std::scoped_lock lock{mutex_};
        active_mode_ = ResearchShadingMode::Deferred;
        mode_selection_counts_.fill(0U);
        overlays_enabled_.fill(false);
        overlay_selection_counts_.fill(0U);
        passes_.clear();
    }
}

