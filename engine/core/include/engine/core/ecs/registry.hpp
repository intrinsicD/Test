#pragma once

#include <cstddef>
#include <iterator>
#include <string_view>
#include <tuple>
#include <typeindex>
#include <utility>

#include <entt/entt.hpp>

#include "engine/core/api.hpp"
#include "engine/core/ecs/entity_id.hpp"

namespace engine::core::ecs
{
    namespace detail
    {
        template <typename Tuple>
        inline auto convert_view_tuple(Tuple&& tuple)
        {
            return std::apply(
                [](auto&& entity_value, auto&&... components)
                {
                    return std::tuple<entity_id, decltype(components)...>{
                        entity_id{entity_value}, std::forward<decltype(components)>(components)...
                    };
                },
                std::forward<Tuple>(tuple));
        }

        template <typename View>
        class registry_view
        {
        public:
            using view_type = View;

            explicit registry_view(View view) : view_{std::move(view)}
            {
            }

            class iterator
            {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type = std::ptrdiff_t;

                explicit iterator(typename View::iterator iterator) : iterator_{iterator}
                {
                }

                iterator& operator++()
                {
                    ++iterator_;
                    return *this;
                }

                [[nodiscard]] bool operator==(const iterator& other) const
                {
                    return iterator_ == other.iterator_;
                }

                [[nodiscard]] bool operator!=(const iterator& other) const
                {
                    return !(*this == other);
                }

                [[nodiscard]] auto operator*() const
                {
                    return convert_view_tuple(*iterator_);
                }

            private:
                typename View::iterator iterator_;
            };

            [[nodiscard]] iterator begin()
            {
                return iterator{view_.begin()};
            }

            [[nodiscard]] iterator end()
            {
                return iterator{view_.end()};
            }

            [[nodiscard]] std::size_t size() const
            {
                return view_.size();
            }

        private:
            View view_;
        };
    } // namespace detail

    class registry
    {
    public:
        using entity_type = entity_id;

        registry();
        ~registry();

        registry(const registry&) = delete;
        registry(registry&&) noexcept = default;
        registry& operator=(const registry&) = delete;
        registry& operator=(registry&&) noexcept = default;

        [[nodiscard]] entity_id create();
        void destroy(entity_id entity);

        [[nodiscard]] bool is_alive(entity_id entity) const;
        [[nodiscard]] std::size_t alive_count() const;

        void clear();

        template <typename Component, typename... Args>
        Component& emplace(entity_id entity, Args&&... args)
        {
            return registry_.template emplace<Component>(entity.value(), std::forward<Args>(args)...);
        }

        template <typename Component>
        bool contains(entity_id entity) const
        {
            return registry_.template any_of<Component>(entity.value());
        }

        template <typename Component>
        Component& get(entity_id entity)
        {
            return registry_.template get<Component>(entity.value());
        }

        template <typename Component>
        const Component& get(entity_id entity) const
        {
            return registry_.template get<Component>(entity.value());
        }

        template <typename Component>
        void remove(entity_id entity)
        {
            registry_.template remove<Component>(entity.value());
        }

        template <typename Component, typename... Args>
        Component& emplace_or_replace(entity_id entity, Args&&... args)
        {
            return registry_.template emplace_or_replace<Component>(entity.value(), std::forward<Args>(args)...);
        }

        template <typename Component>
        Component* try_get(entity_id entity)
        {
            return registry_.template try_get<Component>(entity.value());
        }

        template <typename Component>
        const Component* try_get(entity_id entity) const
        {
            return registry_.template try_get<Component>(entity.value());
        }

        template <typename... Components>
        auto view()
        {
            static_assert(sizeof...(Components) > 0, "registry::view requires at least one component type");
            // each() guarantees a tuple: (entt::entity, Components&...)
            return detail::registry_view{registry_.template view<Components...>().each()};
        }

    private:
        entt::registry registry_;
    };

    ENGINE_CORE_API void
    draw_registry_debug_ui(const registry& registry, std::string_view window_name = "ECS Registry");
} // namespace engine::core::ecs
