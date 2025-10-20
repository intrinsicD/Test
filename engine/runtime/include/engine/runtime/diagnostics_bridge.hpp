#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "engine/scene/validation.hpp"

namespace engine::runtime {

    class DiagnosticsBridge
    {
    public:
        using HierarchyReportCallback = std::function<void(
            const scene::validation::HierarchyValidationReport&, double)>;
        using CallbackId = std::uint64_t;

        static DiagnosticsBridge& instance() noexcept;

        [[nodiscard]] CallbackId register_hierarchy_callback(HierarchyReportCallback callback);
        void unregister_hierarchy_callback(CallbackId id);

        void publish_hierarchy_report(const scene::validation::HierarchyValidationReport& report,
                                      double simulation_time);

        void reset_for_testing();

    private:
        DiagnosticsBridge() = default;

        [[nodiscard]] std::size_t compute_report_signature(
            const scene::validation::HierarchyValidationReport& report) const noexcept;

        std::mutex mutex_{};
        std::vector<std::pair<CallbackId, HierarchyReportCallback>> hierarchy_callbacks_{};
        CallbackId next_callback_id_{1};
        std::size_t last_signature_{0};
        std::size_t last_issue_count_{0};
    };

}  // namespace engine::runtime

