#include "engine/core/api.hpp"

namespace engine::core {

std::string_view module_name() noexcept {
    return "core";
}

}  // namespace engine::core

extern "C" ENGINE_CORE_API const char* engine_core_module_name() noexcept {
    return engine::core::module_name().data();
}
