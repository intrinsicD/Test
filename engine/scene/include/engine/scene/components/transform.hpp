#pragma once

#include "engine/math/transform.hpp"

#include <entt/entt.hpp>

#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>

namespace engine::scene::components
{
    struct LocalTransform
    {
        math::Transform<float> value{math::Transform<float>::Identity()};
    };

    struct WorldTransform
    {
        math::Transform<float> value{math::Transform<float>::Identity()};
    };

    struct DirtyTransform
    {
    private:
        bool _{};
    };

    inline void mark_dirty(entt::registry& registry, entt::entity entity)
    {
        registry.emplace_or_replace<DirtyTransform>(entity);
    }

    namespace serialization
    {
        namespace detail
        {
            inline void encode_transform(std::ostream& output, const math::Transform<float>& transform)
            {
                const auto previous_precision = output.precision();
                output << std::setprecision(std::numeric_limits<float>::max_digits10)
                    << transform.scale[0] << ' '
                    << transform.scale[1] << ' '
                    << transform.scale[2] << ' '
                    << transform.rotation.w << ' '
                    << transform.rotation.x << ' '
                    << transform.rotation.y << ' '
                    << transform.rotation.z << ' '
                    << transform.translation[0] << ' '
                    << transform.translation[1] << ' '
                    << transform.translation[2];
                output.precision(previous_precision);
            }

            inline math::Transform<float> decode_transform(std::istream& input)
            {
                math::Transform<float> transform{};
                input >> transform.scale[0] >> transform.scale[1] >> transform.scale[2]
                    >> transform.rotation.w >> transform.rotation.x >> transform.rotation.y >> transform.rotation.z
                    >> transform.translation[0] >> transform.translation[1] >> transform.translation[2];
                return transform;
            }
        } // namespace detail

        inline void encode(std::ostream& output, const LocalTransform& transform)
        {
            detail::encode_transform(output, transform.value);
        }

        inline LocalTransform decode_local(std::istream& input)
        {
            LocalTransform transform{};
            transform.value = detail::decode_transform(input);
            return transform;
        }

        inline void encode(std::ostream& output, const WorldTransform& transform)
        {
            detail::encode_transform(output, transform.value);
        }

        inline WorldTransform decode_world(std::istream& input)
        {
            WorldTransform transform{};
            transform.value = detail::decode_transform(input);
            return transform;
        }

        inline void encode(std::ostream&, const DirtyTransform&)
        {
        }

        inline DirtyTransform decode_dirty(std::istream&)
        {
            return DirtyTransform{};
        }
    } // namespace serialization
} // namespace engine::scene::components
