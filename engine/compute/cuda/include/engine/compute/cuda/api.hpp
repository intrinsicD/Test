#pragma once

#include <string_view>

#include "engine/math/math.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_COMPUTE_CUDA_EXPORTS)
#    define ENGINE_COMPUTE_CUDA_API __declspec(dllexport)
#  else
#    define ENGINE_COMPUTE_CUDA_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_COMPUTE_CUDA_API
#endif

namespace engine::compute::cuda {

[[nodiscard]] std::string_view module_name() noexcept;

[[nodiscard]] ENGINE_COMPUTE_CUDA_API math::vec3 default_device_axis() noexcept;

[[nodiscard]] ENGINE_COMPUTE_CUDA_API math::mat4 default_device_transform() noexcept;

}  // namespace engine::compute::cuda

extern "C" ENGINE_COMPUTE_CUDA_API const char* engine_compute_cuda_module_name() noexcept;
