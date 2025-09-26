#include "engine/geometry/api.hpp"

namespace engine::geometry {

std::string_view module_name() noexcept {
    return "geometry";
}

}  // namespace engine::geometry

extern "C" ENGINE_GEOMETRY_API const char* engine_geometry_module_name() noexcept {
    return engine::geometry::module_name().data();
}
