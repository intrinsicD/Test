#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "engine/scene/components/hierarchy.hpp"

#include <entt/entt.hpp>

namespace entt
{
    inline std::ostream& operator<<(std::ostream& os, const entity e)
    {
        using underlying = std::underlying_type_t<entity>;
        return os << static_cast<underlying>(e);
    }

    inline std::ostream& operator<<(std::ostream& os, const null_t &)
    {
        return os << "null";
    }
}

namespace engine::scene
{
    class Scene;

    class Entity
    {
    public:
        using id_type = entt::entity;

        Entity() = default;
        Entity(id_type id, Scene* scene) noexcept;

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] id_type id() const noexcept;

        [[nodiscard]] Scene& scene() const;

        void reset() noexcept;
        void destroy();

        void set_parent(Entity parent);
        void detach_from_parent();
        [[nodiscard]] Entity parent() const;

        template <typename Component, typename... Args>
        Component& emplace(Args&&... args);

        template <typename Component, typename... Args>
        Component& emplace_or_replace(Args&&... args);

        template <typename Component>
        [[nodiscard]] Component& get();

        template <typename Component>
        [[nodiscard]] const Component& get() const;

        template <typename Component>
        [[nodiscard]] bool has() const noexcept;

        template <typename Component>
        void remove() const;

    private:
        friend class Scene;


        id_type id_{entt::null};
        Scene* scene_{nullptr};
    };

    class Scene
    {
    public:
        using registry_type = entt::registry;
        using entity_type = entt::entity;

        Scene();
        explicit Scene(std::string name);

        [[nodiscard]] std::string_view name() const noexcept;
        void set_name(std::string name);

        [[nodiscard]] Entity create_entity();
        [[nodiscard]] Entity create_entity(std::string name);
        void destroy_entity(Entity& entity);
        void destroy_entity(entity_type entity);

        [[nodiscard]] Entity wrap(entity_type entity) noexcept;
        [[nodiscard]] bool valid(entity_type entity) const noexcept;

        void set_parent(Entity& child, Entity parent);
        void detach_from_parent(Entity& child);

        void update();

        registry_type& registry() noexcept;
        const registry_type& registry() const noexcept;

        template <typename... Components>
        [[nodiscard]] auto view();

        template <typename... Components>
        [[nodiscard]] auto view() const;

        void save(std::ostream& output) const;
        void load(std::istream& input);

        std::size_t size() const noexcept;

    private:
        registry_type registry_{};
        std::string name_{};

        void initialize_systems();
    };

    inline Entity::Entity(id_type id, Scene* scene) noexcept : id_{id}, scene_{scene}
    {
    }

    inline bool Entity::valid() const noexcept
    {
        return scene_ != nullptr && scene_->valid(id_);
    }

    inline Entity::id_type Entity::id() const noexcept
    {
        return id_;
    }

    inline Scene& Entity::scene() const
    {
        if (scene_ == nullptr)
        {
            throw std::logic_error{"Entity is not associated with a scene"};
        }

        return *scene_;
    }

    inline void Entity::reset() noexcept
    {
        id_ = entt::null;
        scene_ = nullptr;
    }

    inline void Entity::destroy()
    {
        if (scene_ != nullptr)
        {
            scene_->destroy_entity(*this);
        }
    }

    inline void Entity::set_parent(Entity parent)
    {
        scene().set_parent(*this, parent);
    }

    inline void Entity::detach_from_parent()
    {
        scene().detach_from_parent(*this);
    }

    inline Entity Entity::parent() const
    {
        if (!valid())
        {
            return {};
        }

        if (auto* hierarchy = scene_->registry().try_get<components::Hierarchy>(id_); hierarchy != nullptr)
        {
            return scene_->wrap(hierarchy->parent);
        }

        return {};
    }

    template <typename Component, typename... Args>
    inline Component& Entity::emplace(Args&&... args)
    {
        return scene_->registry().template emplace<Component>(id_, std::forward<Args>(args)...);
    }

    template <typename Component, typename... Args>
    inline Component& Entity::emplace_or_replace(Args&&... args)
    {
        return scene_->registry().template emplace_or_replace<Component>(id_, std::forward<Args>(args)...);
    }

    template <typename Component>
    inline Component& Entity::get()
    {
        return scene_->registry().template get<Component>(id_);
    }

    template <typename Component>
    inline const Component& Entity::get() const
    {
        return scene_->registry().template get<Component>(id_);
    }

    template <typename Component>
    inline bool Entity::has() const noexcept
    {
        return scene_->registry().template any_of<Component>(id_);
    }

    template <typename Component>
    inline void Entity::remove() const
    {
        scene_->registry().template remove<Component>(id_);
    }

    template <typename... Components>
    inline auto Scene::view()
    {
        return registry_.template view<Components...>();
    }

    template <typename... Components>
    inline auto Scene::view() const
    {
        return registry_.template view<Components...>();
    }
} // namespace engine::scene
