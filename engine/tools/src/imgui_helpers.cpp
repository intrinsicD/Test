#include "engine/tools/imgui_helpers.hpp"
#include "engine/tools/profiling/profiler.hpp"
#include "engine/runtime/api.hpp"
#include "engine/scene/validation.hpp"

#include <imgui.h>
#include <cstdint>

namespace engine::tools::imgui
{
    void begin_frame()
    {
        ImGui::NewFrame();
    }

    void end_frame()
    {
        ImGui::Render();
    }

    void render_diagnostics(const engine::runtime::RuntimeDiagnostics& diagnostics)
    {
        if (ImGui::Begin("Runtime Diagnostics"))
        {
            // Lifecycle counters
            if (ImGui::CollapsingHeader("Lifecycle", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Initialize Count: %lu", diagnostics.initialize_count);
                ImGui::Text("Tick Count: %lu", diagnostics.tick_count);
                ImGui::Text("Shutdown Count: %lu", diagnostics.shutdown_count);
                ImGui::Separator();
                ImGui::Text("Last Tick: %.3f ms", diagnostics.last_tick_ms);
                ImGui::Text("Average Tick: %.3f ms", diagnostics.average_tick_ms);
                ImGui::Text("Max Tick: %.3f ms", diagnostics.max_tick_ms);
            }

            // Streaming metrics
            if (ImGui::CollapsingHeader("Streaming"))
            {
                ImGui::Text("Worker Count: %zu", diagnostics.streaming.worker_count);
                ImGui::Text("Queue Capacity: %zu", diagnostics.streaming.queue_capacity);
                ImGui::Text("Pending Tasks: %zu", diagnostics.streaming.pending_tasks);
                ImGui::Text("Active Workers: %zu", diagnostics.streaming.active_workers);
                ImGui::Separator();
                ImGui::Text("Total Enqueued: %lu", diagnostics.streaming.total_enqueued);
                ImGui::Text("Total Executed: %lu", diagnostics.streaming.total_executed);
                ImGui::Text("Streaming Pending: %lu", diagnostics.streaming.streaming_pending);
                ImGui::Text("Streaming Loading: %lu", diagnostics.streaming.streaming_loading);
                ImGui::Text("Total Requests: %lu", diagnostics.streaming.streaming_total_requests);
                ImGui::Text("Total Completed: %lu", diagnostics.streaming.streaming_total_completed);
                ImGui::Text("Total Failed: %lu", diagnostics.streaming.streaming_total_failed);
                ImGui::Text("Total Cancelled: %lu", diagnostics.streaming.streaming_total_cancelled);
            }

            // Scene validation
            if (ImGui::CollapsingHeader("Scene Validation"))
            {
                const auto& validation = diagnostics.scene_validation;
                const auto& metrics = validation.metrics;

                ImGui::Text("OK: %s", validation.ok() ? "Yes" : "No");
                ImGui::Text("Total Issues: %zu", metrics.issue_count);
                ImGui::Text("Cycles: %zu", metrics.cycle_count);
                ImGui::Text("Dangling Parents: %zu", metrics.dangling_parent_count);
                ImGui::Text("Missing Hierarchies: %zu", metrics.missing_parent_hierarchy_count);
                ImGui::Text("Non-Finite Transforms: %zu", metrics.non_finite_transform_count);
                ImGui::Separator();
                ImGui::Text("Alert Level: %d", static_cast<int>(diagnostics.scene_validation_alert_level));
                ImGui::Text("Consecutive Failures: %lu", diagnostics.scene_validation_consecutive_failure_frames);
            }

            // Stage timings
            if (ImGui::CollapsingHeader("Stage Timings"))
            {
                for (const auto& stage : diagnostics.stage_timings)
                {
                    ImGui::Text("%s: %.3f ms (avg: %.3f, max: %.3f)",
                                stage.name.c_str(),
                                stage.last_ms,
                                stage.average_ms,
                                stage.max_ms);
                }
            }
        }
        ImGui::End();
    }

    void render_validation_report(
        const engine::scene::validation::HierarchyValidationReport& report)
    {
        const auto& metrics = report.metrics;

        ImGui::Text("OK: %s", report.ok() ? "Yes" : "No");
        ImGui::Text("Total Issues: %zu", metrics.issue_count);
        ImGui::Text("Cycles: %zu", metrics.cycle_count);
        ImGui::Text("Dangling Parents: %zu", metrics.dangling_parent_count);
        ImGui::Text("Missing Hierarchies: %zu", metrics.missing_parent_hierarchy_count);

        if (!report.issues.empty())
        {
            ImGui::Separator();
            ImGui::Text("Issues:");

            for (size_t i = 0; i < report.issues.size() && i < 10; ++i)
            {
                const auto& issue = report.issues[i];
                ImGui::Text("  %s: %s",
                            engine::scene::validation::to_string(issue.type).data(),
                            issue.message.c_str());
            }

            if (report.issues.size() > 10)
            {
                ImGui::Text("  ... and %zu more issues", report.issues.size() - 10);
            }
        }
    }

    void render_profiler_window(bool* p_open)
    {
        if (!ImGui::Begin("Profiler", p_open))
        {
            ImGui::End();
            return;
        }

        auto& profiler = profiling::global_profiler();
        auto report = profiler.generate_report();

        ImGui::Text("Total Duration: %.3f ms", report.total_duration_ms);
        ImGui::Separator();

        if (ImGui::BeginTable("ProfileData", 5,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Calls");
            ImGui::TableSetupColumn("Total (ms)");
            ImGui::TableSetupColumn("Average (ms)");
            ImGui::TableSetupColumn("Min / Max (ms)");
            ImGui::TableHeadersRow();

            for (const auto& entry : report.entries)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%lu", entry.call_count);
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", entry.duration_ms);
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", entry.average_ms);
                ImGui::TableNextColumn();
                ImGui::Text("%.3f / %.3f", entry.min_ms, entry.max_ms);
            }

            ImGui::EndTable();
        }

        if (ImGui::Button("Reset"))
        {
            profiler.reset();
        }

        ImGui::End();
    }
} // namespace engine::tools::imgui