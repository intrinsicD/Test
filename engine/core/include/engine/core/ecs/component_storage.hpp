#pragma once

#include <utility>

#include "engine/core/ecs/entity_id.hpp"
#include "engine/core/ecs/registry.hpp"

namespace engine::core::ecs {

template <typename Component>
class component_storage {
public:
    using component_type = Component;

    explicit component_storage(registry& owner) noexcept : registry_{&owner} {}
    explicit component_storage(registry* owner) noexcept : registry_{owner} {}

    [[nodiscard]] bool contains(entity_id entity) const {
        return registry_->template contains<Component>(entity);
    }

    template <typename... Args>
    Component& emplace(entity_id entity, Args&&... args) {
        return registry_->template emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename... Args>
    Component& emplace_or_replace(entity_id entity, Args&&... args) {
        return registry_->template emplace_or_replace<Component>(entity, std::forward<Args>(args)...);
    }

    Component& get(entity_id entity) {
        return registry_->template get<Component>(entity);
    }

    const Component& get(entity_id entity) const {
        return registry_->template get<Component>(entity);
    }

    void remove(entity_id entity) {
        registry_->template remove<Component>(entity);
    }

    Component* try_get(entity_id entity) {
        return registry_->template try_get<Component>(entity);
    }

    const Component* try_get(entity_id entity) const {
        return registry_->template try_get<Component>(entity);
    }

private:
    registry* registry_;
};

}  // namespace engine::core::ecs

