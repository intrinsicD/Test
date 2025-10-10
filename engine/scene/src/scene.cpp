#include "engine/scene/scene.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/systems.hpp"
#include "engine/scene/serialization/serializer.hpp"

#include <utility>

namespace engine::scene
{
    Scene::Scene()
    {
        initialize_systems();
    }

    Scene::Scene(std::string name) : name_{std::move(name)}
    {
        initialize_systems();
    }

    std::string_view Scene::name() const noexcept
    {
        return name_;
    }

    void Scene::set_name(std::string name)
    {
        name_ = std::move(name);
    }

    Entity Scene::create_entity()
    {
        const auto entity = registry_.create();
        return Entity{entity, this};
    }

    Entity Scene::create_entity(std::string name)
    {
        auto entity = create_entity();
        entity.emplace<components::Name>(components::Name{std::move(name)});
        return entity;
    }

    void Scene::destroy_entity(Entity& entity)
    {
        if (entity.scene_ != this)
        {
            return;
        }

        destroy_entity(entity.id_);
        entity.reset();
    }

    void Scene::destroy_entity(entity_type entity)
    {
        if (registry_.valid(entity))
        {
            if (auto* hierarchy = registry_.try_get<components::Hierarchy>(entity); hierarchy != nullptr)
            {
                auto child = hierarchy->first_child;
                while (child != entt::null)
                {
                    if (auto* child_hierarchy = registry_.try_get<components::Hierarchy>(child); child_hierarchy !=
                        nullptr)
                    {
                        const auto next = child_hierarchy->next_sibling;
                        child_hierarchy->parent = entt::null;
                        child_hierarchy->previous_sibling = entt::null;
                        child_hierarchy->next_sibling = entt::null;
                        systems::mark_subtree_dirty(registry_, child);
                        child = next;
                    }
                    else
                    {
                        child = entt::null;
                    }
                }
            }

            systems::detach_from_parent(registry_, entity);
            registry_.destroy(entity);
        }
    }

    Entity Scene::wrap(entity_type entity) noexcept
    {
        if (!registry_.valid(entity))
        {
            return {};
        }

        return Entity{entity, this};
    }

    bool Scene::valid(entity_type entity) const noexcept
    {
        return registry_.valid(entity);
    }

    void Scene::set_parent(Entity& child, Entity parent)
    {
        // Validate that the child belongs to this scene before mutating the hierarchy.
        if (child.scene_ != this)
        {
            throw std::logic_error{"Child entity does not belong to this scene"};
        }

        if (!child.valid())
        {
            throw std::logic_error{"Child entity is not valid"};
        }

        entt::entity parent_id = entt::null;
        if (parent.scene_ != nullptr)
        {
            if (parent.scene_ != this)
            {
                throw std::logic_error{"Parent entity does not belong to this scene"};
            }

            if (!parent.valid())
            {
                throw std::logic_error{"Parent entity is not valid"};
            }

            parent_id = parent.id_;
        }

        systems::set_parent(registry_, child.id_, parent_id);
    }

    void Scene::detach_from_parent(Entity& child)
    {
        if (child.scene_ != this)
        {
            throw std::logic_error{"Child entity does not belong to this scene"};
        }

        if (!child.valid())
        {
            return;
        }

        systems::detach_from_parent(registry_, child.id_);
    }

    void Scene::update()
    {
        systems::propagate_transforms(registry_);
    }

    Scene::registry_type& Scene::registry() noexcept
    {
        return registry_;
    }

    const Scene::registry_type& Scene::registry() const noexcept
    {
        return registry_;
    }

    void Scene::initialize_systems()
    {
        systems::register_scene_systems(registry_);
    }

    void Scene::save(std::ostream& output) const
    {
        serialization::save(*this, output);
    }

    void Scene::load(std::istream& input)
    {
        serialization::load(*this, input);
    }

    std::size_t Scene::size() const noexcept
    {
        std::size_t count = 0;
        auto view = registry_.view<entt::entity>();
        view.each([&](entt::entity) { ++count; });
        return count;
    }
} // namespace engine::scene
