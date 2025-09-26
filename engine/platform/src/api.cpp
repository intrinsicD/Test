#include "engine/platform/api.hpp"

namespace engine::platform {

std::string_view module_name() noexcept {
    return "platform";
}

}  // namespace engine::platform

extern "C" ENGINE_PLATFORM_API const char* engine_platform_module_name() noexcept {
    return engine::platform::module_name().data();
}
