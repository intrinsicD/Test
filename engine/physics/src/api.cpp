#include "engine/physics/api.hpp"

namespace engine::physics {

std::string_view module_name() noexcept {
    return "physics";
}

}  // namespace engine::physics

extern "C" ENGINE_PHYSICS_API const char* engine_physics_module_name() noexcept {
    return engine::physics::module_name().data();
}
