#pragma once

#include <cstdint>
#include <functional>
#include <ostream>
#include <limits>

#include <entt/entity/registry.hpp>

#include "engine/core/api.hpp"

namespace engine::core::ecs {

class entity_id {
public:
    using value_type = entt::entity;
    using entity_type = std::uint32_t;

    constexpr entity_id() noexcept = default;
    explicit constexpr entity_id(value_type value ) noexcept : value_{value} {}
    explicit constexpr entity_id(entity_type value ) noexcept : value_{value} {}

    [[nodiscard]] static constexpr entity_id null() noexcept {
        return entity_id(entt::null);
    }

    [[nodiscard]] constexpr value_type value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr std::uint32_t index() const noexcept {
        return static_cast<std::uint32_t>(value_);
    }

    [[nodiscard]] constexpr std::uint32_t generation() const noexcept {
        return entt::to_version(value_);
    }

    [[nodiscard]] constexpr bool is_null() const noexcept {
        return value_ == entt::null;
    }

    explicit constexpr operator bool() const noexcept {
        return !is_null();
    }

    friend constexpr bool operator==(entity_id lhs, entity_id rhs) noexcept {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(entity_id lhs, entity_id rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    value_type value_{entt::null};
};

[[nodiscard]] inline constexpr entity_id make_entity_id(entt::registry &registry) noexcept {
    return entity_id{registry.create()};
}

}  // namespace engine::core::ecs

namespace engine::core::ecs {

inline std::ostream& operator<<(std::ostream& stream, entity_id id) {
    if (!id) {
        return stream << "[null]";
    }
    return stream << '[' << id.index() << ':' << id.generation() << ']';
}

}  // namespace engine::core::ecs

namespace std {

template <>
struct hash<engine::core::ecs::entity_id> {
    [[nodiscard]] std::size_t operator()(const engine::core::ecs::entity_id& id) const noexcept {
        return std::hash<std::uint64_t>{}(entt::to_integral(id.value()));
    }
};

}  // namespace std

