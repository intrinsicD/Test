#pragma once

#include <cstddef>
#include <functional>
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

struct ExecutionReport {
    std::vector<std::string> execution_order;
    std::vector<double> kernel_durations;
};

class ENGINE_COMPUTE_API KernelDispatcher {
public:
    using kernel_type = std::function<void()>;

    [[nodiscard]] std::size_t add_kernel(
        std::string name,
        kernel_type kernel,
        std::vector<std::size_t> dependencies = {});

    void clear() noexcept;

    [[nodiscard]] ExecutionReport dispatch();

    [[nodiscard]] std::size_t size() const noexcept;

private:
    struct KernelNode {
        std::string name;
        kernel_type callback;
        std::vector<std::size_t> dependencies;
    };

    std::vector<KernelNode> kernels_;
};

[[nodiscard]] ENGINE_COMPUTE_API std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_COMPUTE_API math::mat4 identity_transform() noexcept;

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept;
