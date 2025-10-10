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
    ++alive_entities_;
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
    if (alive_entities_ > 0) {
        --alive_entities_;
    }
}

bool registry::is_alive(entity_id entity) const {
    return registry_.valid(entity.value());
}

std::size_t registry::alive_count() const {
    return alive_entities_;
}

void registry::clear() {
    spdlog::debug("Clearing registry ({} entities)", alive_count());
    registry_.clear();
    alive_entities_ = 0;
}

void draw_registry_debug_ui(const registry& registry, std::string_view window_name) {
    if (!ImGui::Begin(window_name.data(), nullptr)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Alive entities: %zu", registry.alive_count());
    ImGui::Separator();

    ImGui::End();
}

}  // namespace engine::core::ecs

