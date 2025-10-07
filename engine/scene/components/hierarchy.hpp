#pragma once

#include <entt/entt.hpp>

#include <istream>
#include <ostream>
#include <type_traits>

namespace engine::scene::components
{
    struct Hierarchy
    {
        entt::entity parent{entt::null};
        entt::entity first_child{entt::null};
        entt::entity next_sibling{entt::null};
        entt::entity previous_sibling{entt::null};
    };

    [[nodiscard]] inline bool is_root(const Hierarchy& hierarchy) noexcept
    {
        return hierarchy.parent == entt::null;
    }

    [[nodiscard]] inline bool has_children(const Hierarchy& hierarchy) noexcept
    {
        return hierarchy.first_child != entt::null;
    }

    namespace serialization
    {
        struct HierarchyRecord
        {
            using entity_type = std::underlying_type_t<entt::entity>;

            entity_type parent{static_cast<entity_type>(entt::null)};
            entity_type first_child{static_cast<entity_type>(entt::null)};
            entity_type next_sibling{static_cast<entity_type>(entt::null)};
            entity_type previous_sibling{static_cast<entity_type>(entt::null)};

            [[nodiscard]] static constexpr entity_type null_value() noexcept
            {
                return static_cast<entity_type>(entt::null);
            }
        };

        inline void encode(std::ostream& output, const Hierarchy& hierarchy)
        {
            output << static_cast<HierarchyRecord::entity_type>(hierarchy.parent) << ' '
                   << static_cast<HierarchyRecord::entity_type>(hierarchy.first_child) << ' '
                   << static_cast<HierarchyRecord::entity_type>(hierarchy.next_sibling) << ' '
                   << static_cast<HierarchyRecord::entity_type>(hierarchy.previous_sibling);
        }

        inline HierarchyRecord decode_hierarchy(std::istream& input)
        {
            HierarchyRecord record{};
            input >> record.parent >> record.first_child >> record.next_sibling >> record.previous_sibling;
            return record;
        }

        template <typename Resolver>
        [[nodiscard]] inline Hierarchy instantiate(const HierarchyRecord& record, Resolver&& resolver)
        {
            const auto resolve = [&](HierarchyRecord::entity_type value) {
                if (value == HierarchyRecord::null_value())
                {
                    return entt::entity(entt::null);
                }

                return resolver(value);
            };

            return Hierarchy{
                resolve(record.parent),
                resolve(record.first_child),
                resolve(record.next_sibling),
                resolve(record.previous_sibling),
            };
        }
    } // namespace serialization
} // namespace engine::scene::components
