#include "engine/scene/validation.hpp"

#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

#include "engine/math/transform.hpp"
#include "engine/math/vector.hpp"

#include <cmath>
#include <optional>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace engine::scene::validation
{
    namespace
    {
        using entity_type = entt::entity;
        using underlying_type = std::underlying_type_t<entity_type>;

        [[nodiscard]] std::string format_entity(entity_type entity)
        {
            std::ostringstream stream;
            stream << static_cast<underlying_type>(entity);
            return stream.str();
        }

        void record_issue(HierarchyValidationReport& report,
                          entity_type entity,
                          entity_type related,
                          HierarchyIssueType type,
                          std::string message)
        {
            HierarchyValidationIssue issue{};
            issue.entity = entity;
            issue.related = related;
            issue.type = type;
            issue.message = std::move(message);
            report.issues.push_back(std::move(issue));
            ++report.metrics.issue_count;
            switch (type)
            {
            case HierarchyIssueType::Cycle:
                ++report.metrics.cycle_count;
                break;
            case HierarchyIssueType::DanglingParent:
                ++report.metrics.dangling_parent_count;
                break;
            case HierarchyIssueType::MissingParentHierarchy:
                ++report.metrics.missing_parent_hierarchy_count;
                break;
            case HierarchyIssueType::NonFiniteLocalTransform:
            case HierarchyIssueType::NonFiniteWorldTransform:
                ++report.metrics.non_finite_transform_count;
                break;
            case HierarchyIssueType::TransformMismatch:
                ++report.metrics.transform_mismatch_count;
                break;
            }
        }

        [[nodiscard]] bool is_finite(const math::Transform<float>& transform) noexcept
        {
            const auto& scale = transform.scale;
            const auto& rotation = transform.rotation;
            const auto& translation = transform.translation;
            return std::isfinite(scale[0]) && std::isfinite(scale[1]) && std::isfinite(scale[2]) &&
                   std::isfinite(rotation.w) && std::isfinite(rotation.x) && std::isfinite(rotation.y) &&
                   std::isfinite(rotation.z) &&
                   std::isfinite(translation[0]) && std::isfinite(translation[1]) && std::isfinite(translation[2]);
        }

        [[nodiscard]] float max_transform_delta(const math::Transform<float>& lhs,
                                                const math::Transform<float>& rhs) noexcept
        {
            float max_delta = 0.0F;
            for (int i = 0; i < 3; ++i)
            {
                max_delta = std::max(max_delta, std::fabs(lhs.scale[i] - rhs.scale[i]));
                max_delta = std::max(max_delta, std::fabs(lhs.translation[i] - rhs.translation[i]));
            }
            const auto diff_w = std::fabs(lhs.rotation.w - rhs.rotation.w);
            const auto diff_x = std::fabs(lhs.rotation.x - rhs.rotation.x);
            const auto diff_y = std::fabs(lhs.rotation.y - rhs.rotation.y);
            const auto diff_z = std::fabs(lhs.rotation.z - rhs.rotation.z);
            max_delta = std::max({max_delta, diff_w, diff_x, diff_y, diff_z});

            // Account for quaternion double-cover (q and -q encode the same rotation).
            const auto neg_diff_w = std::fabs(lhs.rotation.w + rhs.rotation.w);
            const auto neg_diff_x = std::fabs(lhs.rotation.x + rhs.rotation.x);
            const auto neg_diff_y = std::fabs(lhs.rotation.y + rhs.rotation.y);
            const auto neg_diff_z = std::fabs(lhs.rotation.z + rhs.rotation.z);
            const float neg_max = std::max({neg_diff_w, neg_diff_x, neg_diff_y, neg_diff_z});
            return std::min(max_delta, neg_max);
        }

        [[nodiscard]] bool has_dirty_ancestor(const entt::registry& registry, entity_type entity)
        {
            auto current = entity;
            while (current != entt::null)
            {
                if (!registry.valid(current))
                {
                    return true;
                }

                if (registry.any_of<components::DirtyTransform>(current))
                {
                    return true;
                }

                const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
                current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
            }
            return false;
        }

        using TransformCache = std::unordered_map<entity_type, std::optional<math::Transform<float>>>;
        using RecursionSet = std::unordered_set<entity_type>;

        std::optional<math::Transform<float>> compute_expected_world(
            const entt::registry& registry,
            entity_type entity,
            TransformCache& cache,
            RecursionSet& recursion_guard)
        {
            if (auto it = cache.find(entity); it != cache.end())
            {
                return it->second;
            }

            if (!recursion_guard.insert(entity).second)
            {
                return std::nullopt;
            }

            const auto* local = registry.try_get<components::LocalTransform>(entity);
            const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);

            std::optional<math::Transform<float>> parent_world{};
            if (hierarchy != nullptr && hierarchy->parent != entt::null && registry.valid(hierarchy->parent))
            {
                parent_world = compute_expected_world(registry, hierarchy->parent, cache, recursion_guard);
            }

            recursion_guard.erase(entity);

            if (local == nullptr)
            {
                cache.emplace(entity, parent_world);
                return parent_world;
            }

            math::Transform<float> combined = local->value;
            if (parent_world.has_value())
            {
                combined = math::combine(parent_world.value(), local->value);
            }
            cache.emplace(entity, combined);
            return combined;
        }

        void validate_hierarchy_links(const entt::registry& registry, HierarchyValidationReport& report)
        {
            std::unordered_set<entity_type> processed{};
            auto view = registry.view<const components::Hierarchy>();

            for (auto entity : view)
            {
                if (processed.contains(entity))
                {
                    continue;
                }

                std::unordered_set<entity_type> path{};
                entity_type current = entity;
                entity_type child = entt::null;

                while (current != entt::null)
                {
                    processed.insert(current);
                    const auto* current_hierarchy = registry.try_get<components::Hierarchy>(current);

                    if (child != entt::null && current == child)
                    {
                        record_issue(report,
                                     child,
                                     current,
                                     HierarchyIssueType::Cycle,
                                     "Entity references itself as a parent");
                        break;
                    }

                    if (!registry.valid(current))
                    {
                        std::ostringstream message;
                        message << "Entity " << format_entity(child)
                                << " references invalid parent " << format_entity(current);
                        record_issue(report, child, current, HierarchyIssueType::DanglingParent, message.str());
                        break;
                    }

                    if (current_hierarchy == nullptr)
                    {
                        std::ostringstream message;
                        message << "Entity " << format_entity(child)
                                << " references parent " << format_entity(current)
                                << " that is missing a Hierarchy component";
                        record_issue(report,
                                     child,
                                     current,
                                     HierarchyIssueType::MissingParentHierarchy,
                                     message.str());
                        break;
                    }

                    if (!path.insert(current).second)
                    {
                        std::ostringstream message;
                        message << "Cycle detected when traversing parent chain for entity "
                                << format_entity(child) << " via parent " << format_entity(current);
                        record_issue(report,
                                     child,
                                     current,
                                     HierarchyIssueType::Cycle,
                                     message.str());
                        break;
                    }

                    child = current;
                    current = current_hierarchy->parent;
                }
            }
        }

        void validate_transforms(const entt::registry& registry,
                                 const HierarchyValidationOptions& options,
                                 HierarchyValidationReport& report)
        {
            TransformCache cache{};
            RecursionSet recursion_guard{};
            auto view = registry.view<const components::LocalTransform>();

            for (auto entity : view)
            {
                const auto* local = registry.try_get<components::LocalTransform>(entity);
                if (local == nullptr)
                {
                    continue;
                }

                if (!is_finite(local->value))
                {
                    std::ostringstream message;
                    message << "Entity " << format_entity(entity)
                            << " has non-finite values in its LocalTransform";
                    record_issue(report,
                                 entity,
                                 entt::null,
                                 HierarchyIssueType::NonFiniteLocalTransform,
                                 message.str());
                }

                const auto* world = registry.try_get<components::WorldTransform>(entity);
                if (world != nullptr && !is_finite(world->value))
                {
                    std::ostringstream message;
                    message << "Entity " << format_entity(entity)
                            << " has non-finite values in its WorldTransform";
                    record_issue(report,
                                 entity,
                                 entt::null,
                                 HierarchyIssueType::NonFiniteWorldTransform,
                                 message.str());
                }

                if (world == nullptr)
                {
                    continue;
                }

                if (options.skip_dirty_entities && has_dirty_ancestor(registry, entity))
                {
                    continue;
                }

                recursion_guard.clear();
                const auto expected = compute_expected_world(registry, entity, cache, recursion_guard);
                if (!expected.has_value())
                {
                    continue;
                }

                const float delta = max_transform_delta(expected.value(), world->value);
                if (delta > options.transform_tolerance)
                {
                    std::ostringstream message;
                    message << "World transform mismatch for entity " << format_entity(entity)
                            << " exceeds tolerance (" << delta << ")";
                    record_issue(report,
                                 entity,
                                 entt::null,
                                 HierarchyIssueType::TransformMismatch,
                                 message.str());
                }
            }
        }
    } // namespace

    HierarchyValidationReport validate_hierarchy(const entt::registry& registry,
                                                 const HierarchyValidationOptions& options)
    {
        HierarchyValidationReport report{};
        validate_hierarchy_links(registry, report);
        validate_transforms(registry, options, report);
        return report;
    }

    HierarchyValidationReport validate_hierarchy(const Scene& scene,
                                                 const HierarchyValidationOptions& options)
    {
        return validate_hierarchy(scene.registry(), options);
    }
} // namespace engine::scene::validation
