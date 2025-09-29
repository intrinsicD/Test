#pragma once

namespace engine::geometry {
    template<typename IteratorType>
    class Range {
    public:
        using iterator_type = IteratorType;
        Range(iterator_type begin, iterator_type end) : begin_(begin), end_(end) {}
        [[nodiscard]] iterator_type begin() const { return begin_; }
        [[nodiscard]] iterator_type end() const { return end_; }
    private:
        iterator_type begin_;
        iterator_type end_;
    };
} // namespace engine::geometry