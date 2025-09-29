#pragma once

#include "property_set.hpp"

#include <ostream>
#include <cstdint>
#include <limits>
#include <compare>
#include <utility>
#include <vector>

namespace engine::geometry {
    using PropertyIndex = std::uint32_t;
    constexpr PropertyIndex kInvalidPropertyIndex = std::numeric_limits<PropertyIndex>::max();

    class PropertyHandle {
    public:
        using index_type = PropertyIndex;

        constexpr PropertyHandle() noexcept = default;

        explicit constexpr PropertyHandle(index_type idx) noexcept : index_(idx) {
        }

        [[nodiscard]] constexpr index_type &index() noexcept { return index_; }
        [[nodiscard]] constexpr index_type index() const noexcept { return index_; }
        [[nodiscard]] constexpr bool is_valid() const noexcept { return index_ != kInvalidPropertyIndex; }
        constexpr void reset() noexcept { index_ = kInvalidPropertyIndex; }

        [[nodiscard]] auto operator<=>(const PropertyHandle &) const noexcept = default;

    protected:
        index_type index_{kInvalidPropertyIndex};
    };

    class VertexHandle final : public PropertyHandle {
    public:
        using PropertyHandle::PropertyHandle;
    };

    class HalfedgeHandle final : public PropertyHandle {
    public:
        using PropertyHandle::PropertyHandle;
    };

    class EdgeHandle final : public PropertyHandle {
    public:
        using PropertyHandle::PropertyHandle;
    };

    class FaceHandle final : public PropertyHandle {
    public:
        using PropertyHandle::PropertyHandle;
    };

    class NodeHandle final : public PropertyHandle {
    public:
        using PropertyHandle::PropertyHandle;
    };

    std::ostream &operator<<(std::ostream &os, VertexHandle v);

    std::ostream &operator<<(std::ostream &os, HalfedgeHandle h);

    std::ostream &operator<<(std::ostream &os, EdgeHandle e);

    std::ostream &operator<<(std::ostream &os, FaceHandle f);

    std::ostream &operator<<(std::ostream &os, NodeHandle n);

    template<class T>
    using VertexProperty = HandleProperty<VertexHandle, T>;

    template<class T>
    using HalfedgeProperty = HandleProperty<HalfedgeHandle, T>;

    template<class T>
    using EdgeProperty = HandleProperty<EdgeHandle, T>;

    template<class T>
    using FaceProperty = HandleProperty<FaceHandle, T>;

    template<class T>
    using NodeProperty = HandleProperty<NodeHandle, T>;
}
