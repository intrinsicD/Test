#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/properties/property_handle.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"
#include "engine/geometry/utils/bounded_heap.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <queue>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace engine::geometry
{
    class ENGINE_GEOMETRY_API KdTree
    {
    public:
        struct Node
        {
            Aabb aabb{};
            std::size_t first_point = 0;
            std::size_t num_points = 0;
            std::array<std::size_t, 2> children{};
            std::uint8_t split_axis = 0U;
            float split_position = 0.0f;
            bool is_leaf = true;

            Node()
            {
                children.fill(NodeHandle().index());
            }
        };

        Nodes node_props_;
        NodeProperty<Node> nodes;

        Property<math::vec3> points;

        template <class T>
        [[nodiscard]] NodeProperty<T> add_node_property(const std::string& name, T default_value = T())
        {
            return NodeProperty<T>(node_props_.add<T>(name, default_value));
        }

        template <class T>
        [[nodiscard]] NodeProperty<T> get_node_property(const std::string& name) const
        {
            return NodeProperty<T>(node_props_.get<T>(name));
        }

        template <class T>
        [[nodiscard]] NodeProperty<T> node_property(const std::string& name, T default_value = T())
        {
            return NodeProperty<T>(node_props_.get_or_add<T>(name, default_value));
        }

        template <class T>
        void remove_node_property(NodeProperty<T>& prop)
        {
            node_props_.remove(prop);
        }

        [[nodiscard]] bool has_node_property(const std::string& name) const { return node_props_.exists(name); }

        [[nodiscard]] std::size_t get_max_points_per_leaf() const noexcept { return max_points_per_leaf_; }

        [[nodiscard]] std::size_t get_max_depth() const noexcept { return max_depth_; }

        [[nodiscard]] const std::vector<std::size_t>& get_point_indices() const noexcept { return point_indices_; }

        // Rebuild the tree from the supplied position property.
        bool build(const Property<math::vec3>& positions, std::size_t max_points_per_leaf, std::size_t max_depth)
        {
            points = positions;
            if (!points)
            {
                return false;
            }

            max_points_per_leaf_ = std::max<std::size_t>(1, max_points_per_leaf);
            max_depth_ = std::max<std::size_t>(1, max_depth);

            const std::size_t num_points = points.vector().size();
            if (num_points == 0)
            {
                node_props_.clear();
                point_indices_.clear();
                return false;
            }

            node_props_.clear();
            node_props_.reserve(num_points * 2);

            point_indices_.resize(num_points);
            std::iota(point_indices_.begin(), point_indices_.end(), 0);

            nodes = add_node_property<Node>("n:nodes");
            const NodeHandle root = create_node();
            nodes[root].first_point = 0;
            nodes[root].num_points = num_points;
            nodes[root].aabb = compute_bounds(0, num_points);

            // The root owns the entire index span and recursively partitions it.
            build_node(root, 0, 0, num_points);
            return true;
        }

        // Collect every point contained inside the axis-aligned query volume.
        void query(const Aabb& region, std::vector<std::size_t>& result) const
        {
            result.clear();
            if (node_props_.empty()) return;

            std::vector<NodeHandle> stack{NodeHandle{0}};
            while (!stack.empty())
            {
                const NodeHandle node_idx = stack.back();
                stack.pop_back();
                const Node& node = nodes[node_idx];

                if (!Intersects(node.aabb, region))
                {
                    continue;
                }

                if (node.is_leaf)
                {
                    for (std::size_t i = 0; i < node.num_points; ++i)
                    {
                        const std::size_t pi = point_indices_[node.first_point + i];
                        if (Contains(region, points[pi]))
                        {
                            result.push_back(pi);
                        }
                    }
                }
                else
                {
                    for (const auto child_index : node.children)
                    {
                        const NodeHandle child(child_index);
                        if (child.is_valid())
                        {
                            stack.push_back(child);
                        }
                    }
                }
            }
        }

        // Collect all points whose Euclidean distance from the query point is below the radius.
        void query_radius(const math::vec3& query_point, float radius, std::vector<std::size_t>& result) const
        {
            result.clear();
            if (node_props_.empty() || radius < 0.0f) return;

            const float radius_sq = radius * radius;
            std::vector<NodeHandle> stack{NodeHandle{0}};
            while (!stack.empty())
            {
                const NodeHandle node_idx = stack.back();
                stack.pop_back();
                const Node& node = nodes[node_idx];

                if (static_cast<float>(SquaredDistance(node.aabb, query_point)) > radius_sq)
                {
                    continue;
                }

                if (node.is_leaf)
                {
                    for (std::size_t i = 0; i < node.num_points; ++i)
                    {
                        const std::size_t pi = point_indices_[node.first_point + i];
                        const math::vec3 diff = points[pi] - query_point;
                        if (math::length_squared(diff) <= radius_sq)
                        {
                            result.push_back(pi);
                        }
                    }
                }
                else
                {
                    for (const auto child_index : node.children)
                    {
                        const NodeHandle child(child_index);
                        if (child.is_valid())
                        {
                            stack.push_back(child);
                        }
                    }
                }
            }
        }

        // Return the indices of the k closest points using a best-first traversal.
        void query_knn(const math::vec3& query_point, std::size_t k, std::vector<std::size_t>& results) const
        {
            results.clear();
            if (node_props_.empty() || k == 0) return;

            using QueueElement = std::pair<float, std::size_t>;
            utils::BoundedHeap<QueueElement> heap(k);

            using Traversal = std::pair<float, NodeHandle>;
            std::priority_queue<Traversal, std::vector<Traversal>, std::greater<>> pq;

            auto node_distance = [&](NodeHandle ni)
            {
                return static_cast<float>(SquaredDistance(nodes[ni].aabb, query_point));
            };
            auto point_distance = [&](std::size_t pi)
            {
                const math::vec3 diff = points[pi] - query_point;
                return math::length_squared(diff);
            };

            constexpr NodeHandle root(0);
            pq.emplace(node_distance(root), root);
            float tau = std::numeric_limits<float>::infinity();
            auto update_tau = [&]()
            {
                tau = (heap.size() == k) ? heap.top().first : std::numeric_limits<float>::infinity();
            };

            while (!pq.empty())
            {
                const float node_dist = pq.top().first;
                const NodeHandle node_idx = pq.top().second;
                pq.pop();

                if (heap.size() == k && node_dist > tau)
                {
                    break;
                }

                const Node& node = nodes[node_idx];
                if (node.is_leaf)
                {
                    for (std::size_t i = 0; i < node.num_points; ++i)
                    {
                        const std::size_t pi = point_indices_[node.first_point + i];
                        const float dist = point_distance(pi);
                        const QueueElement candidate(dist, pi);
                        if (heap.size() < k || candidate < heap.top())
                        {
                            heap.push(candidate);
                            update_tau();
                        }
                    }
                }
                else
                {
                    for (const auto child_index : node.children)
                    {
                        const NodeHandle child(child_index);
                        if (!child.is_valid())
                        {
                            continue;
                        }
                        const float child_dist = node_distance(child);
                        if (child_dist <= tau)
                        {
                            pq.emplace(child_dist, child);
                        }
                    }
                }
            }

            auto data = heap.get_sorted_data();
            results.resize(data.size());
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                results[i] = data[i].second;
            }
        }

        // Return the index of the closest point, or max() if the tree is empty.
        void query_nearest(const math::vec3& query_point, std::size_t& result) const
        {
            result = std::numeric_limits<std::size_t>::max();
            if (node_props_.empty())
            {
                return;
            }

            double best_dist_sq = std::numeric_limits<double>::max();
            using Traversal = std::pair<float, NodeHandle>;
            std::priority_queue<Traversal, std::vector<Traversal>, std::greater<>> pq;

            auto node_distance = [&](NodeHandle ni)
            {
                return static_cast<float>(SquaredDistance(nodes[ni].aabb, query_point));
            };
            auto point_distance = [&](std::size_t pi)
            {
                const math::vec3 diff = points[pi] - query_point;
                return static_cast<double>(math::length_squared(diff));
            };

            constexpr NodeHandle root(0);
            pq.emplace(node_distance(root), root);

            while (!pq.empty())
            {
                const float node_dist = pq.top().first;
                const NodeHandle node_idx = pq.top().second;
                pq.pop();

                if (static_cast<double>(node_dist) >= best_dist_sq)
                {
                    break;
                }

                const Node& node = nodes[node_idx];
                if (node.is_leaf)
                {
                    for (std::size_t i = 0; i < node.num_points; ++i)
                    {
                        const std::size_t pi = point_indices_[node.first_point + i];
                        const double dist_sq = point_distance(pi);
                        if (dist_sq < best_dist_sq)
                        {
                            best_dist_sq = dist_sq;
                            result = pi;
                        }
                    }
                }
                else
                {
                    for (const auto child_index : node.children)
                    {
                        const NodeHandle child(child_index);
                        if (!child.is_valid())
                        {
                            continue;
                        }
                        const float child_dist = node_distance(child);
                        if (static_cast<double>(child_dist) < best_dist_sq)
                        {
                            pq.emplace(child_dist, child);
                        }
                    }
                }
            }
        }

        [[nodiscard]] bool validate_structure() const
        {
            if (node_props_.empty())
            {
                return point_indices_.empty();
            }
            return validate_node(NodeHandle{0});
        }

    private:
        [[nodiscard]] NodeHandle create_node()
        {
            node_props_.push_back();
            return NodeHandle(node_props_.size() - 1);
        }

        [[nodiscard]] Aabb compute_bounds(std::size_t first, std::size_t count) const
        {
            if (count == 0)
            {
                return {};
            }

            Aabb bounds = BoundingAabb(points[point_indices_[first]]);
            for (std::size_t i = 1; i < count; ++i)
            {
                const std::size_t pi = point_indices_[first + i];
                Merge(bounds, points[pi]);
            }
            return bounds;
        }

        void build_node(NodeHandle node_idx, std::size_t depth, std::size_t begin, std::size_t end)
        {
            Node& node = nodes[node_idx];
            node.first_point = begin;
            node.num_points = end - begin;
            node.aabb = compute_bounds(begin, node.num_points);

            if (depth >= max_depth_ || node.num_points <= max_points_per_leaf_)
            {
                node.is_leaf = true;
                return;
            }

            const math::vec3 extent = Extent(node.aabb);
            int axis = 0;
            if (extent[1] > extent[0]) axis = 1;
            if (extent[2] > extent[axis]) axis = 2;

            if (extent[axis] <= std::numeric_limits<float>::epsilon())
            {
                node.is_leaf = true;
                return;
            }

            const std::size_t count = node.num_points;
            const std::size_t mid = begin + count / 2;
            auto comp = [&](std::size_t lhs, std::size_t rhs)
            {
                return points[lhs][axis] < points[rhs][axis];
            };
            std::nth_element(point_indices_.begin() + begin, point_indices_.begin() + mid,
                             point_indices_.begin() + end, comp);

            const std::size_t left_count = mid - begin;
            const std::size_t right_count = end - mid;
            if (left_count == 0 || right_count == 0)
            {
                node.is_leaf = true;
                return;
            }

            node.is_leaf = false;
            node.split_axis = static_cast<std::uint8_t>(axis);
            const std::size_t median_index = point_indices_[mid];
            node.split_position = points[median_index][axis];

            const NodeHandle left_child = create_node();
            nodes[node_idx].children[0] = left_child.index();
            build_node(left_child, depth + 1, begin, mid);

            const NodeHandle right_child = create_node();
            nodes[node_idx].children[1] = right_child.index();
            build_node(right_child, depth + 1, mid, end);
        }

        [[nodiscard]] bool validate_node(NodeHandle node_idx) const
        {
            const Node& node = nodes[node_idx];
            if (node.first_point + node.num_points > point_indices_.size())
            {
                return false;
            }

            if (node.is_leaf)
            {
                return true;
            }

            const NodeHandle left(node.children[0]);
            const NodeHandle right(node.children[1]);
            if (!left.is_valid() || !right.is_valid())
            {
                return false;
            }

            const Node& left_node = nodes[left];
            const Node& right_node = nodes[right];

            if (left_node.first_point != node.first_point)
            {
                return false;
            }
            if (left_node.first_point + left_node.num_points != right_node.first_point)
            {
                return false;
            }
            if (right_node.first_point + right_node.num_points != node.first_point + node.num_points)
            {
                return false;
            }

            return validate_node(left) && validate_node(right);
        }

        std::size_t max_points_per_leaf_ = 16;
        std::size_t max_depth_ = 24;
        std::vector<std::size_t> point_indices_;
    };
}
