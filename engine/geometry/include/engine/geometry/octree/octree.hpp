#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/properties/property_handle.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/utils/shape_interactions.hpp"

#include "engine/geometry/utils/bounded_heap.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <algorithm>
#include <queue>
#include <limits>
#include <numeric>
#include <iterator>
#include <utility>

namespace engine::geometry
{
    class ENGINE_GEOMETRY_API Octree
    {
    public:
        struct Node
        {
            Aabb aabb;
            size_t first_element = std::numeric_limits<size_t>::max();
            size_t num_straddlers = 0; // number of elements that straddle child node boundaries
            size_t num_elements = 0;
            // total number of elements in this node (including straddlers).Necessary for early out in queries
            std::array<size_t, 8> children{};
            bool is_leaf = true;

            Node()
            {
                children.fill(NodeHandle().index());
            }

            friend std::ostream& operator<<(std::ostream& os, const Node& n)
            {
                os << " aabb_min: " << n.aabb.min;
                os << " aabb_max: " << n.aabb.max;
                os << " first_element: " << n.first_element;
                os << " num_straddlers: " << n.num_straddlers;
                os << " num_elements: " << n.num_elements;
                os << " is_leaf: " << n.is_leaf;
                os << " children: ";
                for (const auto& c : n.children)
                {
                    os << c << " ";
                }
                return os;
            }
        };

        enum class SplitPoint { Center, Mean, Median };

        struct SplitPolicy
        {
            SplitPoint split_point = SplitPoint::Center;
            bool tight_children = false; // shrink child boxes to exactly fit contents
            float epsilon = 0.0f; // optional padding when tightening
        };

        Nodes node_props_;
        NodeProperty<Node> nodes;

        Property<Aabb> element_aabbs;

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


        [[nodiscard]] size_t get_max_elements_per_node() const noexcept
        {
            return max_elements_per_node;
        }

        [[nodiscard]] size_t get_max_octree_depth() const noexcept
        {
            return max_octree_depth;
        }

        [[nodiscard]] const SplitPolicy& get_split_policy() const noexcept
        {
            return split_policy;
        }

        [[nodiscard]] const std::vector<size_t>& get_element_indices() const noexcept
        {
            return element_indices;
        }

        bool build(const Property<Aabb>& aabbs, const SplitPolicy& policy, const size_t max_per_node,
                   const size_t max_depth)
        {
            element_aabbs = aabbs;

            if (!element_aabbs)
            {
                return false;
            }

            split_policy = policy;
            max_elements_per_node = max_per_node;
            max_octree_depth = max_depth;

            node_props_.clear(); // Clear previous state
            const size_t num_elements = element_aabbs.vector().size();

            if (num_elements == 0)
            {
                element_indices.clear();
                return false;
            }

            element_indices.resize(num_elements);
            std::iota(element_indices.begin(), element_indices.end(), 0);

            nodes = add_node_property<Node>("n:nodes");

            // Create root node
            const auto root_idx = create_node();
            nodes[root_idx].first_element = 0;
            nodes[root_idx].num_elements = num_elements;
            nodes[root_idx].aabb = BoundingAabb(element_aabbs.span());

            subdivide_volume(root_idx, 0);
            return true;
        }

        //TODO: can this work for all shapes which has Intersect and Contains Aabb?
        void query(const Aabb& query_aabb, std::vector<size_t>& result) const
        {
            result.clear();
            if (node_props_.empty()) return;

            constexpr double eps = 0.0; // set to a small positive tolerance if you want numerical slack
            const double query_volume = Volume(query_aabb);

            std::vector<NodeHandle> stack{NodeHandle{0}};
            while (!stack.empty())
            {
                const NodeHandle node_idx = stack.back();
                stack.pop_back();
                const Node& node = nodes[node_idx];

                if (!Intersects(node.aabb, query_aabb)) continue;


                const double node_volume = Volume(node.aabb);
                const double strictly_larger = (query_volume > node_volume + eps);

                if (strictly_larger && Contains(query_aabb, node.aabb))
                {
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        result.push_back(element_indices[node.first_element + i]);
                    }
                    continue;
                }

                if (node.is_leaf)
                {
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        size_t ei = element_indices[node.first_element + i];
                        if (Intersects(element_aabbs[ei], query_aabb))
                        {
                            result.push_back(ei);
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < node.num_straddlers; ++i)
                    {
                        size_t ei = element_indices[node.first_element + i];
                        if (Intersects(element_aabbs[ei], query_aabb))
                        {
                            result.push_back(ei);
                        }
                    }
                    for (const auto ci : node.children)
                    {
                        auto nhci = NodeHandle(ci);
                        if (nhci.is_valid() &&
                            Intersects(nodes[nhci].aabb, query_aabb))
                        {
                            stack.push_back(nhci);
                        }
                    }
                }
            }
        }

        //TODO: can this work for all shapes which has Intersect and Contains Aabb?
        void query(const Sphere& query_sphere, std::vector<size_t>& result) const
        {
            result.clear();
            if (node_props_.empty()) return;

            constexpr double eps = 0.0; // set small >0 for tolerance if desired
            const double query_volume = Volume(query_sphere);

            std::vector<NodeHandle> stack{NodeHandle{0}};
            while (!stack.empty())
            {
                const NodeHandle node_idx = stack.back();
                stack.pop_back();
                const Node& node = nodes[node_idx];

                if (!Intersects(node.aabb, query_sphere)) continue;

                const double node_volume = Volume(node.aabb);
                const double strictly_larger = (query_volume > node_volume + eps);

                if (strictly_larger && Contains(query_sphere, node.aabb))
                {
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        result.push_back(element_indices[node.first_element + i]);
                    }
                    continue;
                }

                if (node.is_leaf)
                {
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        size_t ei = element_indices[node.first_element + i];
                        if (Intersects(element_aabbs[ei], query_sphere))
                        {
                            result.push_back(ei);
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < node.num_straddlers; ++i)
                    {
                        size_t ei = element_indices[node.first_element + i];
                        if (Intersects(element_aabbs[ei], query_sphere))
                        {
                            result.push_back(ei);
                        }
                    }
                    for (const auto ci : node.children)
                    {
                        const auto nhci = NodeHandle(ci);
                        if (nhci.is_valid() &&
                            Intersects(nodes[nhci].aabb, query_sphere))
                        {
                            stack.push_back(nhci);
                        }
                    }
                }
            }
        }

        using QueueElement = std::pair<float, size_t>;

        void query_knn(const math::vec3& query_point, size_t k, std::vector<size_t>& results) const
        {
            results.clear();
            if (node_props_.empty() || k == 0) return;

            utils::BoundedHeap<QueueElement> heap(k);

            using Trav = std::pair<float, NodeHandle>; // (node lower-bound d2, node index)
            std::priority_queue<Trav, std::vector<Trav>, std::greater<Trav>> pq;

            constexpr NodeHandle root(0);
            auto d2_node = [&](NodeHandle ni)
            {
                return SquaredDistance(nodes[ni].aabb, query_point);
            };
            auto d2_elem = [&](size_t ei)
            {
                return SquaredDistance(
                    element_aabbs[ei], query_point);
            };

            pq.emplace(d2_node(root), root);
            float tau = std::numeric_limits<float>::infinity();
            auto update_tau = [&]()
            {
                tau = (heap.size() == k) ? heap.top().first : std::numeric_limits<float>::infinity();
            };

            while (!pq.empty())
            {
                auto [nd2, ni] = pq.top();
                pq.pop();

                // Global prune: the best remaining node is already worse than our kth best.
                if (heap.size() == k && nd2 >= tau) break;

                const Node& node = nodes[ni];

                if (node.is_leaf)
                {
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        const size_t ei = element_indices[node.first_element + i];
                        const float ed2 = d2_elem(ei);
                        if (heap.size() < k || ed2 < tau)
                        {
                            heap.push({ed2, ei});
                            update_tau();
                        }
                    }
                }
                else
                {
                    // Score straddlers at this node
                    for (size_t i = 0; i < node.num_straddlers; ++i)
                    {
                        const size_t ei = element_indices[node.first_element + i];
                        const double ed2 = d2_elem(ei);
                        if (heap.size() < k || ed2 < tau)
                        {
                            heap.push({ed2, ei});
                            update_tau();
                        }
                    }
                    // Push children best-first, pruned by current tau
                    for (const auto ci : node.children)
                    {
                        const auto nhci = NodeHandle(ci);
                        if (!nhci.is_valid()) continue;
                        const double cd2 = d2_node(nhci);
                        if (cd2 < tau) pq.emplace(cd2, ci);
                    }
                }
            }

            auto pairs = heap.get_sorted_data(); // ascending
            results.resize(pairs.size());
            for (size_t i = 0; i < pairs.size(); ++i) results[i] = pairs[i].second;
        }

        void query_nearest(const math::vec3& query_point, size_t& result) const
        {
            result = std::numeric_limits<size_t>::max();
            if (node_props_.empty())
            {
                return;
            }

            double min_dist_sq = std::numeric_limits<double>::max();

            using TraversalElement = std::pair<float, NodeHandle>;
            std::priority_queue<TraversalElement, std::vector<TraversalElement>, std::greater<TraversalElement>> pq;

            constexpr auto root_idx = NodeHandle(0);
            const double root_dist_sq = SquaredDistance(
                nodes[root_idx].aabb, query_point);
            pq.emplace(root_dist_sq, root_idx);

            while (!pq.empty())
            {
                const float node_dist_sq = pq.top().first;
                const NodeHandle node_idx = pq.top().second;
                pq.pop();

                if (node_dist_sq >= min_dist_sq)
                {
                    break;
                }

                const Node& node = nodes[node_idx];

                if (node.is_leaf)
                {
                    // This is a leaf, so process its elements.
                    for (size_t i = 0; i < node.num_elements; ++i)
                    {
                        assert(node.first_element + i < element_indices.size());
                        const size_t elem_idx = element_indices[node.first_element + i];
                        assert(elem_idx < element_aabbs.vector().size());
                        const double elem_dist_sq = SquaredDistance(element_aabbs[elem_idx], query_point);

                        if (elem_dist_sq < min_dist_sq)
                        {
                            min_dist_sq = elem_dist_sq;
                            result = elem_idx;
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < node.num_straddlers; ++i)
                    {
                        assert(node.first_element + i < element_indices.size());
                        const size_t elem_idx = element_indices[node.first_element + i];
                        assert(elem_idx < element_aabbs.vector().size());
                        const double elem_dist_sq = SquaredDistance(element_aabbs[elem_idx], query_point);

                        if (elem_dist_sq < min_dist_sq)
                        {
                            min_dist_sq = elem_dist_sq;
                            result = elem_idx;
                        }
                    }
                    // This is an internal node, so traverse to its children.
                    for (const auto child_idx : node.children)
                    {
                        const auto nhci = NodeHandle(child_idx);
                        if (nhci.is_valid())
                        {
                            const double child_dist_sq = SquaredDistance(nodes[nhci].aabb, query_point);
                            if (child_dist_sq < min_dist_sq)
                            {
                                pq.emplace(child_dist_sq, child_idx);
                            }
                        }
                    }
                }
            }
        }

        [[nodiscard]] bool validate_structure() const
        {
            if (node_props_.empty()) return element_indices.empty();
            return validate_node(NodeHandle{0});
        }

    private:
        [[nodiscard]] bool validate_node(NodeHandle node_idx) const
        {
            const Node& node = nodes[node_idx];
            if (node.first_element > element_indices.size()) return false;
            if (node.first_element + node.num_elements > element_indices.size()) return false;

            if (node.is_leaf)
            {
                return node.num_straddlers == 0;
            }

            size_t accumulated = node.first_element + node.num_straddlers;
            size_t child_total = 0;
            for (const auto ci : node.children)
            {
                const auto nhci = NodeHandle(ci);
                if (!nhci.is_valid()) continue;

                const Node& child = nodes[nhci];
                if (child.first_element != accumulated) return false;
                if (child.num_elements == 0) return false;
                if (child.first_element + child.num_elements > node.first_element + node.num_elements) return false;
                if (!validate_node(nhci)) return false;

                accumulated += child.num_elements;
                child_total += child.num_elements;
            }

            return accumulated == node.first_element + node.num_elements &&
                child_total + node.num_straddlers == node.num_elements;
        }

        NodeHandle create_node()
        {
            node_props_.push_back();
            return NodeHandle(node_props_.size() - 1);
        }

        void subdivide_volume(const NodeHandle node_idx, size_t depth)
        {
            const Node& node = nodes[node_idx]; // We'll be modifying the node

            if (depth >= max_octree_depth || node.num_elements <= max_elements_per_node)
            {
                nodes[node_idx].is_leaf = true;
                return;
            }

            math::vec3 sp = choose_split_point(node_idx);

            //Jitter/tighten the split point when it hits data
            for (int ax = 0; ax < 3; ++ax)
            {
                const float lo = node.aabb.min[ax], hi = node.aabb.max[ax];
                float& s = sp[ax];
                if (s <= lo || s >= hi) s = 0.5f * (lo + hi);
                if (s == lo) s = std::nextafter(s, hi);
                else if (s == hi) s = std::nextafter(s, lo);
            }

            std::array<Aabb, 8> octant_aabbs;
            for (int j = 0; j < 8; ++j)
            {
                math::vec3 child_min = {
                    (j & 1) ? sp[0] : node.aabb.min[0], (j & 2) ? sp[1] : node.aabb.min[1],
                    (j & 4) ? sp[2] : node.aabb.min[2]
                };
                math::vec3 child_max = {
                    (j & 1) ? node.aabb.max[0] : sp[0], (j & 2) ? node.aabb.max[1] : sp[1],
                    (j & 4) ? node.aabb.max[2] : sp[2]
                };
                octant_aabbs[j] = {.min = child_min, .max = child_max};
            }

            std::array<std::vector<size_t>, 8> child_elements;
            scratch_indices.clear();
            scratch_indices.reserve(node.num_elements);
            auto& straddlers = scratch_indices;

            for (size_t i = 0; i < node.num_elements; ++i)
            {
                size_t elem_idx = element_indices[node.first_element + i];
                const auto& elem_aabb = element_aabbs[elem_idx];
                int found_child = -1;

                if (elem_aabb.min == elem_aabb.max)
                {
                    const math::vec3& p = elem_aabb.min;
                    // Element is a point. Directly assign it to one of the octants.
                    int code = 0;
                    code |= (p[0] >= sp[0]) ? 1 : 0;
                    code |= (p[1] >= sp[1]) ? 2 : 0;
                    code |= (p[2] >= sp[2]) ? 4 : 0;
                    child_elements[code].push_back(elem_idx);
                }
                else
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        if (Contains(octant_aabbs[j], elem_aabb))
                        {
                            if (found_child == -1)
                            {
                                found_child = j;
                            }
                            else
                            {
                                // The element is contained in more than one child box, which shouldn't happen with this logic.
                                // Treat as a straddler just in case of floating point issues.
                                found_child = -1;
                                break;
                            }
                        }
                    }
                    if (found_child != -1)
                    {
                        child_elements[found_child].push_back(elem_idx);
                    }
                    else
                    {
                        // Fallback: assign by center if we will tighten children;
                        // otherwise keep as straddler to preserve correctness.
                        if (split_policy.tight_children)
                        {
                            const math::vec3 c = Center(elem_aabb);
                            int code = 0;
                            code |= (c[0] >= sp[0]) ? 1 : 0;
                            code |= (c[1] >= sp[1]) ? 2 : 0;
                            code |= (c[2] >= sp[2]) ? 4 : 0;
                            child_elements[code].push_back(elem_idx);
                        }
                        else
                        {
                            straddlers.push_back(elem_idx);
                        }
                    }
                }
            }

            // If we couldn't push a significant number of elements down, it's better to stop and make this a leaf.
            // This prevents creating child nodes with very few elements.
            if (straddlers.size() == node.num_elements)
            {
                nodes[node_idx].is_leaf = true;
                return;
            }

            // This node is now an internal node. It stores nothing itself.
            // Re-arrange the element_indices array.
            size_t current_pos = node.first_element;
            // First, place all the straddlers
            for (size_t idx : straddlers)
            {
                element_indices[current_pos++] = idx;
            }
            // Then, place the elements for each child sequentially
            std::array<size_t, 8> child_starts;
            for (int i = 0; i < 8; ++i)
            {
                child_starts[i] = current_pos;
                for (size_t idx : child_elements[i])
                {
                    element_indices[current_pos++] = idx;
                }
            }

            // --- This node is now officially an internal node ---
            // Its 'first_element' points to the start of the straddlers
            // Its 'num_straddlers' counts how many straddlers there are
            // Its 'num_elements' counts all elements (straddlers + children)
            // Its children[] point to the new child nodes (created below)
            // We need to keep the straddlers at the start of the range for correct querying,
            // But we also still need to keep track of the total number of elements for early out. This is important!
            nodes[node_idx].is_leaf = false;
            nodes[node_idx].num_straddlers = straddlers.size();

            // Create children and recurse
            for (int i = 0; i < 8; ++i)
            {
                if (!child_elements[i].empty())
                {
                    const auto child_node_handle = create_node();
                    nodes[node_idx].children[i] = child_node_handle.index();

                    Node& child = nodes[child_node_handle];
                    child.first_element = child_starts[i];
                    child.num_elements = child_elements[i].size();

                    if (split_policy.tight_children)
                    {
                        child.aabb = tight_child_aabb(child_elements[i].begin(), child_elements[i].end(),
                                                      split_policy.epsilon);
                    }
                    else
                    {
                        child.aabb = octant_aabbs[i];
                    }

                    subdivide_volume(child_node_handle, depth + 1);
                }
            }
        }

        [[nodiscard]] math::vec3 compute_mean_center(size_t first, size_t size,
                                                     const math::vec3& fallback_center) const
        {
            if (size == 0)
            {
                return fallback_center; // fallback; or pass node_idx and use aabbs[node_idx]
            }
            math::vec3 acc(0.0f, 0.0f, 0.0f);
            for (size_t i = 0; i < size; ++i)
            {
                const auto idx = element_indices[first + i];
                acc += Center(element_aabbs[idx]);
            }
            return acc / float(size);
        }

        [[nodiscard]] math::vec3 compute_median_center(size_t first, size_t size,
                                                       const math::vec3& fallback_center) const
        {
            if (size == 0)
            {
                return fallback_center; // fallback; or pass node_idx and use aabbs[node_idx]
            }
            std::vector<math::vec3> centers;
            centers.reserve(size);
            for (size_t i = 0; i < size; ++i)
            {
                centers.push_back(Center(element_aabbs[element_indices[first + i]]));
            }
            const size_t median_idx = centers.size() / 2;
            auto kth = [](std::vector<math::vec3>& centers, size_t median_idx, int dim)
            {
                std::nth_element(centers.begin(), centers.begin() + median_idx, centers.end(),
                                 [dim](const auto& a, const auto& b) { return a[dim] < b[dim]; });
                return centers[median_idx][dim];
            };
            return {kth(centers, median_idx, 0), kth(centers, median_idx, 1), kth(centers, median_idx, 2)};
        }

        [[nodiscard]] math::vec3 choose_split_point(NodeHandle node_idx) const
        {
            const auto& node = nodes[node_idx];
            const math::vec3 fallback_center = Center(nodes[node_idx].aabb);
            switch (split_policy.split_point)
            {
            case SplitPoint::Mean: return compute_mean_center(node.first_element, node.num_elements,
                                                              fallback_center);
            case SplitPoint::Median: return compute_median_center(node.first_element, node.num_elements,
                                                                  fallback_center);
            case SplitPoint::Center:
            default: return fallback_center;
            }
        }

        template <typename FwdIt>
        [[nodiscard]] Aabb tight_child_aabb(FwdIt begin, FwdIt end, float eps = 0.0f) const
        {
            if (begin == end)
            {
                return Aabb(); // Return an explicitly invalid AABB
            }

            Aabb tight = element_aabbs[*begin];

            for (auto it = std::next(begin); it != end; ++it)
            {
                Merge(tight, element_aabbs[*it]);
            }

            if (eps > 0.0f)
            {
                math::vec3 padding(eps, eps, eps);
                tight.min -= padding;
                tight.max += padding;
            }
            return tight;
        }

        [[nodiscard]] Aabb tight_child_aabb(const std::vector<size_t>& elems, float eps = 0.0f) const
        {
            return tight_child_aabb(elems.begin(), elems.end(), eps);
        }

        size_t max_elements_per_node = 32;
        size_t max_octree_depth = 10;
        SplitPolicy split_policy;
        std::vector<size_t> element_indices;
        std::vector<size_t> scratch_indices;
    };
}
