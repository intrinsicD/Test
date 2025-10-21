#pragma once

#include <vector>

#include "engine/compute/api.hpp"

namespace engine::compute {

struct CycleDetectionResult {
    bool has_cycle{false};
    std::vector<kernel_id> cycle;
};

[[nodiscard]] ENGINE_COMPUTE_API CycleDetectionResult detect_cycles(const DependencyGraph& graph) noexcept;

}  // namespace engine::compute
