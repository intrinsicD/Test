#include "engine/core/ecs/registry.hpp"

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace engine::core::ecs {

registry::registry() = default;
registry::~registry() = default;

entity_id registry::create() {
    const auto entity = registry_.create();
    const auto id = entity_id{entity};
    spdlog::debug("Created entity [{}:{}]", id.index(), id.generation());
    return id;
}

void registry::destroy(entity_id entity) {
    if (!entity) {
        return;
    }

    if (!registry_.valid(entity.value())) {
        spdlog::warn("Attempted to destroy invalid entity [{}:{}]", entity.index(), entity.generation());
        return;
    }

    spdlog::debug("Destroying entity [{}:{}]", entity.index(), entity.generation());
    registry_.destroy(entity.value());
}

bool registry::is_alive(entity_id entity) const {
    return registry_.valid(entity.value());
}

std::size_t registry::alive_count() const {
    return registry_.alive_count();
}

void registry::clear() {
    spdlog::debug("Clearing registry ({} entities)", alive_count());
    registry_.clear();
}

void draw_registry_debug_ui(const registry& registry, std::string_view window_name) {
    if (!ImGui::Begin(window_name.data(), nullptr)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Alive entities: %zu", registry.alive_count());
    ImGui::Separator();

    registry.visit_components([&](const std::type_index& type, std::size_t size) {
        ImGui::Text("%s : %zu", type.name(), size);
    });

    ImGui::End();
}

}  // namespace engine::core::ecs

