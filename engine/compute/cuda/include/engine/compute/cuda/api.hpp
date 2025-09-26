#pragma once

#include <string_view>

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

}  // namespace engine::compute::cuda

extern "C" ENGINE_COMPUTE_CUDA_API const char* engine_compute_cuda_module_name() noexcept;
