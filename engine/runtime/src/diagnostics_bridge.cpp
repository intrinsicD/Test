#include "engine/runtime/diagnostics_bridge.hpp"

#include <algorithm>
#include <functional>
#include <utility>

#include <spdlog/spdlog.h>

namespace engine::runtime
{
    namespace
    {
        template <typename T>
        constexpr void hash_combine(std::size_t& seed, const T& value) noexcept
        {
            seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        }
    } // namespace

    DiagnosticsBridge& DiagnosticsBridge::instance() noexcept
    {
        static DiagnosticsBridge bridge;
        return bridge;
    }

    DiagnosticsBridge::CallbackId DiagnosticsBridge::register_hierarchy_callback(
        HierarchyReportCallback callback)
    {
        std::lock_guard lock{mutex_};
        const CallbackId id = next_callback_id_++;
        hierarchy_callbacks_.emplace_back(id, std::move(callback));
        return id;
    }

    void DiagnosticsBridge::unregister_hierarchy_callback(CallbackId id)
    {
        std::lock_guard lock{mutex_};
        auto it = std::remove_if(hierarchy_callbacks_.begin(), hierarchy_callbacks_.end(), [id](const auto& entry)
        {
            return entry.first == id;
        });
        hierarchy_callbacks_.erase(it, hierarchy_callbacks_.end());
    }

    void DiagnosticsBridge::publish_hierarchy_report(
        const scene::validation::HierarchyValidationReport& report,
        double simulation_time)
    {
        std::vector<HierarchyReportCallback> callbacks{};
        callbacks.reserve(hierarchy_callbacks_.size());

        const std::size_t signature = compute_report_signature(report);

        bool should_log_issues = false;
        bool should_log_resolution = false;
        std::size_t previous_issue_count = 0;

        {
            std::lock_guard lock{mutex_};
            for (const auto& entry : hierarchy_callbacks_)
            {
                callbacks.push_back(entry.second);
            }

            previous_issue_count = last_issue_count_;
            last_issue_count_ = report.metrics.issue_count;

            if (signature != last_signature_)
            {
                should_log_issues = report.metrics.issue_count > 0U;
                should_log_resolution = report.metrics.issue_count == 0U && previous_issue_count > 0U;
                last_signature_ = signature;
            }
            else if (report.metrics.issue_count == 0U && previous_issue_count > 0U)
            {
                should_log_resolution = true;
            }
        }

        if (should_log_issues)
        {
            spdlog::warn(
                "Runtime hierarchy validation detected {} issue(s) at t={:.3f}s",
                report.metrics.issue_count,
                simulation_time);

            for (const auto& issue : report.issues)
            {
                const auto entity_id = static_cast<std::uint32_t>(issue.entity);
                const auto related_id = static_cast<std::uint32_t>(issue.related);
                spdlog::warn(
                    "  [{}] entity={} related={} message={}",
                    scene::validation::to_string(issue.type),
                    entity_id,
                    related_id,
                    issue.message);
            }
        }
        else if (should_log_resolution)
        {
            spdlog::info(
                "Runtime hierarchy validation cleared after reporting {} issue(s)",
                previous_issue_count);
        }

        for (const auto& callback : callbacks)
        {
            if (callback)
            {
                callback(report, simulation_time);
            }
        }
    }

    std::size_t DiagnosticsBridge::compute_report_signature(
        const scene::validation::HierarchyValidationReport& report) const noexcept
    {
        std::size_t seed = 0;
        hash_combine(seed, report.metrics.issue_count);
        hash_combine(seed, report.metrics.cycle_count);
        hash_combine(seed, report.metrics.dangling_parent_count);
        hash_combine(seed, report.metrics.missing_parent_hierarchy_count);
        hash_combine(seed, report.metrics.non_finite_transform_count);
        hash_combine(seed, report.metrics.transform_mismatch_count);

        for (const auto& issue : report.issues)
        {
            hash_combine(seed, static_cast<std::uint32_t>(issue.entity));
            hash_combine(seed, static_cast<std::uint32_t>(issue.related));
            hash_combine(seed, static_cast<int>(issue.type));
            hash_combine(seed, issue.message);
        }

        return seed;
    }

    void DiagnosticsBridge::reset_for_testing()
    {
        std::lock_guard lock{mutex_};
        hierarchy_callbacks_.clear();
        next_callback_id_ = 1;
        last_signature_ = 0;
        last_issue_count_ = 0;
    }
} // namespace engine::runtime