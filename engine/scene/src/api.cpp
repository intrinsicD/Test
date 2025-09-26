#include "engine/scene/api.hpp"

namespace engine::scene {

std::string_view module_name() noexcept {
    return "scene";
}

}  // namespace engine::scene

extern "C" ENGINE_SCENE_API const char* engine_scene_module_name() noexcept {
    return engine::scene::module_name().data();
}
