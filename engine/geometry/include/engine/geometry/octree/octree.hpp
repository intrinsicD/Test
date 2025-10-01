#pragma once

#include "engine/geometry/api.hpp"
#include "engine/geometry/properties/property_set.hpp"
#include "engine/geometry/properties/property_handle.hpp"
#include "engine/geometry/shapes/aabb.hpp"
#include "engine/geometry/shapes/sphere.hpp"
#include "engine/geometry/utils/bounded_heap.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <algorithm>
#include <queue>
#include <limits>
#include <numeric>
#include <iterator>
#include <utility>
#include <numbers>

namespace engine::geometry {
    class Octree {
    public:
        struct Node {
            Aabb aabb;
            size_t first_element = std::numeric_limits<size_t>::max();
            size_t num_straddlers = 0;
            size_t num_elements = 0;
            std::array<size_t, 8> children;
            bool is_leaf = true;

            Node() {
                children.fill(std::numeric_limits<size_t>::max());
            }

            friend std::ostream &operator<<(std::ostream &os, const Node &n) {
                os << " aabb_min: " << n.aabb.min;
                os << " aabb_max: " << n.aabb.max;
                os << " first_element: " << n.first_element;
                os << " num_straddlers: " << n.num_straddlers;
                os << " num_elements: " << n.num_elements;
                os << " is_leaf: " << n.is_leaf;
                os << " children: ";
                for (const auto &c: n.children) {
                    os << c << " ";
                }
                return os;
            }
        };

        enum class SplitPoint { Center, Mean, Median };

        struct SplitPolicy {
            SplitPoint split_point = SplitPoint::Center;
            bool tight_children = false; // shrink child boxes to exactly fit contents
            float epsilon = 0.0f; // optional padding when tightening
        };

        Nodes node_props_;
        NodeProperty<Node> nodes;

        Property<Aabb > element_aabbs;

        [[nodiscard]] size_t get_max_elements_per_node() const noexcept {
            return max_elements_per_node;
        }

        [[nodiscard]] size_t get_max_octree_depth() const noexcept {
            return max_octree_depth;
        }

        [[nodiscard]] const SplitPolicy &get_split_policy() const noexcept {
            return split_policy;
        }

        [[nodiscard]] const std::vector<size_t> &get_element_indices() const noexcept {
            return element_indices;
        }

        void build(const Property<Aabb > &aabbs, const SplitPolicy &policy, const size_t max_per_node,
                   const size_t max_depth) {
            element_aabbs = aabbs;

            if (!element_aabbs) {
                Log::Error("Element AABBs property is not set. Cannot build octree.");
                return;
            }

            split_policy = policy;
            max_elements_per_node = max_per_node;
            max_octree_depth = max_depth;

            node_props_.clear(); // Clear previous state
            const size_t num_elements = element_aabbs.vector().size();

            if (num_elements == 0) {
                element_indices.clear();
                return;
            }

            element_indices.resize(num_elements);
            std::iota(element_indices.begin(), element_indices.end(), 0);

            nodes = node_props_.add<Node>("n:nodes");

            // Create root node
            const size_t root_idx = create_node();
            nodes[root_idx].first_element = 0;
            nodes[root_idx].num_elements = num_elements;
            nodes[root_idx].aabb = Aabb::Build(element_aabbs.vector().begin(), element_aabbs.vector().end());

            subdivide_volume(root_idx, 0);
        }

        void query(const Aabb &query_aabb, std::vector<size_t> &result) const {
            result.clear();
            if (node_props_.empty()) return;

            auto vol_aabb = [](const Aabb &b) {
                const Vector<float, 3> e = b.max - b.min;
                return std::max(0.0f, e.x) * std::max(0.0f, e.y) * std::max(0.0f, e.z);
            };
            const float eps = 0.0f; // set to a small positive tolerance if you want numerical slack

            std::vector<size_t> stack{0};
            while (!stack.empty()) {
                const size_t node_idx = stack.back();
                stack.pop_back();
                const Node &node = nodes[node_idx];

                if (!IntersectsTraits<Aabb, Aabb >::intersects(node.aabb, query_aabb)) continue;

                const float qv = vol_aabb(query_aabb);
                const float nv = vol_aabb(node.aabb);
                const bool strictly_larger = (qv > nv + eps);

                if (strictly_larger &&
                    ContainsTraits<Aabb, Aabb >::Contains(query_aabb, node.aabb)) {
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        result.push_back(element_indices[node.first_element + i]);
                    }
                    continue;
                }

                if (node.is_leaf) {
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        size_t ei = element_indices[node.first_element + i];
                        if (IntersectsTraits<Aabb, Aabb >::intersects(element_aabbs[ei], query_aabb)) {
                            result.push_back(ei);
                        }
                    }
                } else {
                    for (size_t i = 0; i < node.num_straddlers; ++i) {
                        size_t ei = element_indices[node.first_element + i];
                        if (IntersectsTraits<Aabb, Aabb >::intersects(element_aabbs[ei], query_aabb)) {
                            result.push_back(ei);
                        }
                    }
                    for (size_t ci: node.children) {
                        if (ci != std::numeric_limits<size_t>::max() &&
                            IntersectsTraits<Aabb, Aabb >::intersects(nodes[ci].aabb, query_aabb)) {
                            stack.push_back(ci);
                        }
                    }
                }
            }
        }

        void query(const Sphere<float> &query_sphere, std::vector<size_t> &result) const {
            result.clear();
            if (node_props_.empty()) return;

            auto vol_aabb = [](const Aabb &b) {
                const Vector<float, 3> e = b.max - b.min;
                return std::max(0.0f, e.x) * std::max(0.0f, e.y) * std::max(0.0f, e.z);
            };
            auto vol_sphere = [](const Sphere<float> &s) {
                return (4.0f / 3.0f) * glm::pi<float>() * s.radius * s.radius * s.radius;
            };
            const float eps = 0.0f; // set small >0 for tolerance if desired

            std::vector<size_t> stack{0};
            while (!stack.empty()) {
                const size_t node_idx = stack.back();
                stack.pop_back();
                const Node &node = nodes[node_idx];

                if (!IntersectsTraits<Aabb, Sphere<float> >::intersects(node.aabb, query_sphere)) continue;

                const bool strictly_larger = (vol_sphere(query_sphere) > vol_aabb(node.aabb) + eps);

                if (strictly_larger &&
                    ContainsTraits<Sphere<float>, Aabb >::Contains(query_sphere, node.aabb)) {
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        result.push_back(element_indices[node.first_element + i]);
                    }
                    continue;
                }

                if (node.is_leaf) {
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        size_t ei = element_indices[node.first_element + i];
                        if (IntersectsTraits<Aabb, Sphere<
                            float> >::intersects(element_aabbs[ei], query_sphere)) {
                            result.push_back(ei);
                        }
                    }
                } else {
                    for (size_t i = 0; i < node.num_straddlers; ++i) {
                        size_t ei = element_indices[node.first_element + i];
                        if (IntersectsTraits<Aabb, Sphere<
                            float> >::intersects(element_aabbs[ei], query_sphere)) {
                            result.push_back(ei);
                        }
                    }
                    for (size_t ci: node.children) {
                        if (ci != std::numeric_limits<size_t>::max() &&
                            IntersectsTraits<Aabb, Sphere<float> >::intersects(nodes[ci].aabb, query_sphere)) {
                            stack.push_back(ci);
                        }
                    }
                }
            }
        }

        using QueueElement = std::pair<float, size_t>;

        void query_knn(const Vector<float, 3> &query_point, size_t k, std::vector<size_t> &results) const {
            results.clear();
            if (node_props_.empty() || k == 0) return;

            BoundedHeap<QueueElement> heap(k);

            using Trav = std::pair<float, size_t>; // (node lower-bound d2, node index)
            std::priority_queue<Trav, std::vector<Trav>, std::greater<Trav> > pq;

            const size_t root = 0;
            auto d2_node = [&](size_t ni) {
                return SquaredDistanceTraits<Aabb, Vector<float, 3> >::squared_distance(
                    nodes[ni].aabb, query_point);
            };
            auto d2_elem = [&](size_t ei) {
                return SquaredDistanceTraits<Aabb, Vector<float, 3> >::squared_distance(
                    element_aabbs[ei], query_point);
            };

            pq.push({d2_node(root), root});
            float tau = std::numeric_limits<float>::infinity();
            auto update_tau = [&]() {
                tau = (heap.size() == k) ? heap.top().first : std::numeric_limits<float>::infinity();
            };

            while (!pq.empty()) {
                auto [nd2, ni] = pq.top();
                pq.pop();

                // Global prune: the best remaining node is already worse than our kth best.
                if (heap.size() == k && nd2 >= tau) break;

                const Node &node = nodes[ni];

                if (node.is_leaf) {
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        const size_t ei = element_indices[node.first_element + i];
                        const float ed2 = d2_elem(ei);
                        if (heap.size() < k || ed2 < tau) {
                            heap.push({ed2, ei});
                            update_tau();
                        }
                    }
                } else {
                    // Score straddlers at this node
                    for (size_t i = 0; i < node.num_straddlers; ++i) {
                        const size_t ei = element_indices[node.first_element + i];
                        const float ed2 = d2_elem(ei);
                        if (heap.size() < k || ed2 < tau) {
                            heap.push({ed2, ei});
                            update_tau();
                        }
                    }
                    // Push children best-first, pruned by current tau
                    for (size_t ci: node.children) {
                        if (ci == std::numeric_limits<size_t>::max()) continue;
                        const float cd2 = d2_node(ci);
                        if (cd2 < tau) pq.push({cd2, ci});
                    }
                }
            }

            auto pairs = heap.get_sorted_data(); // ascending
            results.resize(pairs.size());
            for (size_t i = 0; i < pairs.size(); ++i) results[i] = pairs[i].second;
        }

        void query_nearest(const Vector<float, 3> &query_point, size_t &result) const {
            result = std::numeric_limits<size_t>::max();
            if (node_props_.empty()) {
                return;
            }

            float min_dist_sq = std::numeric_limits<float>::max();

            using TraversalElement = std::pair<float, size_t>;
            std::priority_queue<TraversalElement, std::vector<TraversalElement>, std::greater<TraversalElement> > pq;

            const size_t root_idx = 0;
            const float root_dist_sq = SquaredDistanceTraits<Aabb, Vector<float, 3> >::squared_distance(
                nodes[root_idx].aabb, query_point);
            pq.push({root_dist_sq, root_idx});

            while (!pq.empty()) {
                const float node_dist_sq = pq.top().first;
                const size_t node_idx = pq.top().second;
                pq.pop();

                if (node_dist_sq >= min_dist_sq) {
                    break;
                }

                const Node &node = nodes[node_idx];

                if (node.is_leaf) {
                    // This is a leaf, so process its elements.
                    for (size_t i = 0; i < node.num_elements; ++i) {
                        const size_t elem_idx = element_indices[node.first_element + i];
                        const float elem_dist_sq = SquaredDistanceTraits<Aabb, Vector<float,
                            3> >::squared_distance(
                            element_aabbs[elem_idx], query_point);

                        if (elem_dist_sq < min_dist_sq) {
                            min_dist_sq = elem_dist_sq;
                            result = elem_idx;
                        }
                    }
                } else {
                    for (size_t i = 0; i < node.num_straddlers; ++i) {
                        const size_t elem_idx = element_indices[node.first_element + i];
                        const float elem_dist_sq = SquaredDistanceTraits<Aabb, Vector<float,
                            3> >::squared_distance(
                            element_aabbs[elem_idx], query_point);

                        if (elem_dist_sq < min_dist_sq) {
                            min_dist_sq = elem_dist_sq;
                            result = elem_idx;
                        }
                    }
                    // This is an internal node, so traverse to its children.
                    for (const size_t child_idx: node.children) {
                        if (child_idx != std::numeric_limits<size_t>::max()) {
                            const float child_dist_sq = SquaredDistanceTraits<Aabb, Vector<float,
                                3> >::squared_distance(
                                nodes[child_idx].aabb, query_point);
                            if (child_dist_sq < min_dist_sq) {
                                pq.push({child_dist_sq, child_idx});
                            }
                        }
                    }
                }
            }
        }

    private:
        size_t create_node() {
            node_props_.push_back();
            return node_props_.size() - 1;
        }

        void subdivide_volume(const size_t node_idx, size_t depth) {
            const Node &node = nodes[node_idx]; // We'll be modifying the node

            if (depth >= max_octree_depth || node.num_elements <= max_elements_per_node) {
                nodes[node_idx].is_leaf = true;
                return;
            }

            Vector<float, 3> sp = choose_split_point(node_idx);

            //Jitter/tighten the split point when it hits data
            for (int ax = 0; ax < 3; ++ax) {
                const float lo = node.aabb.min[ax], hi = node.aabb.max[ax];
                float &s = sp[ax];
                if (s <= lo || s >= hi) s = 0.5f * (lo + hi);
                if (s == lo) s = std::nextafter(s, hi);
                else if (s == hi) s = std::nextafter(s, lo);
            }

            std::array<Aabb, 8> octant_aabbs;
            for (int j = 0; j < 8; ++j) {
                Vector<float, 3> child_min = {
                    (j & 1) ? sp.x : node.aabb.min.x, (j & 2) ? sp.y : node.aabb.min.y,
                    (j & 4) ? sp.z : node.aabb.min.z
                };
                Vector<float, 3> child_max = {
                    (j & 1) ? node.aabb.max.x : sp.x, (j & 2) ? node.aabb.max.y : sp.y,
                    (j & 4) ? node.aabb.max.z : sp.z
                };
                octant_aabbs[j] = Aabb(child_min, child_max);
            }

            std::array<std::vector<size_t>, 8> child_elements;
            std::vector<size_t> straddlers;

            for (size_t i = 0; i < node.num_elements; ++i) {
                size_t elem_idx = element_indices[node.first_element + i];
                const auto &elem_aabb = element_aabbs[elem_idx];
                int found_child = -1;

                if (elem_aabb.min == elem_aabb.max) {
                    const Vector<float, 3> &p = elem_aabb.min;
                    // Element is a point. Directly assign it to one of the octants.
                    int code = 0;
                    code |= (p.x >= sp.x) ? 1 : 0;
                    code |= (p.y >= sp.y) ? 2 : 0;
                    code |= (p.z >= sp.z) ? 4 : 0;
                    child_elements[code].push_back(elem_idx);
                } else {
                    for (int j = 0; j < 8; ++j) {
                        if (ContainsTraits<Aabb, Aabb >::Contains(octant_aabbs[j], elem_aabb)) {
                            if (found_child == -1) {
                                found_child = j;
                            } else {
                                // The element is contained in more than one child box, which shouldn't happen with this logic.
                                // Treat as a straddler just in case of floating point issues.
                                found_child = -1;
                                break;
                            }
                        }
                    }
                    if (found_child != -1) {
                        child_elements[found_child].push_back(elem_idx);
                    } else {
                        // Fallback: assign by center if we will tighten children;
                        // otherwise keep as straddler to preserve correctness.
                        if (split_policy.tight_children) {
                            const Vector<float, 3> c = elem_aabb.center();
                            int code = 0;
                            code |= (c.x >= sp.x) ? 1 : 0;
                            code |= (c.y >= sp.y) ? 2 : 0;
                            code |= (c.z >= sp.z) ? 4 : 0;
                            child_elements[code].push_back(elem_idx);
                        } else {
                            straddlers.push_back(elem_idx);
                        }
                    }
                }
            }

            // If we couldn't push a significant number of elements down, it's better to stop and make this a leaf.
            // This prevents creating child nodes with very few elements.
            if (straddlers.size() == node.num_elements) {
                nodes[node_idx].is_leaf = true;
                return;
            }

            // This node is now an internal node. It stores nothing itself.
            // Re-arrange the element_indices array.
            size_t current_pos = node.first_element;
            // First, place all the straddlers
            for (size_t idx: straddlers) {
                element_indices[current_pos++] = idx;
            }
            // Then, place the elements for each child sequentially
            std::array<size_t, 8> child_starts;
            for (int i = 0; i < 8; ++i) {
                child_starts[i] = current_pos;
                for (size_t idx: child_elements[i]) {
                    element_indices[current_pos++] = idx;
                }
            }

            // --- This node is now officially an internal node ---
            // It's not a leaf. In our new design, it doesn't store any elements for querying.
            // Its 'size' and 'first_element' are now meaningless for querying, but we can set them to the straddlers.
            nodes[node_idx].is_leaf = false;
            nodes[node_idx].num_straddlers = straddlers.size();

            // Create children and recurse
            for (int i = 0; i < 8; ++i) {
                if (!child_elements[i].empty()) {
                    const size_t child_node_idx = create_node();
                    nodes[node_idx].children[i] = child_node_idx;

                    Node &child = nodes[child_node_idx];
                    child.first_element = child_starts[i];
                    child.num_elements = child_elements[i].size();

                    if (split_policy.tight_children) {
                        child.aabb = tight_child_aabb(child_elements[i].begin(), child_elements[i].end(),
                                                      split_policy.epsilon);
                    } else {
                        child.aabb = octant_aabbs[i];
                    }

                    subdivide_volume(child_node_idx, depth + 1);
                }
            }
        }

        [[nodiscard]] Vector<float, 3> compute_mean_center(size_t first, size_t size,
                                                           const Vector<float, 3> &fallback_center) const {
            if (size == 0) {
                return fallback_center; // fallback; or pass node_idx and use aabbs[node_idx]
            }
            Vector<float, 3> acc(0.0f, 0.0f, 0.0f);
            for (size_t i = 0; i < size; ++i) {
                const auto idx = element_indices[first + i];
                acc += element_aabbs[idx].center();
            }
            return acc / float(size);
        }

        [[nodiscard]] Vector<float, 3> compute_median_center(size_t first, size_t size,
                                                             const Vector<float, 3> &fallback_center) const {
            if (size == 0) {
                return fallback_center; // fallback; or pass node_idx and use aabbs[node_idx]
            }
            std::vector<Vector<float, 3> > centers;
            centers.reserve(size);
            for (size_t i = 0; i < size; ++i) {
                centers.push_back(element_aabbs[element_indices[first + i]].center());
            }
            const size_t median_idx = centers.size() / 2;
            auto kth = [](std::vector<Vector<float, 3> > &centers, size_t median_idx, int dim) {
                std::nth_element(centers.begin(), centers.begin() + median_idx, centers.end(),
                                 [dim](const auto &a, const auto &b) { return a[dim] < b[dim]; });
                return centers[median_idx][dim];
            };
            return {kth(centers, median_idx, 0), kth(centers, median_idx, 1), kth(centers, median_idx, 2)};
        }

        [[nodiscard]] Vector<float, 3> choose_split_point(size_t node_idx) const {
            const auto &node = nodes[node_idx];
            const Vector<float, 3> fallback_center = nodes[node_idx].aabb.center();
            switch (split_policy.split_point) {
                case SplitPoint::Mean: return compute_mean_center(node.first_element, node.num_elements,
                                                                  fallback_center);
                case SplitPoint::Median: return compute_median_center(node.first_element, node.num_elements,
                                                                      fallback_center);
                case SplitPoint::Center:
                default: return fallback_center;
            }
        }

        template<typename FwdIt>
        [[nodiscard]] Aabb tight_child_aabb(FwdIt begin, FwdIt end, float eps = 0.0f) const {
            if (begin == end) {
                return Aabb(); // Return an explicitly invalid AABB
            }

            Aabb tight = element_aabbs[*begin];

            for (auto it = std::next(begin); it != end; ++it) {
                tight.merge(element_aabbs[*it]);
            }

            if (eps > 0.0f) {
                Vector<float, 3> padding(eps, eps, eps);
                tight.min -= padding;
                tight.max += padding;
            }
            return tight;
        }

        [[nodiscard]] Aabb tight_child_aabb(const std::vector<size_t> &elems, float eps = 0.0f) const {
            return tight_child_aabb(elems.begin(), elems.end(), eps);
        }

        size_t max_elements_per_node = 32;
        size_t max_octree_depth = 10;
        SplitPolicy split_policy;
        std::vector<size_t> element_indices;
        std::vector<size_t> scratch_indices;
    };
}
