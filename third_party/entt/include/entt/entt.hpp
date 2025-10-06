#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace entt {

enum class entity : std::uint64_t {};
inline constexpr entity null{static_cast<entity>(0)};

[[nodiscard]] constexpr std::uint64_t entity_to_integral(entity value) noexcept {
    return static_cast<std::uint64_t>(value);
}

[[nodiscard]] constexpr entity make_entity(std::uint32_t index, std::uint32_t generation) noexcept {
    const auto raw = (static_cast<std::uint64_t>(generation) << 32u) | (static_cast<std::uint64_t>(index) + 1u);
    return static_cast<entity>(raw);
}

[[nodiscard]] constexpr std::uint32_t entity_index(entity value) noexcept {
    const auto raw = entity_to_integral(value);
    return raw == 0 ? std::numeric_limits<std::uint32_t>::max()
                    : static_cast<std::uint32_t>((raw & 0xffffffffu) - 1u);
}

[[nodiscard]] constexpr std::uint32_t entity_generation(entity value) noexcept {
    return static_cast<std::uint32_t>(entity_to_integral(value) >> 32u);
}

struct entity_hash {
    [[nodiscard]] std::size_t operator()(entity value) const noexcept {
        return static_cast<std::size_t>(entity_to_integral(value));
    }
};

class registry {
public:
    using entity_type = entity;

    registry() = default;
    registry(const registry&) = delete;
    registry(registry&&) noexcept = default;
    registry& operator=(const registry&) = delete;
    registry& operator=(registry&&) noexcept = default;
    ~registry() = default;

    [[nodiscard]] entity create() {
        std::uint32_t index = 0;
        std::uint32_t generation = 0;
        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
            auto& data = entities_[index];
            data.alive = true;
            generation = data.generation;
        } else {
            index = static_cast<std::uint32_t>(entities_.size());
            entities_.push_back({true, 0});
            generation = 0;
        }
        return make_entity(index, generation);
    }

    void destroy(entity value) {
        if (!valid(value)) {
            return;
        }
        const auto index = entity_index(value);
        auto& data = entities_[index];
        data.alive = false;
        ++data.generation;
        for (auto& [_, storage] : storages_) {
            storage->erase(value);
        }
        free_list_.push_back(index);
    }

    [[nodiscard]] bool valid(entity value) const {
        if (value == null) {
            return false;
        }
        const auto index = entity_index(value);
        if (index >= entities_.size()) {
            return false;
        }
        const auto& data = entities_[index];
        return data.alive && data.generation == entity_generation(value);
    }

    [[nodiscard]] std::size_t alive_count() const {
        std::size_t alive = 0;
        for (const auto& data : entities_) {
            if (data.alive) {
                ++alive;
            }
        }
        return alive;
    }

    void clear() {
        entities_.clear();
        free_list_.clear();
        storages_.clear();
    }

    template <typename Component, typename... Args>
    Component& emplace(entity value, Args&&... args) {
        auto& storage = assure_storage<Component>();
        return storage.emplace(value, std::forward<Args>(args)...);
    }

    template <typename Component>
    Component& get(entity value) {
        auto* storage = find_storage<Component>();
        return storage->get(value);
    }

    template <typename Component>
    const Component& get(entity value) const {
        const auto* storage = find_storage<Component>();
        return storage->get(value);
    }

    template <typename Component>
    bool any_of(entity value) const {
        const auto* storage = find_storage<Component>();
        return storage != nullptr && storage->contains(value);
    }

    template <typename Component>
    void remove(entity value) {
        if (auto* storage = find_storage<Component>(); storage != nullptr) {
            storage->erase(value);
        }
    }

    template <typename Component, typename... Args>
    Component& emplace_or_replace(entity value, Args&&... args) {
        auto& storage = assure_storage<Component>();
        return storage.emplace_or_replace(value, std::forward<Args>(args)...);
    }

    template <typename Component>
    Component* try_get(entity value) {
        auto* storage = find_storage<Component>();
        return storage != nullptr ? storage->try_get(value) : nullptr;
    }

    template <typename Component>
    const Component* try_get(entity value) const {
        const auto* storage = find_storage<Component>();
        return storage != nullptr ? storage->try_get(value) : nullptr;
    }

    template <typename Func>
    void for_each_storage(Func&& func) const {
        for (const auto& [type, storage] : storages_) {
            func(type, storage->size());
        }
    }

private:
    struct entity_data {
        bool alive{false};
        std::uint32_t generation{0};
    };

    struct storage_base {
        virtual ~storage_base() = default;
        virtual void erase(entity value) = 0;
        [[nodiscard]] virtual bool contains(entity value) const = 0;
        [[nodiscard]] virtual std::size_t size() const = 0;
        [[nodiscard]] virtual std::vector<entity> snapshot() const = 0;
    };

    template <typename Component>
    struct storage_impl final : storage_base {
        using map_type = std::unordered_map<entity, Component, entity_hash>;

        template <typename... Args>
        Component& emplace(entity value, Args&&... args) {
            auto [it, inserted] = components.try_emplace(value, std::forward<Args>(args)...);
            if (inserted) {
                insertion_order.push_back(value);
            }
            return it->second;
        }

        template <typename... Args>
        Component& emplace_or_replace(entity value, Args&&... args) {
            if (auto it = components.find(value); it != components.end()) {
                it->second = Component(std::forward<Args>(args)...);
                return it->second;
            }
            insertion_order.push_back(value);
            auto [it, inserted] = components.emplace(value, Component(std::forward<Args>(args)...));
            (void)inserted;
            return it->second;
        }

        Component& get(entity value) {
            return components.at(value);
        }

        const Component& get(entity value) const {
            return components.at(value);
        }

        Component* try_get(entity value) {
            if (auto it = components.find(value); it != components.end()) {
                return &it->second;
            }
            return nullptr;
        }

        const Component* try_get(entity value) const {
            if (auto it = components.find(value); it != components.end()) {
                return &it->second;
            }
            return nullptr;
        }

        void erase(entity value) override {
            if (auto it = components.find(value); it != components.end()) {
                components.erase(it);
                insertion_order.erase(
                    std::remove(insertion_order.begin(), insertion_order.end(), value),
                    insertion_order.end());
            }
        }

        [[nodiscard]] bool contains(entity value) const override {
            return components.find(value) != components.end();
        }

        [[nodiscard]] std::size_t size() const override {
            return insertion_order.size();
        }

        [[nodiscard]] std::vector<entity> snapshot() const override {
            return insertion_order;
        }

        map_type components;
        std::vector<entity> insertion_order;
    };

    template <typename Component>
    storage_impl<Component>& assure_storage() {
        const auto key = std::type_index(typeid(Component));
        auto it = storages_.find(key);
        if (it == storages_.end()) {
            auto storage = std::make_unique<storage_impl<Component>>();
            it = storages_.emplace(key, std::move(storage)).first;
        }
        return *static_cast<storage_impl<Component>*>(it->second.get());
    }

    template <typename Component>
    storage_impl<Component>* find_storage() {
        const auto key = std::type_index(typeid(Component));
        auto it = storages_.find(key);
        if (it == storages_.end()) {
            return nullptr;
        }
        return static_cast<storage_impl<Component>*>(it->second.get());
    }

    template <typename Component>
    const storage_impl<Component>* find_storage() const {
        const auto key = std::type_index(typeid(Component));
        auto it = storages_.find(key);
        if (it == storages_.end()) {
            return nullptr;
        }
        return static_cast<const storage_impl<Component>*>(it->second.get());
    }

    template <typename... Components>
    std::vector<entity> gather_view_entities() {
        return gather_view_entities_impl<Components...>(*this);
    }

    template <typename... Components>
    std::vector<entity> gather_view_entities() const {
        return gather_view_entities_impl<Components...>(*this);
    }

    template <typename... Components, typename Self>
    static std::vector<entity> gather_view_entities_impl(Self& self) {
        if constexpr (sizeof...(Components) == 0) {
            std::vector<entity> entities;
            entities.reserve(self.entities_.size());
            for (std::uint32_t index = 0; index < self.entities_.size(); ++index) {
                const auto& data = self.entities_[index];
                if (data.alive) {
                    entities.push_back(make_entity(index, data.generation));
                }
            }
            return entities;
        } else {
            auto storages = std::tuple{self.template find_storage<Components>()...};
            if (((std::get<storage_impl<Components>*>(storages) == nullptr) || ...)) {
                return {};
            }
            auto candidates = std::get<0>(storages)->snapshot();
            filter_candidates<Components...>(candidates, storages, std::make_index_sequence<sizeof...(Components)>{});
            return candidates;
        }
    }

    template <typename... Components, typename Tuple, std::size_t... Indices>
    static void filter_candidates(std::vector<entity>& candidates, const Tuple& storages, std::index_sequence<Indices...>) {
        (filter_candidate_at<Indices, Components...>(candidates, storages), ...);
    }

    template <std::size_t Index, typename... Components, typename Tuple>
    static void filter_candidate_at(std::vector<entity>& candidates, const Tuple& storages) {
        if constexpr (Index > 0) {
            const auto* storage = std::get<Index>(storages);
            candidates.erase(
                std::remove_if(
                    candidates.begin(),
                    candidates.end(),
                    [storage](entity value) { return !storage->contains(value); }),
                candidates.end());
        }
    }

    template <typename... Components>
    class view_type {
    public:
        class iterator {
        public:
            using underlying_iterator = typename std::vector<entity>::const_iterator;

            iterator(registry* owner, underlying_iterator current)
                : owner_{owner}, current_{current} {}

            iterator& operator++() {
                ++current_;
                return *this;
            }

            [[nodiscard]] bool operator==(const iterator& other) const {
                return current_ == other.current_;
            }

            [[nodiscard]] bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

            [[nodiscard]] auto operator*() const {
                const auto entity_value = *current_;
                return std::tuple<entity, Components&...>{
                    entity_value,
                    owner_->template get<Components>(entity_value)...};
            }

        private:
            registry* owner_;
            underlying_iterator current_;
        };

        explicit view_type(registry& owner)
            : owner_{&owner}, entities_{owner.template gather_view_entities<Components...>()} {}

        [[nodiscard]] iterator begin() {
            return iterator{owner_, entities_.begin()};
        }

        [[nodiscard]] iterator end() {
            return iterator{owner_, entities_.end()};
        }

        [[nodiscard]] std::size_t size() const {
            return entities_.size();
        }

        [[nodiscard]] view_type each() {
            return *this;
        }

        [[nodiscard]] view_type each() const {
            return *this;
        }

    private:
        registry* owner_;
        std::vector<entity> entities_;
    };

public:
    template <typename... Components>
    view_type<Components...> view() {
        return view_type<Components...>{*this};
    }

private:
    std::vector<entity_data> entities_;
    std::vector<std::uint32_t> free_list_;
    std::unordered_map<std::type_index, std::unique_ptr<storage_base>> storages_;
};

}  // namespace entt

namespace std {

template <>
struct hash<entt::entity> {
    [[nodiscard]] std::size_t operator()(entt::entity value) const noexcept {
        return static_cast<std::size_t>(entt::entity_to_integral(value));
    }
};

}  // namespace std

