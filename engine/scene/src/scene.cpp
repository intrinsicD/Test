#include "engine/scene/scene.hpp"
#include "engine/scene/components.hpp"
#include "engine/scene/systems.hpp"
#include "serialization/serializer.hpp"

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
        return registry_.view<entt::entity>().size();
    }
} // namespace engine::scene
