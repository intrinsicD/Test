#include "engine/compute/api.hpp"

namespace engine::compute {

std::string_view module_name() noexcept {
    return "compute";
}

}  // namespace engine::compute

extern "C" ENGINE_COMPUTE_API const char* engine_compute_module_name() noexcept {
    return engine::compute::module_name().data();
}
