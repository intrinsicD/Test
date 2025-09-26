#include "engine/compute/api.hpp"

namespace engine::compute {

std::string_view module_name() noexcept {
    return "compute";
}

math::mat4 identity_transform() noexcept {
    return math::identity_matrix<float, 4>();
}

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept {
    return engine::compute::module_name().data();
}
