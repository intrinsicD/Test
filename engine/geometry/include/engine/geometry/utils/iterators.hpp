#pragma once

#include <iterator>
#include <cassert>


namespace engine::geometry
{
    template <class DataContainer, typename HandleType>
    class Iterator
    {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HandleType;
        using reference = HandleType&;
        using pointer = HandleType*;
        using iterator_category = std::bidirectional_iterator_tag;
        using handle_type = HandleType;
        using data_container_type = DataContainer;

        Iterator() = default;
        explicit Iterator(handle_type handle, const data_container_type* m = nullptr);

        [[nodiscard]] handle_type operator*() const { return handle_; }
        [[nodiscard]] auto operator<=>(const Iterator& rhs) const = default;

        Iterator& operator++();
        Iterator operator++(int);
        Iterator& operator--();
        Iterator operator--(int);

    private:
        handle_type handle_{};
        const data_container_type* data_{nullptr};
    };

    template <class DataContainer, typename HandleType>
    Iterator<DataContainer,
             HandleType>::Iterator(HandleType handle, const data_container_type* m) : handle_(handle), data_(m)
    {
        if (data_ && data_->has_garbage())
        {
            while (data_->is_valid(handle_) && data_->is_deleted(handle_)) { ++handle_.index(); }
        }
    }

    template <class DataContainer, typename HandleType>
    Iterator<DataContainer, HandleType>& Iterator<DataContainer, HandleType>::operator++()
    {
        ++handle_.index();
        assert(data_);
        while (data_->has_garbage() && data_->is_valid(handle_) && data_->is_deleted(handle_))
        {
            ++handle_.index();
        }
        return *this;
    }

    template <class DataContainer, typename HandleType>
    Iterator<DataContainer, HandleType> Iterator<DataContainer, HandleType>::operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    template <class DataContainer, typename HandleType>
    Iterator<DataContainer, HandleType>& Iterator<DataContainer, HandleType>::operator--()
    {
        --handle_.index();
        assert(data_);
        while (data_->has_garbage() && data_->is_valid(handle_) && data_->is_deleted(handle_))
        {
            --handle_.index();
        }
        return *this;
    }

    template <class DataContainer, typename HandleType>
    Iterator<DataContainer, HandleType> Iterator<DataContainer, HandleType>::operator--(int)
    {
        auto tmp = *this;
        --(*this);
        return tmp;
    }
}
