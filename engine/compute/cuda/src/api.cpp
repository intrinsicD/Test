#include "engine/compute/cuda/api.hpp"

namespace engine::compute::cuda {

std::string_view module_name() noexcept {
    return "compute.cuda";
}

}  // namespace engine::compute::cuda

extern "C" ENGINE_COMPUTE_CUDA_API const char* engine_compute_cuda_module_name() noexcept {
    return engine::compute::cuda::module_name().data();
}
