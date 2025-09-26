#include "engine/animation/api.hpp"

namespace engine::animation {

std::string_view module_name() noexcept {
    return "animation";
}

}  // namespace engine::animation

extern "C" ENGINE_ANIMATION_API const char* engine_animation_module_name() noexcept {
    return engine::animation::module_name().data();
}
