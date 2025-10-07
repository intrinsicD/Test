#pragma once

#include <cassert>
#include <iterator>
#include <cstddef>

namespace engine::geometry {
    template<typename DataStructureType>
    class VertexAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = VertexHandle;
        using reference = VertexHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        VertexAroundVertexCirculator() = default;

        VertexAroundVertexCirculator(const DataStructureType *data_structure_, VertexHandle v) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(v);
            }
        }

        bool operator==(const VertexAroundVertexCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const VertexAroundVertexCirculator &rhs) const { return !(*this == rhs); }

        VertexAroundVertexCirculator &operator++() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->ccw_rotated_halfedge(halfedge_);
            is_active_ = true;
            return *this;
        }

        VertexAroundVertexCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        VertexAroundVertexCirculator &operator--() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->cw_rotated_halfedge(halfedge_);
            return *this;
        }

        VertexAroundVertexCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] VertexHandle operator*() const {
            assert(data_structure_ != nullptr);
            return data_structure_->to_vertex(halfedge_);
        }

        explicit operator bool() const { return halfedge_.is_valid(); }

        VertexAroundVertexCirculator &begin() {
            is_active_ = !halfedge_.is_valid();
            return *this;
        }

        VertexAroundVertexCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

        [[nodiscard]] HalfedgeHandle halfedge() const { return halfedge_; }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    template<typename DataStructureType>
    class HalfedgeAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HalfedgeHandle;
        using reference = HalfedgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        HalfedgeAroundVertexCirculator() = default;

        HalfedgeAroundVertexCirculator(const DataStructureType *data_structure_, VertexHandle v) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(v);
            }
        }

        bool operator==(const HalfedgeAroundVertexCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const HalfedgeAroundVertexCirculator &rhs) const { return !(*this == rhs); }

        HalfedgeAroundVertexCirculator &operator++() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->ccw_rotated_halfedge(halfedge_);
            is_active_ = true;
            return *this;
        }

        HalfedgeAroundVertexCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        HalfedgeAroundVertexCirculator &operator--() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->cw_rotated_halfedge(halfedge_);
            return *this;
        }

        HalfedgeAroundVertexCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] HalfedgeHandle operator*() const { return halfedge_; }
        explicit operator bool() const { return halfedge_.is_valid(); }

        HalfedgeAroundVertexCirculator &begin() {
            is_active_ = !halfedge_.is_valid();
            return *this;
        }

        HalfedgeAroundVertexCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    template<typename DataStructureType>
    class EdgeAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = EdgeHandle;
        using reference = EdgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        EdgeAroundVertexCirculator() = default;

        EdgeAroundVertexCirculator(const DataStructureType *data_structure_, VertexHandle v) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(v);
            }
        }

        bool operator==(const EdgeAroundVertexCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const EdgeAroundVertexCirculator &rhs) const { return !(*this == rhs); }

        EdgeAroundVertexCirculator &operator++() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->ccw_rotated_halfedge(halfedge_);
            is_active_ = true;
            return *this;
        }

        EdgeAroundVertexCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        EdgeAroundVertexCirculator &operator--() {
            assert(data_structure_ != nullptr);
            halfedge_ = data_structure_->cw_rotated_halfedge(halfedge_);
            return *this;
        }

        EdgeAroundVertexCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] EdgeHandle operator*() const {
            assert(data_structure_ != nullptr);
            return data_structure_->edge(halfedge_);
        }

        explicit operator bool() const { return halfedge_.is_valid(); }

        EdgeAroundVertexCirculator &begin() {
            is_active_ = !halfedge_.is_valid();
            return *this;
        }

        EdgeAroundVertexCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    template<typename DataStructureType>
    class FaceAroundVertexCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = FaceHandle;
        using reference = FaceHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        FaceAroundVertexCirculator() = default;

        FaceAroundVertexCirculator(const DataStructureType *data_structure_, VertexHandle v) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(v);
                if (halfedge_.is_valid() && data_structure_->is_boundary(halfedge_)) {
                    operator++();
                }
            }
        }

        bool operator==(const FaceAroundVertexCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const FaceAroundVertexCirculator &rhs) const { return !(*this == rhs); }

        FaceAroundVertexCirculator &operator++() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            do {
                halfedge_ = data_structure_->ccw_rotated_halfedge(halfedge_);
            } while (data_structure_->is_boundary(halfedge_));
            is_active_ = true;
            return *this;
        }

        FaceAroundVertexCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        FaceAroundVertexCirculator &operator--() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            do {
                halfedge_ = data_structure_->cw_rotated_halfedge(halfedge_);
            } while (data_structure_->is_boundary(halfedge_));
            return *this;
        }

        FaceAroundVertexCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] FaceHandle operator*() const {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            return data_structure_->face(halfedge_);
        }

        explicit operator bool() const { return halfedge_.is_valid(); }

        FaceAroundVertexCirculator &begin() {
            is_active_ = !halfedge_.is_valid();
            return *this;
        }

        FaceAroundVertexCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    template<typename DataStructureType>
    class VertexAroundFaceCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = VertexHandle;
        using reference = VertexHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        VertexAroundFaceCirculator() = default;

        VertexAroundFaceCirculator(const DataStructureType *data_structure_, FaceHandle f) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(f);
            }
        }

        bool operator==(const VertexAroundFaceCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const VertexAroundFaceCirculator &rhs) const { return !(*this == rhs); }

        VertexAroundFaceCirculator &operator++() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            halfedge_ = data_structure_->next_halfedge(halfedge_);
            is_active_ = true;
            return *this;
        }

        VertexAroundFaceCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        VertexAroundFaceCirculator &operator--() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            halfedge_ = data_structure_->prev_halfedge(halfedge_);
            return *this;
        }

        VertexAroundFaceCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] VertexHandle operator*() const {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            return data_structure_->to_vertex(halfedge_);
        }

        VertexAroundFaceCirculator &begin() {
            is_active_ = false;
            return *this;
        }

        VertexAroundFaceCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };

    template<typename DataStructureType>
    class HalfedgeAroundFaceCirculator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HalfedgeHandle;
        using reference = HalfedgeHandle;
        using pointer = void;
        using iterator_category = std::bidirectional_iterator_tag;
        using data_structure_type = DataStructureType;

        HalfedgeAroundFaceCirculator() = default;

        HalfedgeAroundFaceCirculator(const DataStructureType *data_structure_, FaceHandle f) : data_structure_(
            data_structure_) {
            if (data_structure_) {
                halfedge_ = data_structure_->halfedge(f);
            }
        }

        bool operator==(const HalfedgeAroundFaceCirculator &rhs) const {
            assert(data_structure_ != nullptr);
            assert(data_structure_ == rhs.data_structure_);
            return is_active_ && (halfedge_ == rhs.halfedge_);
        }

        bool operator!=(const HalfedgeAroundFaceCirculator &rhs) const { return !(*this == rhs); }

        HalfedgeAroundFaceCirculator &operator++() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            halfedge_ = data_structure_->next_halfedge(halfedge_);
            is_active_ = true;
            return *this;
        }

        HalfedgeAroundFaceCirculator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        HalfedgeAroundFaceCirculator &operator--() {
            assert(data_structure_ != nullptr && halfedge_.is_valid());
            halfedge_ = data_structure_->prev_halfedge(halfedge_);
            return *this;
        }

        HalfedgeAroundFaceCirculator operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        [[nodiscard]] HalfedgeHandle operator*() const { return halfedge_; }

        HalfedgeAroundFaceCirculator &begin() {
            is_active_ = false;
            return *this;
        }

        HalfedgeAroundFaceCirculator &end() {
            is_active_ = true;
            return *this;
        }

        [[nodiscard]] std::size_t size()  {
            return std::distance(begin(), end());
        }

    private:
        const data_structure_type *data_structure_{nullptr};
        HalfedgeHandle halfedge_{};
        bool is_active_{true};
    };
} // namespace engine::geometry
