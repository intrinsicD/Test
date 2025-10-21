#include "engine/compute/api.hpp"
#include "engine/compute/dependency_analysis.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace engine::compute {

namespace {

[[nodiscard]] std::string make_cycle_error(const DependencyGraph& graph,
                                           std::string_view context,
                                           const CycleDetectionResult& cycle)
{
    std::ostringstream stream;
    stream << "KernelDispatcher detected a cycle";
    if (!context.empty())
    {
        stream << ' ' << context;
    }
    if (cycle.has_cycle && !cycle.cycle.empty())
    {
        stream << '\n' << "Cycle path: ";
        for (std::size_t index = 0; index < cycle.cycle.size(); ++index)
        {
            const auto identifier = cycle.cycle[index];
            stream << identifier;
            if (identifier < graph.nodes.size())
            {
                stream << " (" << graph.nodes[identifier].name << ')';
            }
            if (index + 1U < cycle.cycle.size())
            {
                stream << " -> ";
            }
        }
    }
    stream << '\n' << graph.to_dot();
    return stream.str();
}

[[nodiscard]] std::string make_dependency_error(const DependencyGraph& graph, const std::set<kernel_id>& missing)
{
    std::ostringstream stream;
    stream << "KernelDispatcher dependency index out of range";
    if (!missing.empty())
    {
        stream << " (missing kernels: ";
        bool first = true;
        for (const auto id : missing)
        {
            if (!first)
            {
                stream << ", ";
            }
            stream << id;
            first = false;
        }
        stream << ')';
    }
    stream << '\n' << graph.to_dot();
    return stream.str();
}

class KernelDispatcherBase : public Dispatcher {
public:
    [[nodiscard]] kernel_id add_kernel(
        std::string name,
        kernel_type kernel,
        std::vector<kernel_id> dependencies) override
    {
        kernels_.push_back(KernelNode{
            std::move(name),
            std::move(kernel),
            std::move(dependencies)});

        try
        {
            validate_registration();
        }
        catch (...)
        {
            kernels_.pop_back();
            throw;
        }

        return kernels_.size() - 1U;
    }

    void clear() noexcept override
    {
        kernels_.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept override
    {
        return kernels_.size();
    }

    [[nodiscard]] DependencyGraph dependency_graph() const override
    {
        return build_dependency_graph();
    }

    [[nodiscard]] ExecutionReport dispatch() override
    {
        const auto graph = dependency_graph();
        const auto count = kernels_.size();

        std::vector<std::vector<kernel_id>> adjacency(count);
        std::vector<std::size_t> indegree(count, 0U);
        std::set<kernel_id> unresolved;

        for (kernel_id node = 0; node < count; ++node)
        {
            for (const auto dep : graph.nodes[node].dependencies)
            {
                adjacency[dep].push_back(node);
                ++indegree[node];
            }
            for (const auto dep : graph.nodes[node].unresolved_dependencies)
            {
                unresolved.insert(dep);
            }
        }

        if (!unresolved.empty())
        {
            throw std::out_of_range{make_dependency_error(graph, unresolved)};
        }

        std::queue<kernel_id> ready;
        for (kernel_id node = 0; node < count; ++node)
        {
            if (indegree[node] == 0U)
            {
                ready.push(node);
            }
        }

        ExecutionReport report;
        report.execution_order.reserve(count);
        report.kernel_durations.reserve(count);
        report.dependency_graph = graph;
        report.clock_domain = clock_.domain;
        report.clock_name = clock_.name;

        while (!ready.empty())
        {
            const auto node = ready.front();
            ready.pop();

            report.execution_order.push_back(kernels_[node].name);
            report.kernel_durations.push_back(invoke_kernel(kernels_[node]));

            for (const auto successor : adjacency[node])
            {
                if (--indegree[successor] == 0U)
                {
                    ready.push(successor);
                }
            }
        }

        if (report.execution_order.size() != count)
        {
            const auto cycle = detect_cycles(graph);
            throw std::runtime_error{make_cycle_error(graph, "during dispatch", cycle)};
        }

        return report;
    }

    void set_clock(ClockConfiguration configuration) override
    {
        if (!configuration.measure)
        {
            throw std::invalid_argument{"ClockConfiguration.measure must be callable"};
        }
        clock_ = std::move(configuration);
    }

    [[nodiscard]] const ClockConfiguration& clock_configuration() const noexcept override
    {
        return clock_;
    }

protected:
    struct KernelNode {
        std::string name;
        kernel_type callback;
        std::vector<kernel_id> dependencies;
    };

private:
    [[nodiscard]] virtual double invoke_kernel(const KernelNode& kernel) = 0;

    [[nodiscard]] DependencyGraph build_dependency_graph() const
    {
        DependencyGraph graph;
        graph.nodes.reserve(kernels_.size());

        for (kernel_id node = 0; node < kernels_.size(); ++node)
        {
            DependencyGraph::Node metadata;
            metadata.name = kernels_[node].name;

            for (const auto dependency : kernels_[node].dependencies)
            {
                if (dependency < kernels_.size())
                {
                    metadata.dependencies.push_back(dependency);
                }
                else
                {
                    metadata.unresolved_dependencies.push_back(dependency);
                }
            }

            graph.nodes.push_back(std::move(metadata));
        }

        return graph;
    }

    void validate_registration() const
    {
        const auto graph = build_dependency_graph();
        const auto cycle = detect_cycles(graph);
        if (cycle.has_cycle)
        {
            throw std::runtime_error{make_cycle_error(graph, "during registration", cycle)};
        }
    }

    std::vector<KernelNode> kernels_;
    ClockConfiguration clock_{make_steady_clock_configuration()};
};

class CpuDispatcher final : public KernelDispatcherBase {
private:
    [[nodiscard]] double invoke_kernel(const KernelNode& kernel) override {
        return clock_configuration().measure(kernel.callback);
    }
};

class CudaDispatcher final : public KernelDispatcherBase {
private:
    [[nodiscard]] double invoke_kernel(const KernelNode& kernel) override {
        return clock_configuration().measure(kernel.callback);
    }
};

}  // namespace

namespace {

[[nodiscard]] constexpr bool compute_cuda_enabled() noexcept {
#if defined(ENGINE_ENABLE_CUDA) && defined(ENGINE_ENABLE_COMPUTE_CUDA)
    return (ENGINE_ENABLE_CUDA != 0) && (ENGINE_ENABLE_COMPUTE_CUDA != 0);
#else
    return false;
#endif
}

}  // namespace

std::string DependencyGraph::to_dot() const
{
    std::ostringstream stream;
    stream << "digraph KernelDispatcher {\n";
    stream << "  node [shape=box];\n";

    for (kernel_id node = 0; node < nodes.size(); ++node)
    {
        stream << "  node" << node << " [label=\"" << node << ':' << nodes[node].name << "\"];\n";
    }

    std::set<kernel_id> unresolved;
    for (const auto& metadata : nodes)
    {
        unresolved.insert(metadata.unresolved_dependencies.begin(), metadata.unresolved_dependencies.end());
    }

    for (const auto pending : unresolved)
    {
        stream << "  unresolved" << pending
               << " [label=\"pending:" << pending << "\", shape=ellipse, style=dashed];\n";
    }

    for (kernel_id node = 0; node < nodes.size(); ++node)
    {
        for (const auto dependency : nodes[node].dependencies)
        {
            stream << "  node" << dependency << " -> node" << node << ";\n";
        }
        for (const auto dependency : nodes[node].unresolved_dependencies)
        {
            stream << "  node" << node << " -> unresolved" << dependency << " [style=dashed];\n";
        }
    }

    stream << "}\n";
    return stream.str();
}

std::string_view module_name() noexcept {
    return "compute";
}

std::unique_ptr<Dispatcher> make_cpu_dispatcher() {
    return std::make_unique<CpuDispatcher>();
}

std::unique_ptr<Dispatcher> make_cuda_dispatcher() {
    return std::make_unique<CudaDispatcher>();
}

bool is_cpu_dispatcher_available() noexcept {
    return true;
}

bool is_cuda_dispatcher_available() noexcept {
    return compute_cuda_enabled();
}

DispatcherCapabilities dispatcher_capabilities() noexcept {
    return DispatcherCapabilities{
        .cpu_available = is_cpu_dispatcher_available(),
        .cuda_available = is_cuda_dispatcher_available(),
    };
}

math::mat4 identity_transform() noexcept {
    return math::identity_matrix<float, 4>();
}

ClockConfiguration make_steady_clock_configuration()
{
    ClockConfiguration configuration{};
    configuration.name = "steady_clock";
    configuration.domain = TimingDomain::Cpu;
    configuration.measure = [](const kernel_callback& callback) -> double {
        const auto start = std::chrono::steady_clock::now();
        if (callback)
        {
            callback();
        }
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - start).count();
    };
    return configuration;
}

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept {
    return engine::compute::module_name().data();
}
