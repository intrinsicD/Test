#include "engine/compute/cuda/api.hpp"

namespace engine::compute::cuda {

std::string_view module_name() noexcept {
    return "compute.cuda";
}

math::vec3 default_device_axis() noexcept {
    return math::normalize(math::vec3{0.0F, 0.0F, 1.0F});
}

math::mat4 default_device_transform() noexcept {
    auto transform = math::identity_matrix<float, 4>();
    transform[2][3] = -1.0F;
    return transform;
}

}  // namespace engine::compute::cuda

extern "C" ENGINE_COMPUTE_CUDA_API const char* engine_compute_cuda_module_name() noexcept {
    return engine::compute::cuda::module_name().data();
}
