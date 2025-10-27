#pragma once

#include "engine/tools/api.hpp"

// Forward declare ImGui types to avoid pulling in the entire header
struct ImGuiContext;

namespace engine::runtime
{
    struct RuntimeDiagnostics;
}

namespace engine::scene::validation
{
    struct HierarchyValidationReport;
}

namespace engine::tools::imgui
{
    // ImGui frame management
    ENGINE_TOOLS_API void begin_frame();
    ENGINE_TOOLS_API void end_frame();

    // Render runtime diagnostics UI
    ENGINE_TOOLS_API void render_diagnostics(const runtime::RuntimeDiagnostics& diagnostics);

    // Render scene validation report UI
    ENGINE_TOOLS_API void render_validation_report(
        const scene::validation::HierarchyValidationReport& report);

    // Render profiling data
    ENGINE_TOOLS_API void render_profiler_window(bool* p_open = nullptr);
} // namespace engine::tools::imgui