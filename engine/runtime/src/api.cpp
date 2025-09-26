#include "engine/runtime/api.hpp"

#include <array>

#include "engine/animation/api.hpp"
#include "engine/assets/api.hpp"
#include "engine/compute/api.hpp"
#include "engine/compute/cuda/api.hpp"
#include "engine/core/api.hpp"
#include "engine/geometry/api.hpp"
#include "engine/io/api.hpp"
#include "engine/physics/api.hpp"
#include "engine/platform/api.hpp"
#include "engine/rendering/api.hpp"
#include "engine/scene/api.hpp"

namespace {

const std::array<std::string_view, 11>& module_names() noexcept {
    static const std::array<std::string_view, 11> names = {
        engine::animation::module_name(),
        engine::assets::module_name(),
        engine::compute::module_name(),
        engine::compute::cuda::module_name(),
        engine::core::module_name(),
        engine::geometry::module_name(),
        engine::io::module_name(),
        engine::physics::module_name(),
        engine::platform::module_name(),
        engine::rendering::module_name(),
        engine::scene::module_name(),
    };
    return names;
}

}  // namespace

namespace engine::runtime {

std::string_view module_name() noexcept {
    return "runtime";
}

std::size_t module_count() noexcept {
    return module_names().size();
}

std::string_view module_name_at(std::size_t index) noexcept {
    const auto& names = module_names();
    if (index >= names.size()) {
        return {};
    }
    return names[index];
}

}  // namespace engine::runtime

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept {
    return engine::runtime::module_name().data();
}

extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept {
    return engine::runtime::module_count();
}

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept {
    const auto name = engine::runtime::module_name_at(index);
    return name.empty() ? nullptr : name.data();
}

