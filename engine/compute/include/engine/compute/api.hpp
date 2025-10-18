#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/math/math.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_COMPUTE_EXPORTS)
#    define ENGINE_COMPUTE_API __declspec(dllexport)
#  else
#    define ENGINE_COMPUTE_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_COMPUTE_API
#endif

#define MAYBE_UNUSED_CONST_AUTO [[maybe_unused]] const auto

namespace engine::compute {

using kernel_id = std::size_t;

struct DependencyGraph {
    struct Node {
        std::string name;
        std::vector<kernel_id> dependencies;
        std::vector<kernel_id> unresolved_dependencies;
    };

    std::vector<Node> nodes;

    [[nodiscard]] ENGINE_COMPUTE_API std::string to_dot() const;
};

struct ExecutionReport {
    std::vector<std::string> execution_order;
    std::vector<double> kernel_durations;
    DependencyGraph dependency_graph;
};

struct DispatcherCapabilities {
    bool cpu_available{false};
    bool cuda_available{false};
};

class ENGINE_COMPUTE_API Dispatcher {
public:
    using kernel_id = engine::compute::kernel_id;
    using kernel_type = std::function<void()>;

    virtual ~Dispatcher() = default;

    [[nodiscard]] virtual kernel_id add_kernel(
        std::string name,
        kernel_type kernel,
        std::vector<kernel_id> dependencies = {}) = 0;

    virtual void clear() noexcept = 0;

    [[nodiscard]] virtual ExecutionReport dispatch() = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    [[nodiscard]] virtual DependencyGraph dependency_graph() const = 0;
};

[[nodiscard]] ENGINE_COMPUTE_API std::unique_ptr<Dispatcher> make_cpu_dispatcher();

[[nodiscard]] ENGINE_COMPUTE_API std::unique_ptr<Dispatcher> make_cuda_dispatcher();

[[nodiscard]] ENGINE_COMPUTE_API bool is_cpu_dispatcher_available() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API bool is_cuda_dispatcher_available() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API DispatcherCapabilities dispatcher_capabilities() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API math::mat4 identity_transform() noexcept;

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept;
