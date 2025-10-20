#include "engine/scene/graph/scene_graph_validator.hpp"

#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/scene.hpp"

#include <algorithm>
#include <sstream>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace engine::scene::graph
{
    namespace
    {
        using entity_type = entt::entity;

        [[nodiscard]] std::string format_entity(entity_type entity)
        {
            std::ostringstream stream;
            stream << static_cast<std::underlying_type_t<entity_type>>(entity);
            return stream.str();
        }
    } // namespace

    SceneGraphValidator::SceneGraphValidator(const entt::registry& registry) noexcept
        : registry_{&registry}
    {
    }

    SceneGraphResult<void> SceneGraphValidator::validate() const
    {
        if (registry_ == nullptr)
        {
            return {};
        }

        std::unordered_set<entity_type> visited;
        auto view = registry_->view<const components::Hierarchy>();
        const auto estimate = static_cast<std::size_t>(view.size());

        std::vector<entity_type> stack;
        std::unordered_set<entity_type> stack_lookup;
        visited.reserve(estimate);
        stack.reserve(estimate > 0 ? estimate : 16U);
        stack_lookup.reserve(estimate > 0 ? estimate : 16U);
        for (auto entity : view)
        {
            if (!visited.contains(entity))
            {
                if (auto result = validate_entity(entity, visited, stack, stack_lookup); !result)
                {
                    return result;
                }
            }
        }

        return {};
    }

    SceneGraphResult<void> SceneGraphValidator::validate_entity(entity_type entity,
                                                                std::unordered_set<entity_type>& visited,
                                                                std::vector<entity_type>& stack,
                                                                std::unordered_set<entity_type>& stack_lookup) const
    {
        stack.push_back(entity);
        stack_lookup.insert(entity);
        visited.insert(entity);

        const auto* hierarchy = registry_->try_get<components::Hierarchy>(entity);
        if (hierarchy != nullptr)
        {
            const auto parent = hierarchy->parent;
            if (parent != entt::null)
            {
                if (!registry_->valid(parent))
                {
                    stack.pop_back();
                    stack_lookup.erase(entity);
                    return {};
                }

                if (!registry_->any_of<components::Hierarchy>(parent))
                {
                    stack.pop_back();
                    stack_lookup.erase(entity);
                    return {};
                }

                if (stack_lookup.contains(parent))
                {
                    std::ostringstream message;
                    message << "Cycle detected in scene hierarchy: "
                            << format_entity(parent) << " is an ancestor of " << format_entity(entity)
                            << " and appears again in the current traversal";
                    stack.pop_back();
                    stack_lookup.erase(entity);
                    return make_scene_graph_error(SceneGraphError::cycle_detected, message.str());
                }

                if (!visited.contains(parent))
                {
                    if (auto result = validate_entity(parent, visited, stack, stack_lookup); !result)
                    {
                        stack.pop_back();
                        stack_lookup.erase(entity);
                        return result;
                    }
                }
            }
        }

        stack.pop_back();
        stack_lookup.erase(entity);
        return {};
    }

    SceneGraphResult<void> validate(const entt::registry& registry)
    {
        SceneGraphValidator validator{registry};
        return validator.validate();
    }

    SceneGraphResult<void> validate(const Scene& scene)
    {
        return validate(scene.registry());
    }
} // namespace engine::scene::graph
