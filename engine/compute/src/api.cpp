#include "engine/compute/api.hpp"

#include <queue>
#include <stdexcept>

namespace engine::compute {

std::string_view module_name() noexcept {
    return "compute";
}

std::size_t KernelDispatcher::add_kernel(
    std::string name,
    kernel_type kernel,
    std::vector<std::size_t> dependencies) {
    kernels_.push_back(KernelNode{std::move(name), std::move(kernel), std::move(dependencies)});
    return kernels_.size() - 1U;
}

void KernelDispatcher::clear() noexcept {
    kernels_.clear();
}

ExecutionReport KernelDispatcher::dispatch() {
    const std::size_t count = kernels_.size();
    std::vector<std::vector<std::size_t>> adjacency(count);
    std::vector<std::size_t> indegree(count, 0U);

    for (std::size_t node = 0; node < count; ++node) {
        for (const auto dep : kernels_[node].dependencies) {
            if (dep >= count) {
                throw std::out_of_range{"KernelDispatcher dependency index out of range"};
            }
            adjacency[dep].push_back(node);
            ++indegree[node];
        }
    }

    std::queue<std::size_t> ready;
    for (std::size_t node = 0; node < count; ++node) {
        if (indegree[node] == 0U) {
            ready.push(node);
        }
    }

    ExecutionReport report;
    report.execution_order.reserve(count);

    while (!ready.empty()) {
        const auto node = ready.front();
        ready.pop();

        if (kernels_[node].callback) {
            kernels_[node].callback();
        }
        report.execution_order.push_back(kernels_[node].name);

        for (const auto successor : adjacency[node]) {
            if (--indegree[successor] == 0U) {
                ready.push(successor);
            }
        }
    }

    if (report.execution_order.size() != count) {
        throw std::runtime_error{"KernelDispatcher detected a cycle"};
    }

    return report;
}

std::size_t KernelDispatcher::size() const noexcept {
    return kernels_.size();
}

math::mat4 identity_transform() noexcept {
    return math::identity_matrix<float, 4>();
}

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept {
    return engine::compute::module_name().data();
}
