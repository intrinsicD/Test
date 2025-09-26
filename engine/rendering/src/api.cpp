#include "engine/rendering/api.hpp"

namespace engine::rendering {

std::string_view module_name() noexcept {
    return "rendering";
}

}  // namespace engine::rendering

extern "C" ENGINE_RENDERING_API const char* engine_rendering_module_name() noexcept {
    return engine::rendering::module_name().data();
}
