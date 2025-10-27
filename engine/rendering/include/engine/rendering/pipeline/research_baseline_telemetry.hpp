#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/rendering/render_pass.hpp"

namespace engine::rendering
{
    enum class ResearchOverlay
    {
        Normals = 0,
        Uv,
        Material,
        LightVolume,
        Count,
    };

    struct ResearchBaselinePassTelemetry
    {
        std::string name{};
        PassPhase phase{PassPhase::Unknown};
        std::uint64_t invocation_count{0};
        std::uint64_t total_draw_calls{0};
        std::uint64_t last_draw_calls{0};
        double last_gpu_time_ms{0.0};
        double max_gpu_time_ms{0.0};
    };

    struct ResearchBaselineTelemetrySnapshot
    {
        ResearchShadingMode active_mode{ResearchShadingMode::Deferred};
        std::array<std::uint64_t, 2> mode_selection_counts{0, 0};
        std::array<bool, static_cast<std::size_t>(ResearchOverlay::Count)> overlays_enabled{};
        std::array<std::uint64_t, static_cast<std::size_t>(ResearchOverlay::Count)> overlay_selection_counts{0, 0, 0, 0};
        std::vector<ResearchBaselinePassTelemetry> passes{};
    };

    class ResearchBaselineTelemetry
    {
    public:
        static ResearchBaselineTelemetry& instance() noexcept;

        void set_shading_mode(ResearchShadingMode mode) noexcept;
        void set_overlays(const ResearchBaselineOptions& options) noexcept;
        void record_pass(std::string_view name,
                         PassPhase phase,
                         std::uint64_t draw_calls,
                         double gpu_time_ms) noexcept;
        [[nodiscard]] ResearchBaselineTelemetrySnapshot snapshot() const noexcept;
        void reset_for_testing() noexcept;

    private:
        ResearchBaselineTelemetry() = default;

        using PassMap = std::map<std::string, ResearchBaselinePassTelemetry, std::less<>>;

        static std::size_t shading_mode_index(ResearchShadingMode mode) noexcept;
        static std::size_t overlay_index(ResearchOverlay overlay) noexcept;
        static void update_overlay(std::array<bool, static_cast<std::size_t>(ResearchOverlay::Count)>& overlays,
                                   std::array<std::uint64_t, static_cast<std::size_t>(ResearchOverlay::Count)>& counts,
                                   ResearchOverlay overlay,
                                   bool enabled) noexcept;

        mutable std::mutex mutex_{};
        ResearchShadingMode active_mode_{ResearchShadingMode::Deferred};
        std::array<std::uint64_t, 2> mode_selection_counts_{0, 0};
        std::array<bool, static_cast<std::size_t>(ResearchOverlay::Count)> overlays_enabled_{};
        std::array<std::uint64_t, static_cast<std::size_t>(ResearchOverlay::Count)> overlay_selection_counts_{0, 0, 0, 0};
        PassMap passes_{};
    };
}

