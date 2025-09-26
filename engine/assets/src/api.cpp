#include "engine/assets/api.hpp"

namespace engine::assets {

std::string_view module_name() noexcept {
    return "assets";
}

}  // namespace engine::assets

extern "C" ENGINE_ASSETS_API const char* engine_assets_module_name() noexcept {
    return engine::assets::module_name().data();
}
