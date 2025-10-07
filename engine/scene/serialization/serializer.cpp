#include "serializer.hpp"

#include "components/hierarchy.hpp"
#include "components/name.hpp"
#include "components/transform.hpp"
#include "engine/scene/scene.hpp"

#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::scene::serialization
{
    namespace
    {
        using entity_type = Scene::entity_type;
        using underlying_type = std::underlying_type_t<entity_type>;

        [[nodiscard]] std::vector<entity_type> collect_entities(const Scene& scene)
        {
            std::vector<entity_type> entities;
            entities.reserve(scene.registry().alive_count());

            auto& registry = const_cast<Scene::registry_type&>(scene.registry());
            auto view = registry.view<>();
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                auto [entity] = *it;
                entities.push_back(entity);
            }

            std::sort(entities.begin(), entities.end(), [](entity_type lhs, entity_type rhs) {
                return static_cast<underlying_type>(lhs) < static_cast<underlying_type>(rhs);
            });

            return entities;
        }
    } // namespace

    void save(const Scene& scene, std::ostream& output)
    {
        using components::serialization::encode;

        const auto name = std::string{scene.name()};
        output << "scene " << std::quoted(name) << ' ' << scene.registry().alive_count() << '\n';

        const auto entities = collect_entities(scene);
        const auto& registry = scene.registry();

        for (const auto entity : entities)
        {
            std::size_t component_count = 0;
            if (registry.any_of<components::Name>(entity)) ++component_count;
            if (registry.any_of<components::Hierarchy>(entity)) ++component_count;
            if (registry.any_of<components::LocalTransform>(entity)) ++component_count;
            if (registry.any_of<components::WorldTransform>(entity)) ++component_count;
            if (registry.any_of<components::DirtyTransform>(entity)) ++component_count;

            output << "entity " << static_cast<underlying_type>(entity) << ' ' << component_count << '\n';

            if (registry.any_of<components::Name>(entity))
            {
                output << "component Name ";
                encode(output, registry.get<components::Name>(entity));
                output << '\n';
            }

            if (registry.any_of<components::Hierarchy>(entity))
            {
                output << "component Hierarchy ";
                encode(output, registry.get<components::Hierarchy>(entity));
                output << '\n';
            }

            if (registry.any_of<components::LocalTransform>(entity))
            {
                output << "component LocalTransform ";
                encode(output, registry.get<components::LocalTransform>(entity));
                output << '\n';
            }

            if (registry.any_of<components::WorldTransform>(entity))
            {
                output << "component WorldTransform ";
                encode(output, registry.get<components::WorldTransform>(entity));
                output << '\n';
            }

            if (registry.any_of<components::DirtyTransform>(entity))
            {
                output << "component DirtyTransform ";
                encode(output, registry.get<components::DirtyTransform>(entity));
                output << '\n';
            }

            output << "entity_end\n";
        }

        output << "scene_end\n";
    }

    void load(Scene& scene, std::istream& input)
    {
        using components::serialization::decode_dirty;
        using components::serialization::decode_hierarchy;
        using components::serialization::decode_local;
        using components::serialization::decode_name;
        using components::serialization::decode_world;

        std::string token;
        if (!(input >> token) || token != "scene")
        {
            throw std::runtime_error{"Scene serialization: expected 'scene' token"};
        }

        std::string name;
        if (!(input >> std::quoted(name)))
        {
            throw std::runtime_error{"Scene serialization: failed to read scene name"};
        }

        std::size_t entity_count = 0;
        if (!(input >> entity_count))
        {
            throw std::runtime_error{"Scene serialization: failed to read entity count"};
        }

        scene.registry().clear();
        scene.set_name(std::move(name));

        struct PendingHierarchy
        {
            entity_type entity;
            components::serialization::HierarchyRecord record;
        };

        std::vector<PendingHierarchy> pending;
        pending.reserve(entity_count);
        std::unordered_map<underlying_type, entity_type> id_map;
        id_map.reserve(entity_count);

        const auto expect_token = [&](std::string expected) {
            std::string actual;
            if (!(input >> actual) || actual != expected)
            {
                throw std::runtime_error{"Scene serialization: expected token '" + expected + "'"};
            }
        };

        for (std::size_t i = 0; i < entity_count; ++i)
        {
            expect_token("entity");

            underlying_type serialized_id{};
            std::size_t component_count = 0;
            if (!(input >> serialized_id >> component_count))
            {
                throw std::runtime_error{"Scene serialization: failed to read entity header"};
            }

            const auto entity = scene.registry().create();
            id_map.emplace(serialized_id, entity);

            for (std::size_t component_index = 0; component_index < component_count; ++component_index)
            {
                expect_token("component");

                std::string component_type;
                if (!(input >> component_type))
                {
                    throw std::runtime_error{"Scene serialization: failed to read component type"};
                }

                if (component_type == "Name")
                {
                    auto component = decode_name(input);
                    scene.registry().emplace_or_replace<components::Name>(entity, std::move(component));
                }
                else if (component_type == "Hierarchy")
                {
                    auto record = decode_hierarchy(input);
                    pending.push_back(PendingHierarchy{entity, record});
                }
                else if (component_type == "LocalTransform")
                {
                    auto component = decode_local(input);
                    scene.registry().emplace_or_replace<components::LocalTransform>(entity, component);
                }
                else if (component_type == "WorldTransform")
                {
                    auto component = decode_world(input);
                    scene.registry().emplace_or_replace<components::WorldTransform>(entity, component);
                }
                else if (component_type == "DirtyTransform")
                {
                    static_cast<void>(decode_dirty(input));
                    scene.registry().emplace_or_replace<components::DirtyTransform>(entity);
                }
                else
                {
                    throw std::runtime_error{"Scene serialization: unknown component type '" + component_type + "'"};
                }
            }

            expect_token("entity_end");
        }

        expect_token("scene_end");

        for (const auto& pending_entry : pending)
        {
            const auto resolver = [&](components::serialization::HierarchyRecord::entity_type value) {
                if (value == components::serialization::HierarchyRecord::null_value())
                {
                    return entt::null;
                }

                const auto it = id_map.find(value);
                if (it == id_map.end())
                {
                    throw std::runtime_error{"Scene serialization: unresolved hierarchy reference"};
                }

                return it->second;
            };

            const auto hierarchy = components::serialization::instantiate(pending_entry.record, resolver);
            scene.registry().emplace_or_replace<components::Hierarchy>(pending_entry.entity, hierarchy);
        }
    }
} // namespace engine::scene::serialization
