#include "engine/core/ecs/system.hpp"

#include <spdlog/spdlog.h>

namespace engine::core::ecs {

lambda_system::lambda_system(std::string name, callback_type callback)
    : name_{std::move(name)}, callback_{std::move(callback)} {}

std::string_view lambda_system::name() const noexcept {
    return name_;
}

void lambda_system::update(registry& registry, double dt) {
    if (callback_) {
        callback_(registry, dt);
    }
}

void system_scheduler::add_system(system_ptr system) {
    systems_.push_back(std::move(system));
}

void system_scheduler::tick(registry& registry, double dt) {
    for (auto& system : systems_) {
        spdlog::trace("Executing system '{}'", system->name());
        system->update(registry, dt);
    }
}

}  // namespace engine::core::ecs

