#include "engine/compute/api.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <stdexcept>

namespace engine::compute {

namespace {

class KernelDispatcherBase : public Dispatcher {
public:
    [[nodiscard]] kernel_id add_kernel(
        std::string name,
        kernel_type kernel,
        std::vector<kernel_id> dependencies) override {
        kernels_.push_back(KernelNode{
            std::move(name),
            std::move(kernel),
            std::move(dependencies)});
        return kernels_.size() - 1U;
    }

    void clear() noexcept override {
        kernels_.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept override {
        return kernels_.size();
    }

    [[nodiscard]] ExecutionReport dispatch() override {
        const auto count = kernels_.size();
        std::vector<std::vector<kernel_id>> adjacency(count);
        std::vector<std::size_t> indegree(count, 0U);

        for (kernel_id node = 0; node < count; ++node) {
            for (const auto dep : kernels_[node].dependencies) {
                if (dep >= count) {
                    throw std::out_of_range{
                        "KernelDispatcher dependency index out of range"};
                }
                adjacency[dep].push_back(node);
                ++indegree[node];
            }
        }

        std::queue<kernel_id> ready;
        for (kernel_id node = 0; node < count; ++node) {
            if (indegree[node] == 0U) {
                ready.push(node);
            }
        }

        ExecutionReport report;
        report.execution_order.reserve(count);
        report.kernel_durations.reserve(count);

        while (!ready.empty()) {
            const auto node = ready.front();
            ready.pop();

            report.execution_order.push_back(kernels_[node].name);
            report.kernel_durations.push_back(invoke_kernel(kernels_[node]));

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

protected:
    struct KernelNode {
        std::string name;
        kernel_type callback;
        std::vector<kernel_id> dependencies;
    };

private:
    [[nodiscard]] virtual double invoke_kernel(const KernelNode& kernel) = 0;

    std::vector<KernelNode> kernels_;
};

class CpuDispatcher final : public KernelDispatcherBase {
private:
    [[nodiscard]] double invoke_kernel(const KernelNode& kernel) override {
        const auto start = std::chrono::steady_clock::now();
        if (kernel.callback) {
            kernel.callback();
        }
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }
};

class CudaDispatcher final : public KernelDispatcherBase {
private:
    [[nodiscard]] double invoke_kernel(const KernelNode& kernel) override {
        const auto launch_start = std::chrono::steady_clock::now();
        if (kernel.callback) {
            kernel.callback();
        }
        const auto launch_end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(launch_end - launch_start).count();
    }
};

}  // namespace

std::string_view module_name() noexcept {
    return "compute";
}

std::unique_ptr<Dispatcher> make_cpu_dispatcher() {
    return std::make_unique<CpuDispatcher>();
}

std::unique_ptr<Dispatcher> make_cuda_dispatcher() {
    return std::make_unique<CudaDispatcher>();
}

math::mat4 identity_transform() noexcept {
    return math::identity_matrix<float, 4>();
}

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept {
    return engine::compute::module_name().data();
}
