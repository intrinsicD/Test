#pragma once

#include "engine/geometry/property_set.hpp"
#include "engine/math/vector.hpp"

#include <compare>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace engine::geometry {

using PointIndex = std::uint32_t;

constexpr PointIndex kInvalidPointIndex = std::numeric_limits<PointIndex>::max();

class PointHandle {
public:
    using index_type = PointIndex;

    constexpr PointHandle() noexcept = default;
    explicit constexpr PointHandle(index_type idx) noexcept : index_(idx) {}

    [[nodiscard]] constexpr index_type index() const noexcept { return index_; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return index_ != kInvalidPointIndex; }
    constexpr void reset() noexcept { index_ = kInvalidPointIndex; }

    [[nodiscard]] auto operator<=>(const PointHandle&) const noexcept = default;

private:
    index_type index_{kInvalidPointIndex};
};

std::ostream& operator<<(std::ostream& os, PointHandle p);

class PointCloud {
public:
    using Point = math::vec3;

    PointCloud();
    PointCloud(const PointCloud& rhs);
    PointCloud(PointCloud&&) noexcept = default;
    PointCloud& operator=(const PointCloud& rhs);
    PointCloud& operator=(PointCloud&&) noexcept = default;
    ~PointCloud();

    void clear();

    void reserve(std::size_t count);

    [[nodiscard]] std::size_t point_count() const noexcept { return point_props_.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return point_count() == 0; }

    [[nodiscard]] bool is_valid(PointHandle handle) const noexcept
    {
        return handle.is_valid() && handle.index() < point_count();
    }

    PointHandle add_point(const Point& point);

    [[nodiscard]] const Point& position(PointHandle handle) const { return point_positions_[handle]; }
    [[nodiscard]] Point& position(PointHandle handle) { return point_positions_[handle]; }

    [[nodiscard]] std::vector<Point>& positions() { return point_positions_.vector(); }
    [[nodiscard]] const std::vector<Point>& positions() const { return point_positions_.vector(); }

    template <class T>
    [[nodiscard]] HandleProperty<PointHandle, T> add_point_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<PointHandle, T>(point_props_.add<T>(name, default_value));
    }

    template <class T>
    [[nodiscard]] HandleProperty<PointHandle, T> get_point_property(const std::string& name) const
    {
        return HandleProperty<PointHandle, T>(point_props_.get<T>(name));
    }

    template <class T>
    [[nodiscard]] HandleProperty<PointHandle, T> point_property(const std::string& name, T default_value = T())
    {
        return HandleProperty<PointHandle, T>(point_props_.get_or_add<T>(name, default_value));
    }

    template <class T>
    void remove_point_property(HandleProperty<PointHandle, T>& property)
    {
        point_props_.remove(property);
    }

private:
    void ensure_position_property();

    PropertySet point_props_;
    HandleProperty<PointHandle, Point> point_positions_;
};

} // namespace engine::geometry

