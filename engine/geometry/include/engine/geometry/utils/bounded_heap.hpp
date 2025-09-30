#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>
#include <stdexcept>

namespace engine::geometry::utils {
    // Keeps the k *smallest* T by operator<. top() = current worst (largest) in the heap.
    // Works great with T = std::pair<float,size_t> (lexicographic compare).
    template<typename T>
    class BoundedHeap {
    public:
        explicit BoundedHeap(size_t max_size) : max_size_(max_size) {
            data_.reserve(max_size_);
        }

        // Add an item; ignores it if the heap is full and item is not better than the current worst.
        void push(const T& item) {
            if (max_size_ == 0) return;                  // handle k == 0 quietly

            if (data_.size() < max_size_) {
                data_.push_back(item);
                std::push_heap(data_.begin(), data_.end()); // default = max-heap
            } else if (item < data_.front()) {
                // Replace current worst (largest) with better (smaller) item.
                std::pop_heap(data_.begin(), data_.end());
                data_.back() = item;
                std::push_heap(data_.begin(), data_.end());
            }
            // NOTE: If you want "keep first-seen on ties", change condition to:
            //   if (item < data_.front() && !(data_.front() < item)) { ... }
            // or equivalently keep strict < but normalize T's tie-break policy.
        }

        // Largest element (i.e., current worst). Precondition: not empty.
        const T& top() const {
            if (data_.empty()) throw std::out_of_range("BoundedHeap::top on empty heap");
            return data_.front();
        }

        size_t size() const { return data_.size(); }
        bool empty() const { return data_.empty(); }
        size_t capacity() const { return max_size_; }

        void clear() { data_.clear(); }

        // Convenience: +inf if not full, otherwise the current worst (useful as pruning threshold).
        T threshold() const {
            if (data_.size() < max_size_) {
                // Caller commonly uses T = pair<float,size_t>; you can specialize if you like.
                return T{}; // Be careful: for distances you'd rather manage tau externally.
            }
            return top();
        }

        // Returns items sorted ascending (best..worst) **without** destroying the heap.
        std::vector<T> get_sorted_data() const {
            std::vector<T> out = data_;
            std::sort(out.begin(), out.end()); // ascending: best first
            return out;
        }

    private:
        size_t max_size_;
        std::vector<T> data_; // max-heap by operator< (largest at front)
    };
}