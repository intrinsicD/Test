#include "../../include/engine/geometry/point_cloud/point_cloud.hpp"

#include <string>
#include <string_view>

namespace engine::geometry {

namespace {
constexpr std::string_view kPositionPropertyName{"p:position"};
constexpr math::vec3 kZeroPoint{0.0F, 0.0F, 0.0F};
}

std::ostream& operator<<(std::ostream& os, PointHandle p)
{
    return os << 'p' << p.index();
}

PointCloud::PointCloud()
{
    ensure_position_property();
}

PointCloud::PointCloud(const PointCloud& rhs)
    : point_props_(rhs.point_props_)
{
    ensure_position_property();
}

PointCloud& PointCloud::operator=(const PointCloud& rhs)
{
    if (this != &rhs)
    {
        point_props_ = rhs.point_props_;
        point_positions_.reset();
        ensure_position_property();
    }
    return *this;
}

PointCloud::~PointCloud() = default;

void PointCloud::clear()
{
    point_props_.clear();
    point_positions_.reset();
    ensure_position_property();
}

void PointCloud::reserve(std::size_t count)
{
    ensure_position_property();
    point_props_.reserve(count);
}

PointHandle PointCloud::add_point(const Point& point)
{
    ensure_position_property();
    const auto index = static_cast<PointHandle::index_type>(point_count());
    point_props_.push_back();
    const PointHandle handle{index};
    point_positions_[handle] = point;
    return handle;
}

void PointCloud::ensure_position_property()
{
    if (point_positions_.is_valid())
    {
        return;
    }

    auto property = point_props_.get<Point>(kPositionPropertyName);
    if (!property.is_valid())
    {
        property = point_props_.add<Point>(std::string(kPositionPropertyName), kZeroPoint);
    }
    point_positions_ = HandleProperty<PointHandle, Point>(property);
}

} // namespace engine::geometry

